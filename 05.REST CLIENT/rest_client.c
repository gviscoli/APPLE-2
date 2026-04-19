/*
 * REST Client per Apple IIe
 * Compilare con cc65 + ip65-2022-05-30
 * Target: apple2enh (Apple IIe Enhanced)
 * Hardware: Uthernet II in slot 3
 *
 * Build:
 *   cl65 -t apple2enh -O -o rest_client.po rest_client.c -L /path/to/ip65/lib -lip65-apple2enh
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ip65 headers */
#include "../00.LIBRERIE/IP65/inc/ip65.h"


/* -----------------------------------------------
 * Configurazione
 * ----------------------------------------------- */
#define UTHERNET_SLOT     3          /* Slot scheda Uthernet II     */
#define PROXY_HOST        "192.100.1.211"  /* IP del tuo proxy locale     */
#define PROXY_PORT        8080              /* Porta HTTP del proxy        */
#define RESPONSE_BUF_SIZE 2048             /* Buffer risposta             */

/* Timeout in tick (~1 tick = 1/60s su Apple IIe) */
#define TCP_TIMEOUT       600              /* 10 secondi                  */

/* -----------------------------------------------
 * Buffer globali (heap limitato su 6502!)
 * ----------------------------------------------- */
static uint8_t  response_buf[RESPONSE_BUF_SIZE];
static char     request_buf[512];
static char     url_buf[128];

/* -----------------------------------------------
 * Utility: stampa errore ip65
 * ----------------------------------------------- */
static void print_ip65_error(void) {
    printf("ip65 errore: ");
    switch (ip65_error) {
        case IP65_ERROR_TIMEOUT_ON_RECEIVE:
            puts("timeout ricezione"); break;
        case IP65_ERROR_TRANSMIT_FAILED:
            puts("trasmissione fallita"); break;
        case IP65_ERROR_DNS_LOOKUP_FAILED:
            puts("DNS lookup fallito"); break;
        case IP65_ERROR_CONNECTION_FAILED:
            puts("connessione fallita"); break;
        default:
            printf("codice %d\n", ip65_error); break;
    }
}

/* -----------------------------------------------
 * Inizializzazione stack TCP/IP
 * ----------------------------------------------- */
static uint8_t network_init(void) {
    uint8_t mac[6];

    printf("Init ip65 (slot %d)...\n", UTHERNET_SLOT);

    /* ip65_init: 0 = successo */
    if (ip65_init(UTHERNET_SLOT) != 0) {
        puts("ERRORE: ip65_init fallito");
        puts("Verificare Uthernet II nello slot corretto");
        return 0;
    }

    /* Ottieni indirizzo MAC per debug */
    eth_get_mac(mac);
    printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    /* DHCP */
    puts("DHCP in corso...");
    if (dhcp_init() != 0) {
        puts("ERRORE: DHCP fallito");
        puts("Tentativo con IP statico non implementato");
        return 0;
    }

    /* Stampa configurazione ottenuta */
    printf("IP    : %s\n", dotted_quad(my_ip));
    printf("Mask  : %s\n", dotted_quad(my_mask));
    printf("GW    : %s\n", dotted_quad(my_gateway));
    printf("DNS   : %s\n", dotted_quad(dns_server));

    return 1;
}

/* -----------------------------------------------
 * HTTP GET verso il proxy
 * Parametri:
 *   path     : percorso API es. "/api/v1/data"
 *   out_buf  : buffer risposta
 *   out_size : dimensione buffer
 * Ritorna: lunghezza body, -1 su errore
 * ----------------------------------------------- */
static int16_t do_get(const char *path,
                      uint8_t *out_buf,
                      uint16_t out_size) {
    /* Costruisce URL http://PROXY_HOST:PROXY_PORT/path */
    snprintf(url_buf, sizeof(url_buf),
             "http://%s:%d%s",
             PROXY_HOST, PROXY_PORT, path);

    printf("\nGET %s\n", url_buf);
    puts("Attendo risposta...");

    /* http_get di ip65:
     *   - risolve hostname (qui IP diretto, niente DNS)
     *   - apre connessione TCP
     *   - invia richiesta HTTP/1.0
     *   - legge risposta nel buffer
     * Ritorna 0 su successo */
    if (http_get(url_buf, out_buf, out_size) != 0) {
        print_ip65_error();
        return -1;
    }

    /* http_response_body punta all'inizio del body */
    /* http_response_length contiene la lunghezza    */
    printf("Status HTTP: %d\n", http_status);
    printf("Body (%d bytes):\n", http_response_length);

    return (int16_t)http_response_length;
}

/* -----------------------------------------------
 * HTTP POST verso il proxy con body JSON
 * Parametri:
 *   path      : percorso API es. "/api/v1/data"
 *   json_body : stringa JSON da inviare
 *   out_buf   : buffer risposta
 *   out_size  : dimensione buffer
 * Ritorna: lunghezza body risposta, -1 su errore
 *
 * NOTA: ip65 non ha http_post nativo — usiamo
 *       tcp_* direttamente per costruire la request
 * ----------------------------------------------- */
static int16_t do_post(const char *path,
                       const char *json_body,
                       uint8_t *out_buf,
                       uint16_t out_size) {
    uint32_t server_ip;
    uint16_t body_len;
    uint16_t recv_len;
    uint16_t total_recv = 0;
    uint8_t  *p;

    body_len = (uint16_t)strlen(json_body);

    /* Risolvi IP del proxy (stringa dotted quad -> uint32) */
    server_ip = parse_dotted_quad(PROXY_HOST);
    if (server_ip == 0) {
        puts("ERRORE: IP proxy non valido");
        return -1;
    }

    printf("\nPOST http://%s:%d%s\n", PROXY_HOST, PROXY_PORT, path);
    printf("Body (%d bytes): %s\n", body_len, json_body);

    /* Connessione TCP al proxy */
    if (tcp_connect(server_ip, PROXY_PORT, TCP_TIMEOUT) != 0) {
        puts("ERRORE: TCP connect fallita");
        print_ip65_error();
        return -1;
    }

    /* Costruisce header HTTP/1.0 POST */
    snprintf(request_buf, sizeof(request_buf),
             "POST %s HTTP/1.0\r\n"
             "Host: %s\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             path,
             PROXY_HOST,
             body_len,
             json_body);

    /* Invia request */
    if (tcp_send((uint8_t *)request_buf,
                 (uint16_t)strlen(request_buf)) != 0) {
        puts("ERRORE: TCP send fallita");
        tcp_close();
        return -1;
    }

    /* Ricevi risposta in loop */
    p = out_buf;
    while (total_recv < out_size - 1) {
        recv_len = tcp_receive(p,
                               out_size - 1 - total_recv,
                               TCP_TIMEOUT);
        if (recv_len == 0) break;   /* connessione chiusa */
        if (recv_len == 0xFFFF) {   /* errore o timeout   */
            print_ip65_error();
            break;
        }
        p          += recv_len;
        total_recv += recv_len;
    }
    out_buf[total_recv] = '\0';

    tcp_close();

    /* Trova e stampa il body (dopo \r\n\r\n) */
    p = (uint8_t *)strstr((char *)out_buf, "\r\n\r\n");
    if (p) {
        p += 4;
        printf("Risposta body:\n%s\n", (char *)p);
    } else {
        printf("Risposta raw:\n%s\n", (char *)out_buf);
    }

    return (int16_t)total_recv;
}

/* -----------------------------------------------
 * Menu interattivo
 * ----------------------------------------------- */
static void show_menu(void) {
    puts("\n--- REST Client Apple IIe ---");
    puts("1. GET  /api/v1/status");
    puts("2. POST /api/v1/data  (JSON)");
    puts("3. GET  custom path");
    puts("Q. Esci");
    printf("Scelta: ");
}

/* -----------------------------------------------
 * Entry point
 * ----------------------------------------------- */
int main(void) {
    char choice;
    char custom_path[64];
    int16_t result;

    /* Inizializzazione */
    clrscr();
    puts("=== Apple IIe REST Client ===");
    puts("Tramite proxy TLS locale\n");

    if (!network_init()) {
        puts("Inizializzazione rete fallita. Premi un tasto.");
        cgetc();
        return 1;
    }

    /* Loop principale */
    for (;;) {
        show_menu();
        choice = cgetc();
        putchar('\n');

        switch (choice) {

            case '1':
                result = do_get("/api/v1/status",
                                response_buf,
                                RESPONSE_BUF_SIZE);
                if (result > 0) {
                    /* Stampa i primi 200 chars del body */
                    response_buf[200] = '\0';
                    printf("%s\n", (char *)http_response_body);
                }
                break;

            case '2':
                result = do_post(
                    "/api/v1/data",
                    "{\"device\":\"apple2e\","
                    "\"msg\":\"hello from 1983\","
                    "\"temp\":42}",
                    response_buf,
                    RESPONSE_BUF_SIZE);
                break;

            case '3':
                printf("Path (es. /api/v1/items): ");
                /* Input da tastiera Apple IIe */
                {
                    uint8_t i = 0;
                    char c;
                    while (i < sizeof(custom_path) - 1) {
                        c = cgetc();
                        if (c == '\r') break;
                        if (c == 0x08 && i > 0) { /* backspace */
                            i--;
                            puts("\b \b");
                        } else {
                            custom_path[i++] = c;
                            putchar(c);
                        }
                    }
                    custom_path[i] = '\0';
                    putchar('\n');
                }
                do_get(custom_path, response_buf, RESPONSE_BUF_SIZE);
                break;

            case 'Q':
            case 'q':
                puts("Arrivederci!");
                return 0;

            default:
                break;
        }

        puts("\nPremi un tasto per continuare...");
        cgetc();
    }
}