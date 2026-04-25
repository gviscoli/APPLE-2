/*
 * chatbot_streaming.c — Chatbot per Apple IIe con risposta in streaming
 *
 * I token dell'LLM vengono stampati a schermo man mano che arrivano,
 * dando l'effetto visivo dei caratteri che appaiono uno ad uno.
 *
 * Differenze rispetto a chatbot.c:
 *   - usa rest_lib_streaming.c (output diretto, nessun buffer di risposta)
 *   - do_post_stream() invece di do_post()
 *   - "BOT:" viene stampato PRIMA della chiamata al proxy
 *   - nessun word wrap sulla risposta (i token arrivano gia' formattati)
 *
 * Compilazione:
 *
 *  cl65 -t apple2 chatbot_streaming.c -o pch_chatbot_stream -O  -m chatbot_streaming.map -vm ../00.LIBRERIE/IP65/lib/ip65_tcp.lib ../00.LIBRERIE/IP65/drivers/ip65_apple2.lib
 * 
 *  java -jar C:\UTILITY\AppleCommander\AppleCommander-win32-x86_64-1.9.0.jar
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <conio.h>

#include "../00.LIBRERIE/IP65/inc/ip65.h"
#include "rest_lib_streaming.c"

/* -----------------------------------------------
 * Costanti schermo Apple IIe
 * ----------------------------------------------- */
#define SCREEN_COLS     40
#define SCREEN_ROWS     24
#define INPUT_MAX       120
#define JSON_BUF_SIZE   160

/* -----------------------------------------------
 * Buffer input e JSON (statici per non consumare stack)
 * ----------------------------------------------- */
static char input_buf[INPUT_MAX + 1];
static char json_buf[JSON_BUF_SIZE];

/* -----------------------------------------------
 * Utility: stampa riga orizzontale
 * ----------------------------------------------- */
static void print_line(void) {
    uint8_t i;
    for (i = 0; i < SCREEN_COLS - 10; i++) putchar('-');
    putchar('\n');
}

/* -----------------------------------------------
 * Leggi una riga da tastiera con echo
 * ----------------------------------------------- */
static uint8_t read_line(char *buf, uint8_t max_len) {
    uint8_t i = 0;
    char    c;

    while (i < max_len) {
        c = cgetc() & 0x7F;

        if (c == '\r' || c == '\n') {
            break;
        }
        if ((c == 0x08 || c == 0x7F) && i > 0) {
            i--;
            putchar(0x08);
            putchar(' ');
            putchar(0x08);
            continue;
        }
        if (c < 0x20) continue;

        buf[i++] = c;
        putchar(c);
    }

    buf[i] = '\0';
    putchar('\n');
    return i;
}

/* -----------------------------------------------
 * Costruisce il JSON body: {"prompt":"<testo>"}
 * ----------------------------------------------- */
static bool build_json(const char *prompt, char *out, uint8_t out_size) {
    uint8_t     pi = 0;
    uint8_t     oi = 0;
    uint8_t     prefix_len;
    const char *prefix = "{\"prompt\":\"";
    const char *suffix = "\"}";

    prefix_len = (uint8_t)strlen(prefix);

    if (oi + prefix_len >= out_size) return false;
    strcpy(out, prefix);
    oi = prefix_len;

    while (prompt[pi] && oi < out_size - 3) {
        if (prompt[pi] == '"' || prompt[pi] == '\\') {
            if (oi + 2 >= out_size - 2) break;
            out[oi++] = '\\';
        }
        out[oi++] = prompt[pi++];
    }

    if (oi + 2 >= out_size) return false;
    out[oi++] = '"';
    out[oi++] = '}';
    out[oi]   = '\0';

    return true;
}

/* ═══════════════════════════════════════════════
 * BANNER ASCII ART
 * ═══════════════════════════════════════════════ */
static void show_ascii_banner(void) {
    clrscr();

    puts("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
    puts("*                                     *");
    puts("*  =================================  *");
    puts("*  |      APPLE ][ - CPU: 6502     |  *");
    puts("*  |                               |  *");
    puts("*  | (powered by Viscoli Giuseppe) |  *");
    puts("*  =================================  *");
    puts("*                                     *");
    puts("*     ~~ LA MACCHINA DEL TEMPO ~~     *");
    puts("*      (PCH.CHATBOT.STREAM.BIN)       *");
    puts("*                                     *");
    puts("* [1983] --> Internet --> LLM [2026]  *");
    puts("*                                     *");
    puts("*   Una vecchia tecnologia che        *");
    puts("*   dialoga con l'AI del futuro!      *");
    puts("*                                     *");
    puts("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
    puts("");
    puts("  < Premi un tasto per iniziare... >");

    cgetc();
    clrscr();
}

/* -----------------------------------------------
 * Schermata di benvenuto
 * ----------------------------------------------- */
static void show_welcome(void) {
    clrscr();
    print_line();
    puts("");
    puts(" Digita la tua domanda e premi");
    puts(" INVIO. (Digita Q per uscire.)");
    puts("");
    print_line();
    puts("");
}

/* -----------------------------------------------
 * Inizializzazione rete
 * ----------------------------------------------- */
static unsigned int check_uthernet(void) {
    unsigned int n, slot;
    slot = (unsigned int)-1;
    for (n = 0; n < 7; n++) {
        if (ip65_init(n) == 0) {
            slot = n;
            break;
        }
    }
    return slot;
}

static bool initialize_network(void) {
    unsigned int slot;

    puts("Ricerca Uthernet...");
    slot = check_uthernet();
    if (slot == (unsigned int)-1) {
        puts("ERRORE: Uthernet non trovata!");
        return false;
    }
    printf("Uthernet slot %d OK\n", slot);

    puts("DHCP...");
    if (dhcp_init() != 0) {
        puts("ERRORE: DHCP fallito!");
        return false;
    }
    puts("Rete OK\n");
    return true;
}

/* -----------------------------------------------
 * Invia un messaggio e stampa la risposta in streaming
 *
 * L'intestazione "BOT:" viene stampata prima della
 * chiamata: i token arrivano direttamente a schermo
 * tramite tcp_stream_cb in rest_lib_streaming.c.
 * ----------------------------------------------- */
static bool send_message(const char *message) {
    int8_t result;

    if (!build_json(message, json_buf, JSON_BUF_SIZE)) {
        puts("[ERR] Messaggio troppo lungo!");
        return false;
    }

    puts("");
    print_line();
    puts("BOT:");
    print_line();

    /* I token vengono stampati direttamente dal callback TCP */
    result = do_post_stream("/api/data", json_buf);

    putchar('\n');   /* assicura che il cursore vada a capo dopo l'ultimo token */
    print_line();

    if (result < 0) {
        puts("[ERR] Connessione al proxy fallita.");
        puts("Verifica che llm_proxy_streaming.py sia attivo.");
        return false;
    }

    return true;
}

/* -----------------------------------------------
 * Entry Point
 * ----------------------------------------------- */
int main(void) {
    uint8_t  msg_len;
    uint8_t  turn = 0;

    show_ascii_banner();
    show_welcome();

    if (!initialize_network()) {
        puts("Premi un tasto...");
        cgetc();
        return 1;
    }

    /* Test connessione al proxy */
    {
        int8_t r;
        puts("Test proxy...");
        r = do_get_stream("/api/data?prompt=test");
        if (r < 0) {
            puts("\nATTENZIONE: proxy non raggiungibile.");
            puts("Continuo comunque...");
        } else {
            puts("\nProxy OK!\n");
        }
    }

    /* Loop principale chat */
    for (;;) {
        printf("TU[%d]: ", (int)turn + 1);
        msg_len = read_line(input_buf, INPUT_MAX);

        if (msg_len == 0) {
            puts("[Messaggio vuoto, riprova]");
            continue;
        }
        if (msg_len == 1 &&
            (input_buf[0] == 'Q' || input_buf[0] == 'q')) {
            break;
        }

        puts("Invio...");
        send_message(input_buf);

        turn++;
    }

    puts("\nArrivederci!");
    return 0;
}
