#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <conio.h>                          /* FIX: cgetc() e' in conio.h di cc65 */

#include "../00.LIBRERIE/IP65/inc/ip65.h"
// #include "../00.LIBRERIE/PCH/pch_network.h"

#include "rest_lib.c"

// Compilazione
//
// cl65 -t apple2 rest_hello.c -o rest_hello.bin -O -m rest_hello.map -vm
//      ../00.LIBRERIE/IP65/lib/ip65.lib
//      ../00.LIBRERIE/IP65/lib/ip65_tcp.lib
//      ../00.LIBRERIE/IP65/drivers/ip65_apple2.lib
//      ../00.LIBRERIE/CC65/lib/apple2.lib
//      ../00.LIBRERIE/PCH/pch_network.lib

// Compilazione
//
// cl65 -t apple2 rest_hello.c -o rest_hello.bin -O  -m rest_hello.map -vm ../00.LIBRERIE/IP65/lib/ip65.lib ../00.LIBRERIE/IP65/lib/ip65_tcp.lib ../00.LIBRERIE/IP65/drivers/ip65_apple2.lib ../00.LIBRERIE/PCH/pch_network.lib
//
// QUELLO GIUSTO:
// cl65 -t apple2 rest_hello.c -o rest_hello.bin -O  -m rest_hello.map -vm ../00.LIBRERIE/IP65/lib/ip65_tcp.lib ../00.LIBRERIE/IP65/drivers/ip65_apple2.lib


//
// java -jar C:\UTILITY\AppleCommander\AppleCommander-win32-x86_64-1.9.0.jar
// 
//

// ---------------------------------------------------------------------------
// Nome: void print_ip(uint32_t ip)
//
// Descrizione: Funzione per stampare l'indirizzo IP attuale
//
//
void print_ip(uint32_t ip) 
{
  unsigned char *bytes = (unsigned char *)&ip;  // Converte in array di 4 byte
  printf("%d.%d.%d.%d\n", bytes[0], bytes[1], bytes[2], bytes[3]);
}

// ---------------------------------------------------------------------------
// Nome: int check_uthernet()
// Return: 
// -1 se non è stata trovata una scheda uthernet
// un intero maggiore di 1 che rappresenta lo slot dove è installata la scheda
//
//
unsigned int check_uthernet()
{
  unsigned int results[7];
  unsigned int n;
  unsigned int slot;

  slot = -1;

  for (n=0; n < 7; n++) {
    results[n] = ip65_init(n);
    if (results[n] == 0) {
      slot = n;
      break;
    }
  }

  return slot;
}

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
 * Utility: show_ipconfig
 * ----------------------------------------------- */
static void show_ipconfig() {
  printf("\n* -------------------------------- *\n\n");
  printf("  IP Address:  ");
  print_ip(cfg_ip);

  printf("  Netmask:     ");
  print_ip(cfg_netmask);

  printf("  Gateway:     ");
  print_ip(cfg_gateway);

  printf("  DNS Server:  ");
  print_ip(cfg_dns);

  printf("  DHCP Server: ");
  print_ip(dhcp_server);

  printf("\n* -------------------------------- *\n\n");

  printf("\n\n");
}


/* Test connettivita' base: prova a connetterti al router
 * (sostituisci con l'IP del tuo gateway) */
static void test_network(void) {
    uint32_t target_ip;
    char     buf[16];
    uint16_t result;

    /* Stampa la configurazione IP ottenuta dal DHCP */
    printf("IP Apple IIe: %u.%u.%u.%u\n",
        (unsigned)(cfg_ip      ) & 0xFF,
        (unsigned)(cfg_ip >>  8) & 0xFF,
        (unsigned)(cfg_ip >> 16) & 0xFF,
        (unsigned)(cfg_ip >> 24) & 0xFF);

    printf("Gateway     : %u.%u.%u.%u\n",
        (unsigned)(cfg_gateway      ) & 0xFF,
        (unsigned)(cfg_gateway >>  8) & 0xFF,
        (unsigned)(cfg_gateway >> 16) & 0xFF,
        (unsigned)(cfg_gateway >> 24) & 0xFF);           

    printf("Provo connect a %s:8080...\n", PROXY_HOST);

    /* Usa parse_dotted_quad per costruire l'IP correttamente */
    strncpy(buf, PROXY_HOST, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    target_ip = parse_dotted_quad(buf);

    printf("parse_dotted_quad(\"%s\") = %lu\n",
           PROXY_HOST, (unsigned long)target_ip);

    /* Ping con IP costruito correttamente */
    printf("PING A %s...\n", PROXY_HOST);
    result = icmp_ping(target_ip);
    printf("PING RESULT: %d\n", result);

    /* TCP connect con lo stesso IP */
    printf("TCP connect a %s:8080...\n", PROXY_HOST);
    if (tcp_connect(target_ip, 8080, tcp_recv_cb) != 0) {
        printf("FALLITO -> codice %d\n", (int)ip65_error);
    } else {
        puts("TCP connect OK!");
        tcp_close();
    }
}

// /* -----------------------------------------------
//  * Utility: ping
//  * ----------------------------------------------- */
// static void ping(uint32_t ip_addr_destination) {
//   uint16_t result, i;

//   for (i = 0; i < 3; i++)
//   {
//     result = icmp_ping(ip_addr_destination);
//     printf("Ping result: %d\n", result);  
//   }
// }

/* -----------------------------------------------
 * Menu interattivo
 * ----------------------------------------------- */
static void show_menu(void) {
    puts("\n--- REST Client Apple IIe ---");
    puts("0. SHOW IP CONFIGURATION");
    puts("1. TEST-CONNECTION");
    puts("2. GET  /api/data");
    puts("3. POST /api/data  (JSON)");
    puts("4. GET  custom path");
    puts("Q. Esci");
    printf("Scelta: ");
}

/* -----------------------------------------------
 * Entry Point
 * ----------------------------------------------- */
int main(void) {
    char     choice;
    int16_t  result;
    char     custom_path[64];              
    uint32_t ip_addr_destination = ((uint32_t)192 << 24) | 
                                    ((uint32_t)100 << 16) | 
                                    ((uint32_t)1 << 8) | 
                                    (uint32_t)17;

    if (!initialize_network()) {
        return 1;
    }

    for (;;) {
        show_menu();
        choice = cgetc();
        putchar('\n');

        switch (choice) {

            case '0':
                show_ipconfig();
                break;

            case '1':
                test_network();
                // ping(ip_addr_destination);
                break;

            case '2':
                result = do_get("/api/data?prompt=HELLO_FROM_APPLE2E",
                                response_buf,
                                RESPONSE_BUF_SIZE);
                
                printf("GET RESULT: %d\n", result);

                if (result > 0) {
                    response_buf[200] = '\0';
                    printf("%s\n", (char *)http_response_body);
                }
                break;

            case '3':
                result = do_post(
                    "/api/data",
                    "{\"device\":\"apple2e\","
                    "\"msg\":\"hello from 1983\","
                    "\"temp\":42}",
                    response_buf,
                    RESPONSE_BUF_SIZE);

                printf("POST RESULT: %d\n", result);

                break;

            case '4':
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