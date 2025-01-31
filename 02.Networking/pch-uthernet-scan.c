#include <apple2.h>
#include <stdio.h>
#include <stdbool.h>

#include "../00.LIBRERIE/IP65/inc/ip65.h"

// Compilazione
//
// cl65 -t apple2 pch-uthernet-scan.c -o pch-uthernet-scan.bin -O  -m pch-uthernet-scan.map -vm ../00.LIBRERIE/IP65/lib/ip65.lib ../00.LIBRERIE/IP65/drivers/ip65_apple2.lib
//
// java -jar C:\UTILITY\AppleCommander\AppleCommander-win32-x86_64-1.9.0.jar
// 
//


#include <peekpoke.h>

#define SLOT_BASE 0xC080  // Base address for slot 0
#define SLOT_COUNT 7      // Number of slots to check (0-7)

// Uthernet II specific identifier or memory location
#define UTHERNET_ID 0x1234  // Replace with actual identifier or memory location

int check_uthernet_slot(int slot) {
    // Example: Check a specific memory location in the slot for Uthernet II
    int address = SLOT_BASE + (slot << 4);  // Calculate slot address
    int value = PEEK(address);              // Read value from slot memory

    // Replace with actual check for Uthernet II
    if (value == UTHERNET_ID) {
        return 1;  // Uthernet II found in this slot
    }
    return 0;  // Uthernet II not found in this slot
}

int main() {
    int slot;  // Dichiarazione della variabile all'inizio della funzione

    printf("Verifica slot Apple IIe...\n");

    for (slot = 0; slot <= SLOT_COUNT; slot++) {
        printf("Controllo slot %d... ", slot);

        if (check_uthernet_slot(slot)) {
            printf("Uthernet II trovato!\n");
        } else {
            printf("Nessuna scheda Uthernet II.\n");
        }
    }

    return 0;
}




// // Base addresses for slots and registers
// #define SLOT_BASE 0xC080      // Base address for the first slot
// #define MR_OFFSET 0x04        // Offset for the Memory Register (MR)
// #define DATA_OFFSET 0x07      // Offset for Data Register (DATA)
// #define SLOT_LEN 8            // Number of slots to scan (1 to 7)

// // Messages
// const char FOUND_MSG[] = "UTHERNET II FOUND IN SLOT: ";
// const char NOT_FOUND_MSG[] = "UTHERNET II NOT FOUND!";

// // Order of slots to scan
// const unsigned char slots[] = {7, 6, 5, 4, 3, 2, 1};

// void cout(char c) {
//     __asm__ (
//         "lda %0\n"        // Carica il valore di 'c' nel registro A
//         "jsr $FDED"       // Chiamata alla routine di sistema per stampare il carattere
//         :                  // Non ci sono valori di output
//         : "r" (c)          // Passa 'c' al codice assembly tramite un registro
//     );
// }


// // Function to output a string to the Apple II console
// void print(const char *str) {
//     while (*str) {
//         cout(*str++);
//     }
// }

// // Function to check if Uthernet II is present in a slot
// bool probe_slot(int slot) {
//     unsigned char *mr = (unsigned char *)(SLOT_BASE + (slot - 1) * 0x10 + MR_OFFSET);
//     unsigned char *data = (unsigned char *)(SLOT_BASE + (slot - 1) * 0x10 + DATA_OFFSET);

//     // Send reset command to the Memory Register (MR)
//     *mr = 0x80;
//     __asm__("nop"); // Wait for reset
//     __asm__("nop");

//     // Check if the slot responds with zero
//     if (*mr != 0x00) {
//         return false; // No valid response, skip slot
//     }

//     // Configure operating mode with auto-increment
//     *mr = 0x03; // Operating mode
//     if (*mr != 0x03) {
//         return false; // Unable to configure, skip slot
//     }

//     // If we reach here, the slot contains a Uthernet II
//     return true;
// }

// int main(void) {
//     bool found = false;
//     int i;              // Variable declaration
//     char slot_char;     // Variable declaration

//     print("SCANNER PER UTHERNET ][\n");

//     // Scan slots in the predefined order
//     for (i = 0; i < SLOT_LEN; i++) {
//         int slot = slots[i];

//         if (probe_slot(slot)) {
//             print(FOUND_MSG);

//             // Print the slot number
//             slot_char = (slot << 4) | 0xB0; // Convert slot to printable format
//             cout(slot_char); // Ensure slot_char is a character
//             cout('\n');

//             found = true;
//             break;
//         }
//     }

//     if (!found) {
//         print(NOT_FOUND_MSG);
//         cout('\n');
//     }

//     return 0;
// }

// #define SLOT_BASE 0xC080 // Indirizzo base per il primo slot

// // Funzione per verificare se una scheda è presente in un determinato slot
// bool check_card_in_slot(int slot) {
//     unsigned char *slot_address = (unsigned char *)(SLOT_BASE + (slot - 1) * 0x10);
//     unsigned char response;

//     // Lettura dell'indirizzo della scheda nello slot
//     response = *slot_address;

//     // printf("Valore letto nello slot %d: 0x%02X --- ", slot, response);


//     // Verifica della presenza della scheda (il comportamento dipende dalla scheda stessa)
//     // La Uthernet II dovrebbe rispondere a un accesso in modo specifico
//     return (response != 0xFF); // La scheda risponde con un valore diverso da 0xFF
// }

// // Funzione per verificare se una scheda è presente e identificare la Uthernet II
// bool is_uthernet_ii(int slot) {
//     unsigned char *slot_address;
//     unsigned char response;
//     unsigned char *control_register;
//     unsigned char test_response;

//     // Calcolo dell'indirizzo di memoria dello slot
//     slot_address = (unsigned char *)(SLOT_BASE + (slot - 1) * 0x10);

//     // Lettura dell'indirizzo della scheda nello slot
//     response = *slot_address;

//     // Verifica della presenza di una scheda generica
//     if (response == 0xFF) 
//     {
//         printf("Scheda assente nello slot %d | valore letto: 0x%02X\n", slot, response);
//         return false; // Nessuna scheda presente
//     }

//     printf("Slot %d: Scheda rilevata (valore letto: 0x%02X)\n", slot, response);


//     // Calcolo dell'indirizzo del registro di controllo (ipotetico)
//     control_register = (unsigned char *)(SLOT_BASE + (slot - 1) * 0x10 + 0x01);

//     // Scriviamo un valore e leggiamo la risposta
//     *control_register = 0x01; // Scrittura ipotetica
//     test_response = *control_register; // Lettura ipotetica

//     // Controlliamo se il comportamento è quello della Uthernet II
//     if (test_response == 0x55) { // Esempio di risposta unica della scheda
//         return true;
//     }

//     return false; // Non è la Uthernet II
// }

// int main(void) {
//     int slot;         // Dichiarazione della variabile "slot"
//     int slot_found = -1;

//     printf("Scanning slots for Uthernet II...\n\n");

//     for (slot = 0; slot < 8; slot++) {
//         if (is_uthernet_ii(slot))
//             printf("UTHERNET trovata nello slot %d :-)\n", slot);
//     }

//     return 0;
// }