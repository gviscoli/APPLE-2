#include <stdio.h>
#include <string.h>
#include "../00.LIBRERIE/IP65/inc/ip65.h"

// Compilazione
//
// cl65 -t apple2 rest_hello.c -o rest_hello.bin -O -m rest_hello.map -vm ../00.LIBRERIE/IP65/lib/ip65.lib ../00.LIBRERIE/IP65/lib/ip65_tcp.lib ../00.LIBRERIE/IP65/drivers/ip65_apple2.lib ../00.LIBRERIE/CC65/lib/apple2.lib
//
// java -jar C:\UTILITY\AppleCommander\AppleCommander-win32-x86_64-1.9.0.jar
// 
//

// Tentiamo di mappare le funzioni mancanti se il tuo header non le vede
// In alcune versioni tcp_is_connected è una macro o definita in tcp.h
extern uint8_t __fastcall__ tcp_get_state(void);
#define TCP_STATE_ESTABLISHED 4
#define TCP_STATE_CLOSED 0

void __fastcall__ on_tcp_receive(const unsigned char* data, int len) {
    int i;
    static uint8_t body_started = 0; 
    for (i = 0; i < len; ++i) {
        if (!body_started) {
            // Cerchiamo la fine degli header HTTP (\r\n\r\n)
            if (i > 3 && data[i-3] == '\r' && data[i-2] == '\n' && data[i-1] == '\r' && data[i] == '\n') {
                body_started = 1;
            }
        } else {
            putchar(data[i]);
        }
    }
}

void make_request(void) {
    uint8_t proxy_ip[4] = {192, 100, 1, 211}; 
    uint16_t proxy_port = 8080;
    char* http_req = "GET /api/data HTTP/1.1\r\nHost: proxy\r\nConnection: close\r\n\r\n";

    printf("Inizializzazione IP65...\n");

    /* CORREZIONE ip65_init: 
       Molte versioni richiedono un parametro. Se non hai caricato un driver 
       dinamicamente, prova a passare 0 (usa il driver linkato staticamente).
    */
    if (ip65_init(0) != 0) { 
        printf("Errore Driver!\n");
        return;
    }

    // Configurazione IP dell'Apple II (necessaria se non usi DHCP)
    // ip65_set_ip(my_ip); // Opzionale se il driver è già configurato

    printf("Connessione in corso...\n");
    // Proviamo la firma a 4 argomenti (IP, Porto Remoto, Porto Locale, Callback)
    if (tcp_connect(proxy_ip, proxy_port, on_tcp_receive) == 0) {
        printf("Errore connessione!\n");
        return;
    }

    tcp_send((unsigned char*)http_req, strlen(http_req));

    // Invece di tcp_get_state, usiamo tcp_is_connected se l'header lo permette
    // Oppure, più semplicemente, un timeout o il controllo del driver
    printf("Ricezione in corso...\n");
    
    // Versione "Brute Force": processa lo stack per un po' 
    // finché non ricevi i dati (per uno stub Hello World basta un attimo)
    {
        uint16_t timeout = 5000; 
        while (timeout--) {
            ip65_process();
        }
    }
    
    printf("\n---\nFine.");
}

int main(void) {
    make_request();
    return 0;
}