/*
 * CHATBOT.C — Chatbot per Apple IIe
 *
 * Comunicazione con LLM tramite proxy Flask locale.
 * Usa rest_lib.c per GET/POST HTTP via ip65 + Uthernet II.
 *
 * Compilazione:
 *
 *  cl65 -t apple2 chatbot.c -o pch_chatbot.bin -O -m chatbot.map -vm \
 *       ../00.LIBRERIE/IP65/lib/ip65_tcp.lib \
 *       ../00.LIBRERIE/IP65/drivers/ip65_apple2.lib
 *
 *  java -jar C:\UTILITY\AppleCommander\AppleCommander-win32-x86_64-1.9.0.jar
 *
 * ─────────────────────────────────────────────────────────────────────────────
 * NOTE DI REVISIONE (architettura 6502 / cc65):
 *
 * [BUG-1] tcp_recv_cb — firma __fastcall__
 *   Con cc65, __fastcall__ passa l'ULTIMO parametro nei registri A/X.
 *   Qui l'ultimo param e' "int len": A = byte basso, X = byte alto.
 *   E' FONDAMENTALE che questa firma coincida esattamente con quella
 *   attesa da ip65; verificare ip65.h prima di modificare.
 *
 * [BUG-2] Stack 6502 — 256 byte hardware
 *   La catena di chiamata piu' profonda e':
 *     main -> send_message -> do_post -> tcp_do_request -> ip65_process
 *   Ogni frame usa 2 byte di stack hardware (indirizzo di ritorno).
 *   5 livelli = 10 byte: sicuro. Le variabili locali cc65 usano lo
 *   "software stack" (puntatore in zero page), non lo stack hardware.
 *   Attenzione se si aggiungono livelli di indirection futuri.
 *
 * [BUG-3] Rilevamento fine-risposta HTTP/1.0
 *   Il loop di polling esce quando: buffer pieno | POLL_MAX_ITERATIONS |
 *   ip65_error != 0. Con HTTP/1.0 il server chiude la connessione
 *   dopo la risposta: ip65 imposta ip65_error = 0x84 ("reset/close").
 *   Il codice gestisce correttamente questo caso (esce dal loop e
 *   processa i dati ricevuti). Comportamento CORRETTO, ma vale la
 *   pena documentarlo esplicitamente.
 *
 * [BUG-4] build_json — out_size e' uint8_t
 *   Limite massimo 255 byte. JSON_BUF_SIZE = 160: ok.
 *   Se in futuro si aumenta JSON_BUF_SIZE oltre 255, cambiare
 *   il tipo del parametro in uint16_t.
 *
 * [BUG-5] cgetc() e bit alto su Apple IIe
 *   Il firmware Apple IIe imposta il bit 7 dei caratteri tastiera.
 *   cc65 cgetc() per target apple2 maschera gia' il bit 7, quindi
 *   il confronto con 'Q'/'q' e' corretto. Per robustezza, la
 *   read_line() qui sotto aggiunge esplicitamente la maschera 0x7F.
 *
 * [BUG-6] POLL_MAX_ITERATIONS = 10000
 *   A 1 MHz, ip65_process() impiega circa 1-5 ms per iterazione.
 *   10000 iterazioni = 10-50 secondi di timeout massimo.
 *   Adeguato per una rete locale, ma potrebbe sembrare un freeze
 *   a schermo se il proxy e' lento. Considerare un indicatore
 *   visivo di "attesa" nel loop (es. stampa di un '.' ogni N iter.).
 * ─────────────────────────────────────────────────────────────────────────────
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
 * Statici per non consumare stack (6502 ha 256 byte!)
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
 * Utility: word wrap
 * Stampa una stringa andando a capo a col_max
 * senza spezzare le parole a meta'
 * ----------------------------------------------- */
static void print_wrapped(const char *text, uint8_t col_max) {
    uint8_t     col  = 0;
    uint8_t     i    = 0;
    uint8_t     wlen = 0;
    const char *p    = text;
    const char *word_start;

    while (*p) {

        if (col == 0) {
            while (*p == ' ') p++;
        }

        word_start = p;
        wlen = 0;
        while (p[wlen] && p[wlen] != ' ' && p[wlen] != '\n') {
            wlen++;
        }

        if (col > 0 && col + wlen >= col_max) {
            putchar('\n');
            col = 0;
        }

        for (i = 0; i < wlen; i++) {
            putchar(word_start[i]);
        }
        col += wlen;
        p   += wlen;

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
 * [FIX BUG-5] maschera esplicita bit 7 (0x7F)
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
 * Costruisce il JSON body per la POST:
 *   {"prompt":"<testo>"}
 * Ritorna false se il testo e' troppo lungo
 * ----------------------------------------------- */
static bool build_json(const char *prompt, char *out, uint8_t out_size) {
    uint8_t     pi = 0;
    uint8_t     oi = 0;
    uint8_t     prefix_len;
    const char *prefix = "{\"prompt\":\"";
    const char *suffix = "\"}";

    (void)suffix;   /* usato solo come riferimento */
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
 * BANNER ASCII ART — mostrato UNA SOLA VOLTA
 * all'avvio del programma.
 *
 * Ogni riga e' esattamente 40 caratteri (schermo
 * Apple IIe in modalita' testo standard).
 * ═══════════════════════════════════════════════ */
static void show_ascii_banner(void) {
    clrscr();

    /* Riga 1  */ puts("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-");
    /* Riga 2  */ puts("*                                      *");
    /* Riga 3  */ puts("*           A P P L E  ] [             *");
    /* Riga 4  */ puts("*         =====================        *");
    /* Riga 5  */ puts("*         |  I  I  e  *  1983 |        *");
    /* Riga 6  */ puts("*         |   6 5 0 2  CPU   |        *");
    /* Riga 7  */ puts("*         |  1  M H z  *  64K|        *");
    /* Riga 8  */ puts("*         =====================        *");
    /* Riga 9  */ puts("*              [_______]               *");
    /* Riga 10 */ puts("*                                      *");
    /* Riga 11 */ puts("*   ~~ MACCHINA DEL TEMPO ~~           *");
    /* Riga 12 */ puts("*                                      *");
    /* Riga 13 */ puts("*   [1983] --> Internet --> [LLM]      *");
    /* Riga 14 */ puts("*                                      *");
    /* Riga 15 */ puts("*   Una vecchia tecnologia             *");
    /* Riga 16 */ puts("*   dialoga con l'AI del futuro!       *");
    /* Riga 17 */ puts("*                                      *");
    /* Riga 18 */ puts("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-");
    /* Riga 19 */ puts("");
    /* Riga 20 */ puts(" < Premi un tasto per iniziare... >");

    cgetc();       /* attendi keypress                 */
    clrscr();      /* pulisci prima della welcome page */
}

/* -----------------------------------------------
 * Schermata di benvenuto (dopo il banner)
 * ----------------------------------------------- */
static void show_welcome(void) {
    print_line();
    puts(" APPLE IIe CHATBOT");
    puts(" Powered by Viscoli Giuseppe");
    print_line();
    puts("");
    puts(" Digita la tua domanda e premi");
    puts(" INVIO. Digita Q per uscire.");
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

    if (!build_json(message, json_buf, JSON_BUF_SIZE)) {
        puts("[ERR] Messaggio troppo lungo!");
        return false;
    }

    result = do_post("/api/data",
                     json_buf,
                     response_buf,
                     RESPONSE_BUF_SIZE);

    if (result < 0) {
        puts("[ERR] Connessione al proxy fallita.");
        puts("Verifica che llm_proxy.py sia attivo.");
        return false;
    }

    puts("");
    print_line();
    puts("BOT:");
    print_line();

    if (http_response_body && http_response_length > 0) {
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
    uint8_t  turn = 0;

    /* ── Banner ASCII art: mostrato una sola volta ── */
    show_ascii_banner();

    /* ── Welcome screen ── */
    show_welcome();

    /* ── Inizializza rete ── */
    if (!initialize_network()) {
        puts("Premi un tasto...");
        cgetc();
        return 1;
    }

    /* ── Test connessione al proxy ── */
    puts("Test proxy...");
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

    /* ── Loop principale chat ── */
    for (;;) {
        printf("TU[%d]: ", (int)turn + 1);
        msg_len = read_line(input_buf, INPUT_MAX);

        if (msg_len == 0) {
            puts("[Messaggio vuoto, riprova]");
            continue;
        }
        if ((msg_len == 1) &&
            (input_buf[0] == 'Q' || input_buf[0] == 'q')) {
            break;
        }

        /* [BUG-6 nota] Per rispettare i vincoli di polling (~10 sec max),
         * si potrebbe aggiungere qui un indicatore visivo tipo:
         *   puts("Invio");  e poi '.' nel loop ip65_process.
         * Richiede di esporre il loop da rest_lib.c o di duplicarlo.
         */
        puts("Invio...");
        send_message(input_buf);

        turn++;
    }

    puts("\nArrivederci!");
    return 0;
}
