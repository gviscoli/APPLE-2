/*
 * rest_lib_streaming.c — HTTP GET/POST verso il proxy con output streaming
 *
 * I caratteri del body vengono stampati a schermo direttamente dal
 * callback TCP, man mano che arrivano dal server.
 *
 * Supporta sia risposte HTTP/1.0 (connessione chiusa a fine stream)
 * sia HTTP/1.1 con Transfer-Encoding: chunked.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* -----------------------------------------------
 * Configurazione
 * ----------------------------------------------- */
#define PROXY_HOST          "192.100.1.211"
#define PROXY_PORT          8081
#define HEADER_BUF_SIZE     512

/* Iterazioni max per attendere i primi header (fase 1) */
#define STREAM_INIT_WAIT    30000u

/* Iterazioni idle consecutive prima di considerare lo stream terminato.
 * Con HTTP/1.1 chunked TE, tra un token LLM e l'altro possono passare
 * anche 1-2 secondi. Su Apple IIe reale a 1 MHz, ip65_process() costa
 * circa 500 cicli => ~0.5ms/iterazione => 20000 iter ~ 10 secondi di idle.
 * Con HTTP/1.1 la connessione chiude dopo il chunk terminale (0\r\n\r\n):
 * questo valore e' solo un safety valve contro hang infiniti. */
#define STREAM_IDLE_MAX     20000u

/* -----------------------------------------------
 * Stati del parser Chunked Transfer Encoding
 *
 * Formato di ogni chunk:
 *   <hex-size>\r\n
 *   <data bytes>\r\n
 * Chunk terminale:
 *   0\r\n
 *   \r\n
 * ----------------------------------------------- */
#define CK_SIZE      0    /* lettura cifre hex della dimensione */
#define CK_SIZE_LF   1    /* attesa \n dopo \r della riga size  */
#define CK_DATA      2    /* lettura e stampa dei byte del chunk */
#define CK_TRAIL_CR  3    /* skip \r dopo i dati del chunk       */
#define CK_TRAIL_LF  4    /* skip \n dopo i dati del chunk       */
#define CK_DONE      5    /* chunk terminale (size 0) ricevuto   */

/* -----------------------------------------------
 * Stato interno dello stream
 * ----------------------------------------------- */
static uint16_t http_status    = 0;
static char     request_buf[512];
static char     s_header_buf[HEADER_BUF_SIZE];
static uint16_t s_header_len   = 0;
static uint8_t  s_headers_done = 0;
static uint8_t  s_got_data     = 0;

/* Chunked TE */
static uint8_t  s_chunked       = 0;   /* 1 se il server usa chunked TE     */
static uint8_t  s_ck_state      = CK_SIZE;
static uint16_t s_ck_remain     = 0;   /* byte rimasti nel chunk corrente   */
static char     s_ck_szline[8];        /* accumula le cifre hex della size  */
static uint8_t  s_ck_szlen      = 0;

/* Flag "almeno un byte di body e' stato stampato".
 * Serve a distinguere un timeout reale (server assente) da un falso
 * timeout in cui la risposta e' arrivata tardi (ip65_error stale o
 * latenza alta dell'LLM) ma e' stata drenata da tcp_close(). */
static uint8_t  s_body_received = 0;

/* -----------------------------------------------
 * print_ip65_error
 * ----------------------------------------------- */
static void print_ip65_error(void) {
    printf("ip65 errore %d: ", (int)ip65_error);
    switch (ip65_error) {
        case 0x80: puts("TIMEOUT ricezione");     break;
        case 0x81: puts("TRASMISSIONE fallita");  break;
        case 0x82: puts("DNS lookup fallito");    break;
        case 0x83: puts("CONNESSIONE fallita");   break;
        case 0x84: puts("Connessione chiusa");    break;
        case 0x85: puts("Out of memory");         break;
        case 0x86: puts("DHCP fallito");          break;
        default:   puts("errore sconosciuto");    break;
    }
}

/* -----------------------------------------------
 * parse_chunk_hex — converte s_ck_szline in uint16_t
 * ----------------------------------------------- */
static uint16_t parse_chunk_hex(void) {
    uint16_t val = 0;
    uint8_t  j;
    char     c;

    for (j = 0; j < s_ck_szlen; j++) {
        c = s_ck_szline[j];
        val <<= 4;
        if      (c >= '0' && c <= '9') val |= (uint16_t)(c - '0');
        else if (c >= 'a' && c <= 'f') val |= (uint16_t)(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') val |= (uint16_t)(c - 'A' + 10);
    }
    return val;
}

/* -----------------------------------------------
 * Callback TCP — due fasi:
 *
 * Fase 1 (s_headers_done == 0):
 *   Accumula byte in s_header_buf fino a trovare \r\n\r\n.
 *   Parsa HTTP status e rileva Transfer-Encoding: chunked.
 *
 * Fase 2 (s_headers_done == 1):
 *   - Se NON chunked: stampa direttamente ogni byte.
 *   - Se chunked: parser della macchina a stati CK_*.
 * ----------------------------------------------- */
static void __fastcall__ tcp_stream_cb(const uint8_t *data, int len) {
    uint16_t i          = 0;
    uint16_t body_start = 0;
    uint8_t  b;

    if (len <= 0) return;

    /* ---- Fase 1: raccolta header ---- */
    if (!s_headers_done) {
        for (; i < (uint16_t)len; i++) {
            if (s_header_len < HEADER_BUF_SIZE - 1) {
                s_header_buf[s_header_len++] = (char)data[i];

                if (s_header_len >= 4 &&
                    s_header_buf[s_header_len - 4] == '\r' &&
                    s_header_buf[s_header_len - 3] == '\n' &&
                    s_header_buf[s_header_len - 2] == '\r' &&
                    s_header_buf[s_header_len - 1] == '\n') {

                    s_header_buf[s_header_len] = '\0';
                    sscanf(s_header_buf, "HTTP/1.%*d %hu", &http_status);

                    /* Rileva Transfer-Encoding: chunked */
                    if (strstr(s_header_buf, "chunked") != NULL) {
                        s_chunked   = 1;
                        s_ck_state  = CK_SIZE;
                        s_ck_szlen  = 0;
                        s_ck_remain = 0;
                    }

                    s_headers_done = 1;
                    i++;
                    break;
                }
            } else {
                /* header_buf pieno: forza body mode senza chunked */
                s_headers_done = 1;
                i++;
                break;
            }
        }
    }

    /* ---- Fase 2: output body ---- */
    if (!s_headers_done) return;

    if (!s_chunked) {
        /* HTTP/1.0 o risposta senza chunked: stampa tutto direttamente */
        body_start = i;
        for (; i < (uint16_t)len; i++) {
            putchar((int)data[i]);
        }
        if (i > body_start) {
            s_got_data      = 1;
            s_body_received = 1;
        }

    } else {
        /* Parser Chunked Transfer Encoding */
        for (; i < (uint16_t)len; i++) {
            b = data[i];

            switch (s_ck_state) {

                case CK_SIZE:
                    if (b == '\r') {
                        s_ck_remain = parse_chunk_hex();
                        s_ck_szlen  = 0;
                        s_ck_state  = CK_SIZE_LF;
                    } else if (b != '\n') {
                        /* accumula cifra hex (ignora eventuale \n residuo) */
                        if (s_ck_szlen < 7)
                            s_ck_szline[s_ck_szlen++] = (char)b;
                    }
                    break;

                case CK_SIZE_LF:
                    /* salta il \n dopo la riga di size */
                    s_ck_state = (s_ck_remain == 0) ? CK_DONE : CK_DATA;
                    break;

                case CK_DATA:
                    putchar((int)b);
                    s_got_data      = 1;
                    s_body_received = 1;
                    s_ck_remain--;
                    if (s_ck_remain == 0) s_ck_state = CK_TRAIL_CR;
                    break;

                case CK_TRAIL_CR:
                    /* salta il \r dopo i dati */
                    s_ck_state = CK_TRAIL_LF;
                    break;

                case CK_TRAIL_LF:
                    /* salta il \n dopo i dati, torna a leggere il prossimo size */
                    s_ck_state = CK_SIZE;
                    break;

                case CK_DONE:
                    /* chunk terminale ricevuto: ignora il resto */
                    goto done_chunked;
            }
        }
        done_chunked:;
    }
}

/* -----------------------------------------------
 * tcp_do_stream — logica comune a GET e POST
 * ----------------------------------------------- */
static int8_t tcp_do_stream(uint16_t request_len) {
    uint32_t server_ip;
    uint16_t poll_count;
    uint16_t idle_count;
    char     host_buf[16];

    strncpy(host_buf, PROXY_HOST, sizeof(host_buf) - 1);
    host_buf[sizeof(host_buf) - 1] = '\0';
    server_ip = parse_dotted_quad(host_buf);
    if (server_ip == 0) {
        puts("ERRORE: IP proxy non valido");
        return -1;
    }

    /* Reset stato */
    s_header_len    = 0;
    s_headers_done  = 0;
    s_got_data      = 0;
    s_body_received = 0;
    http_status     = 0;
    s_chunked       = 0;
    s_ck_state      = CK_SIZE;
    s_ck_szlen      = 0;
    s_ck_remain     = 0;

    if (tcp_connect(server_ip, PROXY_PORT, tcp_stream_cb) != 0) {
        puts("ERRORE: TCP connect fallita");
        print_ip65_error();
        return -1;
    }

    if (tcp_send((const uint8_t *)request_buf, request_len) != 0) {
        puts("ERRORE: TCP send fallita");
        print_ip65_error();
        tcp_close();
        return -1;
    }

    /* --- Fase 1: attendi header HTTP --- */
    poll_count = 0;
    while (!s_headers_done && poll_count < STREAM_INIT_WAIT) {
        ip65_process();
        poll_count++;
        if (ip65_error != 0) break;
    }

    if (!s_headers_done) {
        /* Prima dreniamo il buffer TCP: tcp_close() chiama il callback
         * con eventuali dati gia' arrivati ma non ancora consegnati.
         * Solo DOPO decidiamo se e' un errore reale. */
        tcp_close();
        if (s_body_received) {
            /* Risposta arrivata tardi (ip65_error stale o latenza alta):
             * i dati sono stati stampati durante il drain, non e' un errore. */
            return 0;
        }
        puts("ERRORE: server non raggiungibile o timeout.");
        return -1;
    }

    if (http_status != 0 && (http_status < 200 || http_status >= 300)) {
        printf("\nERRORE: HTTP status %d\n", http_status);
        tcp_close();
        return -1;
    }

    /* --- Fase 2: stream body fino a chiusura connessione --- */
    idle_count = 0;
    while (idle_count < STREAM_IDLE_MAX) {
        s_got_data = 0;
        ip65_process();

        /* Connessione chiusa dal server (HTTP/1.0 = fine stream normale) */
        if (ip65_error != 0) break;

        /* Chunk terminale ricevuto (HTTP/1.1 chunked) */
        if (s_chunked && s_ck_state == CK_DONE) break;

        if (s_got_data) {
            idle_count = 0;
        } else {
            idle_count++;
        }
    }

    tcp_close();
    return 0;
}

/* -----------------------------------------------
 * do_get_stream — HTTP GET con output streaming
 * Ritorna: 0 OK, -1 errore
 * ----------------------------------------------- */
static int8_t do_get_stream(const char *path) {
    uint16_t request_len;

    http_status = 0;
    request_len = (uint16_t)snprintf(
        request_buf, sizeof(request_buf),
        "GET %s HTTP/1.0\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n"
        "\r\n",
        path,
        PROXY_HOST);

    return tcp_do_stream(request_len);
}

/* -----------------------------------------------
 * do_post_stream — HTTP POST con output streaming
 * Ritorna: 0 OK, -1 errore
 * ----------------------------------------------- */
static int8_t do_post_stream(const char *path,
                              const char *json_body) {
    uint16_t body_len;
    uint16_t request_len;
    uint16_t needed;

    http_status = 0;
    body_len    = (uint16_t)strlen(json_body);
    needed      = 80u
                + (uint16_t)strlen(path)
                + (uint16_t)strlen(PROXY_HOST)
                + body_len;

    if (needed >= (uint16_t)sizeof(request_buf)) {
        printf("ERRORE: request_buf troppo piccolo "
               "(%d needed, %d available)\n",
               (int)needed, (int)sizeof(request_buf));
        return -1;
    }

    request_len = (uint16_t)snprintf(
        request_buf, sizeof(request_buf),
        "POST %s HTTP/1.0\r\n"
        "Host: %s\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        path,
        PROXY_HOST,
        (int)body_len,
        json_body);

    return tcp_do_stream(request_len);
}
