#include <stdio.h>

#include "../00.LIBRERIE/CC65/inc/cc65.h"
#include "../00.LIBRERIE/IP65/inc/ip65.h"

// Compilazione
//
// cl65 -t apple2 slotscan.c -o slotscan.bin -O  -m slotscan.map -vm ../00.LIBRERIE/IP65/lib/ip65.lib ../00.LIBRERIE/IP65/lib/ip65_tcp.lib ../00.LIBRERIE/IP65/drivers/ip65_apple2.lib ../00.LIBRERIE/CC65/lib/apple2.lib
//

void main() {
    unsigned char slot;

    slot = A2_SLOT_SCAN();  // Scansiona gli slot per trovare una scheda di rete
    if (slot == 0) {
        printf("Nessuna scheda di rete trovata!\n");
        return;
    }

    printf("Scheda di rete trovata nello slot %d\n", slot);

    // Inizializza IP65 con lo slot rilevato
    if (ip65_init(slot) != 0) {
        printf("Errore durante l'inizializzazione della rete.\n");
        return;
    }

    printf("Rete inizializzata con successo!\n");
}
