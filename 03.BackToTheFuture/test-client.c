

// Compilazione
//
// cl65 -t apple2 test-client.c stream3.c -o test-client.bin -O  -m test-client.map -vm ../00.LIBRERIE/IP65/lib/ip65.lib ../00.LIBRERIE/IP65/drivers/ip65_apple2.lib
//
// cl65 -t apple2 test-client.c net-utility.s -o test-client.bin -O  -m test-client.map -vm ../00.LIBRERIE/IP65/lib/ip65.lib ../00.LIBRERIE/IP65/drivers/ip65_apple2.lib
//
// ca65 net-utility.s -o net-utility.o
// cc65 -O -t apple2 test-client.c --add-source
// ca65 test-client.s -o test-client.o
// ld65 -t apple2 -o test-client.bin test-client.o net-utility.o ../00.LIBRERIE/IP65/lib/ip65.lib ../00.LIBRERIE/IP65/drivers/ip65_apple2.lib
//
// java -jar C:\UTILITY\AppleCommander\AppleCommander-win32-x86_64-1.9.0.jar
// 
//

#include <stdio.h>
#include "../00.LIBRERIE/IP65/inc/ip65.h"


// Dichiarazione della funzione Assembly
extern void init_ip_via_dhcp(void);

// Funzione per stampare l'indirizzo IP attuale
void print_ip_address(void) {
    extern unsigned char cfg_ip[4];  // Indirizzo IP assegnato dal DHCP

    printf("Indirizzo IP assegnato: %d.%d.%d.%d\n",
           cfg_ip[0], cfg_ip[1], cfg_ip[2], cfg_ip[3]);
}

int main(void) {
    printf("Inizializzazione della rete via DHCP...\n");
    init_ip_via_dhcp();  // Chiama la funzione Assembly
    printf("Operazione completata.\n");

    print_ip_address();  // Stampa l'IP assegnato

    // printf("Inizializzazione della rete via DHCP...\n");

    // if (init_ip_via_dhcp() == 0) {
    //     printf("Rete configurata con successo!\n");
    //     print_ip_address();  // Stampa l'IP assegnato
    // } else {
    //     printf("Errore nella configurazione della rete.\n");
    // }

    return 0;
}