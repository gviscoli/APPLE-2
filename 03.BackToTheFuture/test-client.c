

// Compilazione
//
// cl65 -t apple2 test-client.c -o test-client.bin -O  -m test-client.map -vm ../00.LIBRERIE/IP65/lib/ip65.lib ../00.LIBRERIE/IP65/lib/ip65_tcp.lib ../00.LIBRERIE/IP65/drivers/ip65_apple2.lib ../00.LIBRERIE/CC65/lib/apple2.lib
//
// java -jar C:\UTILITY\AppleCommander\AppleCommander-win32-x86_64-1.9.0.jar
// 
//

#include <stdio.h>
#include <string.h>

#include "../00.LIBRERIE/IP65/inc/ip65.h"
#include "../00.LIBRERIE/PCH/pch_network.h"


#define SERVER "google.com"  // Cambia con il dominio del server
#define PORT   80             // Porta HTTP
#define BUFFER_SIZE 512

unsigned char buffer[BUFFER_SIZE];  // Buffer per i dati ricevuti

void main() 
{
    uint32_t server_ip;
    unsigned char ip_bytes[4];
    char http_request[] = "GET / HTTP/1.1\r\nHost: example.com\r\nConnection: close\r\n\r\n";
    int received;

    ip65_init(0);  // Inizializza la rete

    printf("Avvio DHCP...\n");
    if (dhcp_init() != 0) {
        printf("Errore DHCP!\n");
        return;
    }

    printf("Risoluzione DNS per %s...\n", SERVER);
    
    if (dns_resolve(SERVER) != 0) {
        printf("Errore DNS!\n");
        return;
    }

    // unsigned char ip_bytes[4];
    // memcpy(ip_bytes, &server_ip, 4);
    // printf("Indirizzo IP: %d.%d.%d.%d\n", ip_bytes[0], ip_bytes[1], ip_bytes[2], ip_bytes[3]);

    // printf("Connessione a %s...\n", SERVER);
    // if (ip65_tcp_connect(server_ip, PORT) != 0) {
    //     printf("Errore di connessione!\n");
    //     return;
    // }

    // printf("Invio richiesta HTTP...\n");
    // char http_request[] = "GET / HTTP/1.1\r\nHost: example.com\r\nConnection: close\r\n\r\n";
    // if (ip65_tcp_send((unsigned char*)http_request, strlen(http_request)) != 0) {
    //     printf("Errore invio dati!\n");
    //     return;
    // }

    // printf("Ricezione dati...\n");
    // int received;
    // while ((received = ip65_tcp_recv(buffer, BUFFER_SIZE)) > 0) {
    //     buffer[received] = '\0';  // Assicura che il buffer sia una stringa valida
    //     printf("%s", buffer);     // Stampa la risposta HTTP
    // }

    printf("\nChiusura connessione.\n");
    tcp_close();
}