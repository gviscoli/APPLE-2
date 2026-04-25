/*
 * CHAT.C — Chatbot per Apple IIe
 *
 * Comunicazione con LLM tramite proxy Flask locale.
 * Usa rest_lib.c per GET/POST HTTP via ip65 + Uthernet II.
 *
 * Compilazione:
 * 
 *  cl65 -t apple2 chatbot.c -o pch_chatbot.bin -O  -m chatbot.map -vm ../00.LIBRERIE/IP65/lib/ip65_tcp.lib ../00.LIBRERIE/IP65/drivers/ip65_apple2.lib
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
#include "rest_lib.c"

/* -----------------------------------------------
 * Costanti schermo Apple IIe
 * 40 colonne in modalita' testo standard
 * ----------------------------------------------- */
#define SCREEN_COLS     40
#define SCREEN_ROWS     24
#define INPUT_MAX       120     /* max caratteri digitabili       */
#define JSON_BUF_SIZE   160     /* {"prompt":"..."} — max 160 ch  */

/* -----------------------------------------------
 * Buffer input utente e JSON
 * Statici per non consumare stack (6502 ha 256 byte di stack!)
 * ----------------------------------------------- */
static char input_buf[INPUT_MAX + 1];
static char json_buf[JSON_BUF_SIZE];

/* -----------------------------------------------
 * Utility: stampa riga orizzontale
 * ----------------------------------------------- */
static void print_line(void) {
    uint8_t i;
    for (i = 0; i < SCREEN_COLS-10; i++) putchar('-');
    putchar('\n');
}

/* -----------------------------------------------
 * Utility: stampa separatore per bot/utente
 * ----------------------------------------------- */
static void print_separator(void) {
    uint8_t i;
    for (i = 0; i < 5; i++) putchar('=');
    putchar('\n');
}

/* -----------------------------------------------
 * Utility: word wrap
 * Stampa una stringa andando a capo a SCREEN_COLS
 * senza spezzare le parole a meta'
 * ----------------------------------------------- */
static void print_wrapped(const char *text, uint8_t col_max) {
    uint8_t  col  = 0;
    uint8_t  i    = 0;
    uint8_t  wlen = 0;
    const char *p = text;
    const char *word_start;

    while (*p) {

        /* Salta spazi iniziali a inizio riga */
        if (col == 0) {
            while (*p == ' ') p++;
        }

        /* Trova fine parola corrente */
        word_start = p;
        wlen = 0;
        while (p[wlen] && p[wlen] != ' ' && p[wlen] != '\n') {
            wlen++;
        }

        /* Se la parola non entra, vai a capo */
        if (col > 0 && col + wlen >= col_max) {
            putchar('\n');
            col = 0;
        }

        /* Stampa la parola */
        for (i = 0; i < wlen; i++) {
            putchar(word_start[i]);
        }
        col += wlen;
        p   += wlen;

        /* Gestisci separatore */
        if (*p == '\n') {
            putchar('\n');
            col = 0;
            p++;
        } else if (*p == ' ') {
            if (col < col_max - 1) {
                putchar(' ');
                col++;
            }
            p++;
        }
    }

    if (col > 0) putchar('\n');
}

/* -----------------------------------------------
 * Leggi una riga da tastiera con echo
 * Gestisce: backspace, invio, limite caratteri
 * Ritorna: lunghezza stringa letta
 * ----------------------------------------------- */
static uint8_t read_line(char *buf, uint8_t max_len) {
    uint8_t i = 0;
    char    c;

    while (i < max_len) {
        c = cgetc() & 0x7F;   /* [FIX BUG-5] strip high bit */

        if (c == '\r' || c == '\n') {
            break;
        }
        if ((c == 0x08 || c == 0x7F) && i > 0) {
            /* Backspace */
            i--;
            putchar(0x08);
            putchar(' ');
            putchar(0x08);
            continue;
        }
        /* Ignora caratteri di controllo */
        if (c < 0x20) continue;

        buf[i++] = c;
        putchar(c);
    }

    buf[i] = '\0';
    putchar('\n');
    return i;
}

/* -----------------------------------------------
 * Costruisce il JSON body per la POST:
 *   {"prompt":"<testo>"}
 *
 * Caratteri speciali nel testo vengono escaped:
 *   "  ->  \"
 *   \  ->  \\
 *
 * Ritorna false se il testo e' troppo lungo
 * ----------------------------------------------- */
static bool build_json(const char *prompt, char *out, uint8_t out_size) {
    uint8_t  pi = 0;    /* indice in prompt */
    uint8_t  oi = 0;    /* indice in out    */
    uint8_t  prefix_len;
    const char *prefix = "{\"prompt\":\"";
    const char *suffix = "\"}";

    prefix_len = (uint8_t)strlen(prefix);

    /* Copia prefix */
    if (oi + prefix_len >= out_size) return false;
    strcpy(out, prefix);
    oi = prefix_len;

    /* Copia prompt con escaping */
    while (prompt[pi] && oi < out_size - 3) {
        if (prompt[pi] == '"' || prompt[pi] == '\\') {
            if (oi + 2 >= out_size - 2) break;
            out[oi++] = '\\';
        }
        out[oi++] = prompt[pi++];
    }

    /* Copia suffix */
    if (oi + 2 >= out_size) return false;
    out[oi++] = '"';
    out[oi++] = '}';
    out[oi]   = '\0';

    return true;
}

/* ═══════════════════════════════════════════════
 * BANNER ASCII ART — mostrato UNA SOLA VOLTA
 * all'avvio del programma.
 *
 * Ogni riga e' esattamente 40 caratteri (schermo
 * Apple IIe in modalita' testo standard).
 * ═══════════════════════════════════════════════ */
static void show_ascii_banner(void) {
    clrscr();

    /* Riga 1  */ puts("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
    /* Riga 2  */ puts("*                                     *");
    /* Riga 4  */ puts("*  =================================  *");    
    /* Riga 3  */ puts("*  |      APPLE ][ - CPU: 6502     |  *");
    /* Riga 4  */ puts("*  |                               |  *");    
    /* Riga 5  */ puts("*  | (powered by Viscoli Giuseppe) |  *");    
    /* Riga 6  */ puts("*  =================================  *");           
    /* Riga 7  */ puts("*                                     *");
    /* Riga 8  */ puts("*     ~~ LA MACCHINA DEL TEMPO ~~     *");
    /* Riga 9  */ puts("*           (PCH.CHATBOT.BIN)         *");
    /* Riga 10 */ puts("*                                     *");
    /* Riga 11 */ puts("* [1983] --> Internet --> LLM [2026]  *");
    /* Riga 12 */ puts("*                                     *");
    /* Riga 13 */ puts("*   Una vecchia tecnologia che        *");
    /* Riga 14 */ puts("*   dialoga con l'AI del futuro!      *");
    /* Riga 15 */ puts("*                                     *");
    /* Riga 16 */ puts("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
    /* Riga 17 */ puts("");
    /* Riga 18 */ puts("  < Premi un tasto per iniziare... >");

    cgetc();       /* attendi keypress                 */
    clrscr();      /* pulisci prima della welcome page */
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
 * Invia un messaggio al proxy e stampa la risposta
 * Ritorna: true se ok, false se errore
 * ----------------------------------------------- */
static bool send_message(const char *message) {
    int16_t result;

    /* Costruisci JSON */
    if (!build_json(message, json_buf, JSON_BUF_SIZE)) {
        puts("[ERR] Messaggio troppo lungo!");
        return false;
    }

    /* POST al proxy */
    result = do_post("/api/data",
                     json_buf,
                     response_buf,
                     RESPONSE_BUF_SIZE);

    if (result < 0) {
        puts("[ERR] Connessione al proxy fallita.");
        puts("Verifica che llm_proxy.py sia attivo.");
        return false;
    }

    /* Stampa risposta con word wrap */
    puts("");
    print_separator();
    puts("BOT:");
    print_separator();

    if (http_response_body && http_response_length > 0) {
        /* Assicura terminazione stringa */
        if (http_response_length < RESPONSE_BUF_SIZE) {
            http_response_body[http_response_length] = '\0';
        }
        print_wrapped((char *)http_response_body, SCREEN_COLS);
    } else {
        puts("[nessuna risposta]");
    }

    print_line();
    return true;
}

/* -----------------------------------------------
 * Entry Point
 * ----------------------------------------------- */
int main(void) {
    uint8_t  msg_len;
    uint8_t  turn = 0;     /* contatore turni conversazione */

    /* Banner ASCII art: mostrato una sola volta */
    show_ascii_banner();

    /* Welcome screen */    
    show_welcome();

    /* Inizializza rete */
    if (!initialize_network()) {
        puts("Premi un tasto...");
        cgetc();
        return 1;
    }

    /* Test connessione al proxy */
    // puts("Test proxy...");
    {
        int16_t r = do_get("/api/data?prompt=test",
                            response_buf,
                            RESPONSE_BUF_SIZE);
        if (r < 0) {
            puts("ATTENZIONE: proxy non raggiungibile.");
            puts("Continuo comunque...");
        } else {
            puts("Proxy OK!\n");
        }
    }

    /* ── Loop principale chat  */
    for (;;) {
        /* Prompt utente */
        print_separator();
        printf("TU[%d]: ", (int)turn + 1);


        msg_len = read_line(input_buf, INPUT_MAX);

        /* Gestisci comando uscita */
        if (msg_len == 0) {
            puts("[Messaggio vuoto, riprova]");
            continue;
        }
        if ((msg_len == 1) &&
            (input_buf[0] == 'Q' || input_buf[0] == 'q')) {
            break;
        }

        /* Invia al proxy LLM */
        // s("Invio...");
        print_separator();
        send_message(input_buf);

        turn++;

        // /* Pausa prima del prossimo turno */
        // puts("");
        // puts("[Premi un tasto per continuare]");
        // cgetc();
        // puts("");
    }

    puts("\nArrivederci!");
    return 0;
}