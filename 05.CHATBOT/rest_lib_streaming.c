/*
 * rest_lib_streaming.c — HTTP GET/POST verso il proxy con output streaming
 *
 * A differenza di rest_lib.c, NON accumula la risposta in un buffer.
 * I caratteri del body vengono stampati a schermo direttamente dal
 * callback TCP, man mano che arrivano dal server.
 *
 * Il loop di polling si ferma quando ip65 segnala la chiusura della
 * connessione (ip65_error != 0), oppure dopo STREAM_IDLE_MAX iterazioni
 * consecutive senza dati (safety valve).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* -----------------------------------------------
 * Configurazione
 * ----------------------------------------------- */
#define PROXY_HOST          "192.100.1.211"
#define PROXY_PORT          8080
#define HEADER_BUF_SIZE     512

/* Iterazioni max per attendere i primi header (fase 1) */
#define STREAM_INIT_WAIT    30000u

/* Iterazioni idle consecutive prima di considerare lo stream terminato (fase 2).
 * Su Apple IIe reale a 1 MHz, ~1ms per iterazione -> ~5 secondi di idle. */
#define STREAM_IDLE_MAX      5000u

/* -----------------------------------------------
 * Stato interno dello stream
 * ----------------------------------------------- */
static uint16_t  http_status    = 0;
static char      request_buf[512];
static char      s_header_buf[HEADER_BUF_SIZE];
static uint16_t  s_header_len   = 0;
static uint8_t   s_headers_done = 0;
static uint8_t   s_got_data     = 0;  /* 1 se il callback ha stampato dati nell'ultima iterazione */

/* -----------------------------------------------
 * print_ip65_error
 * ----------------------------------------------- */
static void print_ip65_error(void) {
    printf("ip65 errore %d: ", (int)ip65_error);
    switch (ip65_error) {
        case 0x80: puts("TIMEOUT ricezione");          break;
        case 0x81: puts("TRASMISSIONE fallita");       break;
        case 0x82: puts("DNS lookup fallito");         break;
        case 0x83: puts("CONNESSIONE fallita");        break;
        case 0x84: puts("Connessione chiusa");         break;
        case 0x85: puts("Out of memory");              break;
        case 0x86: puts("DHCP fallito");               break;
        default:   puts("errore sconosciuto");         break;
    }
}

/* -----------------------------------------------
 * Callback TCP — due fasi:
 *
 * Fase 1 (s_headers_done == 0):
 *   Accumula byte in s_header_buf fino a trovare \r\n\r\n.
 *   Quando trovato, parsa l'HTTP status e passa in fase 2.
 *
 * Fase 2 (s_headers_done == 1):
 *   Stampa ogni byte direttamente a schermo con putchar().
 *   Imposta s_got_data = 1 per segnalare attivita' al loop.
 * ----------------------------------------------- */
static void __fastcall__ tcp_stream_cb(const uint8_t *data, int len) {
    uint16_t i = 0;
    uint16_t body_start;

    if (len <= 0) return;

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
                    s_headers_done = 1;
                    i++;   /* prossimo byte e' gia' body */
                    break;
                }
            } else {
                /* header_buf pieno senza trovare fine-header: forza body mode */
                s_headers_done = 1;
                i++;
                break;
            }
        }
    }

    if (s_headers_done) {
        body_start = i;
        for (; i < (uint16_t)len; i++) {
            putchar((int)data[i]);
        }
        if (i > body_start) {
            s_got_data = 1;
        }
    }
}

/* -----------------------------------------------
 * tcp_do_stream — logica comune a GET e POST
 *
 * Fase 1: attende gli header HTTP (max STREAM_INIT_WAIT iterazioni).
 * Fase 2: stampa il body token per token fino a chiusura connessione
 *         o STREAM_IDLE_MAX iterazioni consecutive senza dati.
 *
 * Ritorna: 0 OK, -1 errore
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
    s_header_len   = 0;
    s_headers_done = 0;
    s_got_data     = 0;
    http_status    = 0;

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
        puts("\nERRORE: timeout in attesa della risposta");
        tcp_close();
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
        if (ip65_error != 0) break;  /* 0x84 = connessione chiusa = fine stream */
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
 *
 * L'output viene stampato direttamente a schermo.
 * Non esiste buffer di ritorno.
 *
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
 *
 * L'output viene stampato direttamente a schermo.
 * Non esiste buffer di ritorno.
 *
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
