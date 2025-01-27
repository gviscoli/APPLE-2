#include <apple2.h>
#include <stdio.h>

#include "../00.LIBRERIE/IP65/inc/ip65.h"

// Compilazione
//
// cl65 -t apple2 pch-uthernet-scan.c -o pch-uthernet-scan.bin -O  -m pch-uthernet-scan.map -vm ../00.LIBRERIE/IP65/lib/ip65.lib ../00.LIBRERIE/IP65/drivers/ip65_apple2.lib
//
//
// 
//

// Funzione per verificare se una Uthernet II è nello slot specificato
int is_uthernet2_installed(int slot) {
    unsigned char *base_address = (unsigned char *)(0xc080 | (slot << 4));

    // Esegui una lettura di prova dal registro
    unsigned char value = *base_address;

    printf("risposta lettura:  %c\n", value);
    
    // Se la scheda è presente, dovrebbe rispondere in modo prevedibile
    // (questa logica dipende dalla documentazione della Uthernet II)
    return (value == 0x55 || value == 0xAA); // Sostituisci con la condizione appropriata
}

int main(void) {
    int slot;         // Dichiarazione della variabile "slot"
    int slot_found = -1;

    printf("Scanning slots for Uthernet II...\n");

    for (slot = 0; slot < 8; slot++) {
        if (is_uthernet2_installed(slot)) {
            slot_found = slot;
            printf("Uthernet II found in slot %d\n", slot);
            break;
        }
    }

    if (slot_found == -1) {
        printf("No Uthernet II card detected.\n");
    }

    return 0;
}