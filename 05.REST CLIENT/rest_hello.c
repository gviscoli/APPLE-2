#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <conio.h>                          /* FIX: cgetc() e' in conio.h di cc65 */

#include "../00.LIBRERIE/IP65/inc/ip65.h"
#include "../00.LIBRERIE/PCH/pch_network.h"

#include "rest_lib.c"

// Compilazione
//
// cl65 -t apple2 rest_hello.c -o rest_hello.bin -O -m rest_hello.map -vm
//      ../00.LIBRERIE/IP65/lib/ip65.lib
//      ../00.LIBRERIE/IP65/lib/ip65_tcp.lib
//      ../00.LIBRERIE/IP65/drivers/ip65_apple2.lib
//      ../00.LIBRERIE/CC65/lib/apple2.lib
//      ../00.LIBRERIE/PCH/pch_network.lib

/* -----------------------------------------------
 * Inizializzazione rete e scheda Uthernet
 * ----------------------------------------------- */
bool initialize_network(void) {
    unsigned int slot;

    printf("\n");
    printf("Start Uthernet searching...\n");

    slot = check_uthernet();
    if (slot == (unsigned int)-1) {
        printf("ERROR Uthernet NO FOUND!\n");
        return false;
    }
    printf("Uthernet FOUND at slot: %d\n", slot);

    if (ip65_init(slot)) {
        printf("ERROR init ip65 init failed!\n");
        return false;
    }
    printf("ip65 init OK\n");

    puts("DHCP in corso...");
    if (dhcp_init() != 0) {
        puts("ERRORE: DHCP fallito");
        return false;
    }
    printf("DHCP OK\n");

    return true;
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
 * Entry Point
 * ----------------------------------------------- */
int main(void) {
    char     choice;
    int16_t  result;
    char     custom_path[64];              /* FIX: dichiarata qui, non mancava */

    if (!initialize_network()) {
        return 1;
    }

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
                {
                    uint8_t i = 0;
                    char    c;
                    /* FIX: sizeof(custom_path) ora funziona perche'
                     * custom_path e' un array dichiarato sopra       */
                    while (i < (uint8_t)(sizeof(custom_path) - 1)) {
                        c = cgetc();
                        if (c == '\r') break;
                        if (c == 0x08 && i > 0) {   /* backspace */
                            i--;
                            printf("\b \b");
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

    return 0;
}