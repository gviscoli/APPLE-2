#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ip65: contiene tcp_connect, tcp_send, tcp_close,
 * parse_dotted_quad, ip65_process, ip65_error, dhcp_init */
//#include "../00.LIBRERIE/IP65/inc/ip65.h"

/* -----------------------------------------------
 * Configurazione
 * ----------------------------------------------- */
#define PROXY_HOST        "192.100.1.211"
#define PROXY_PORT        8080
#define RESPONSE_BUF_SIZE 2048
#define POLL_MAX_ITERATIONS 10000u

/* -----------------------------------------------
 * Variabili HTTP gestite interamente da noi.
 * NON sono extern verso ip65 (ip65 non le ha):
 * le dichiariamo statiche in questo file e le
 * esponiamo come puntatori/valori al chiamante.
 * ----------------------------------------------- */
static uint16_t  http_status          = 0;
static uint16_t  http_response_length = 0;
static uint8_t  *http_response_body   = NULL;

/* -----------------------------------------------
 * Buffer globali (heap limitato su 6502!)
 * ----------------------------------------------- */
static uint8_t  response_buf[RESPONSE_BUF_SIZE];
static char     request_buf[512];

/* -----------------------------------------------
 * Stato interno condiviso tra GET e POST
 * per il callback TCP di ip65
 * ----------------------------------------------- */
static uint8_t  *s_recv_ptr       = NULL;
static uint16_t  s_recv_remaining = 0;
static uint16_t  s_recv_total     = 0;

/* -----------------------------------------------
 * print_ip65_error — ip65 non la fornisce
 * ----------------------------------------------- */
static void print_ip65_error(void) {
    printf("ip65 errore %d: ", (int)ip65_error);
    switch (ip65_error) {
        case 0x80: puts("TIMEOUT ricezione");          break;
        case 0x81: puts("TRASMISSIONE fallita");       break;
        case 0x82: puts("DNS lookup fallito");         break;
        case 0x83: puts("CONNESSIONE fallita");        break;
        case 0x84: puts("Connessione chiusa (reset)"); break;
        case 0x85: puts("Out of memory");              break;
        case 0x86: puts("DHCP fallito");               break;
        default:   puts("errore sconosciuto");         break;
    }
}

/* -----------------------------------------------
 * Callback TCP
 * Firma ESATTA richiesta da ip65 (__fastcall__).
 * Chiamata da ip65_process() quando arrivano dati.
 * Usata sia da do_get che da do_post.
 * ----------------------------------------------- */
static void __fastcall__ tcp_recv_cb(const uint8_t *data, int len) {
    uint16_t copy_len;

    if (len <= 0 || s_recv_ptr == NULL || s_recv_remaining == 0) {
        return;
    }
    copy_len = (uint16_t)len;
    if (copy_len > s_recv_remaining) {
        copy_len = s_recv_remaining;
    }
    memcpy(s_recv_ptr, data, copy_len);
    s_recv_ptr       += copy_len;
    s_recv_remaining -= copy_len;
    s_recv_total     += copy_len;
}

/* -----------------------------------------------
 * Funzione interna comune a GET e POST:
 * - connette via TCP al proxy
 * - invia la request gia' costruita in request_buf
 * - fa polling ip65_process() fino a ricezione completa
 * - parsa status HTTP e individua il body
 * - riempie http_status, http_response_length, http_response_body
 *
 * Parametri:
 *   request_len : lunghezza della stringa in request_buf
 *   out_buf     : buffer dove accumulare la risposta raw
 *   out_size    : dimensione del buffer
 * Ritorna: http_response_length, oppure -1 su errore
 * ----------------------------------------------- */
static int16_t tcp_do_request(uint16_t  request_len,
                               uint8_t  *out_buf,
                               uint16_t  out_size) {
    uint32_t  server_ip;
    uint16_t  poll_count;
    uint8_t  *body_start;
    char      host_buf[16];

    /* parse_dotted_quad vuole char*, non const char* */
    strncpy(host_buf, PROXY_HOST, sizeof(host_buf) - 1);
    host_buf[sizeof(host_buf) - 1] = '\0';
    server_ip = parse_dotted_quad(host_buf);
    if (server_ip == 0) {
        puts("ERRORE: IP proxy non valido");
        return -1;
    }

    /* Inizializza stato callback */
    s_recv_ptr       = out_buf;
    s_recv_remaining = out_size - 1u;
    s_recv_total     = 0;

    /* Connessione TCP — terzo parametro e' il callback */
    if (tcp_connect(server_ip, PROXY_PORT, tcp_recv_cb) != 0) {
        puts("ERRORE: TCP connect fallita");
        print_ip65_error();
        return -1;
    }
    puts("TCP connect OK");

    /* Invia la request (gia' in request_buf) */
    if (tcp_send((const uint8_t *)request_buf, request_len) != 0) {
        puts("ERRORE: TCP send fallita");
        print_ip65_error();
        tcp_close();
        return -1;
    }
    puts("TCP send OK"); 
    /* Polling fino a: buffer pieno | timeout | errore ip65 */
    poll_count = 0;
    while (s_recv_remaining > 0 &&
           poll_count < POLL_MAX_ITERATIONS) {
        ip65_process();
        poll_count++;
        if (ip65_error != 0) break;
    }

    out_buf[s_recv_total] = '\0';
    tcp_close();

    if (s_recv_total == 0) {
        puts("ERRORE: nessun dato ricevuto");
        return -1;
    }

    /* Parsing HTTP status */
    if (sscanf((char *)out_buf, "HTTP/1.%*d %hu", &http_status) != 1) {
        puts("ATTENZIONE: impossibile parsare HTTP status");
        http_status = 0;
    }

    /* Individua inizio body dopo \r\n\r\n */
    body_start = (uint8_t *)strstr((char *)out_buf, "\r\n\r\n");
    if (!body_start) {
        puts("ATTENZIONE: risposta HTTP malformata (no CRLFCRLF)");
        printf("Raw (%d bytes):\n%s\n", s_recv_total, (char *)out_buf);
        return -1;
    }

    body_start          += 4;
    http_response_body   = body_start;
    http_response_length = (uint16_t)(
        s_recv_total - (uint16_t)(body_start - out_buf));

    printf("Status HTTP  : %d\n", http_status);
    printf("Body (%d bytes):\n%s\n",
           http_response_length, (char *)http_response_body);

    if (http_status < 200 || http_status >= 300) {
        printf("ERRORE: status %d\n", http_status);
        return -1;
    }

    return (int16_t)http_response_length;
}

/* -----------------------------------------------
 * HTTP GET verso il proxy
 * Parametri:
 *   path     : es. "/api/v1/status"
 *   out_buf  : buffer risposta
 *   out_size : dimensione buffer
 * Ritorna: lunghezza body, -1 su errore
 * ----------------------------------------------- */
static int16_t do_get(const char *path,
                      uint8_t    *out_buf,
                      uint16_t    out_size) {
    uint16_t request_len;

    http_status          = 0;
    http_response_length = 0;
    http_response_body   = NULL;

    printf("\nGET http://%s:%d%s\n", PROXY_HOST, PROXY_PORT, path);
    puts("Attendo risposta...");

    request_len = (uint16_t)snprintf(
        request_buf, sizeof(request_buf),
        "GET %s HTTP/1.0\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n"
        "\r\n",
        path,
        PROXY_HOST);

    return tcp_do_request(request_len, out_buf, out_size);
}

/* -----------------------------------------------
 * HTTP POST verso il proxy con body JSON
 * Parametri:
 *   path      : es. "/api/v1/data"
 *   json_body : stringa JSON da inviare
 *   out_buf   : buffer risposta
 *   out_size  : dimensione buffer
 * Ritorna: lunghezza body risposta, -1 su errore
 * ----------------------------------------------- */
static int16_t do_post(const char *path,
                       const char *json_body,
                       uint8_t    *out_buf,
                       uint16_t    out_size) {
    uint16_t body_len;
    uint16_t request_len;
    uint16_t needed;

    http_status          = 0;
    http_response_length = 0;
    http_response_body   = NULL;

    body_len = (uint16_t)strlen(json_body);

    /* Verifica capienza request_buf prima di scrivere */
    needed = 80u
           + (uint16_t)strlen(path)
           + (uint16_t)strlen(PROXY_HOST)
           + body_len;
    if (needed >= (uint16_t)sizeof(request_buf)) {
        printf("ERRORE: request_buf troppo piccolo "
               "(%d needed, %d available)\n",
               (int)needed, (int)sizeof(request_buf));
        return -1;
    }

    printf("\nPOST http://%s:%d%s\n", PROXY_HOST, PROXY_PORT, path);
    printf("Body (%d bytes): %s\n", body_len, json_body);

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

    return tcp_do_request(request_len, out_buf, out_size);
}