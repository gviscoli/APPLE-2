#include "../00.LIBRERIE/CC65/inc/cc65.h"
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../00.LIBRERIE/IP65/inc/ip65.h"
#include "../00.LIBRERIE/PCH/pch_network.h"

// Compilazione
//
// cl65 -t apple2 pch-UthernetCheck.c -o pch-Uth.bin -O  -m pch-UthernetCheck.map -vm ../00.LIBRERIE/IP65/lib/ip65.lib ../00.LIBRERIE/IP65/lib/ip65_tcp.lib ../00.LIBRERIE/IP65/drivers/ip65_apple2.lib ../00.LIBRERIE/CC65/lib/apple2.lib
//

//
// java -jar C:\UTILITY\AppleCommander\AppleCommander-win32-x86_64-1.9.0.jar
//
// java -jar C:\UTILITY\AppleCommander\AppleCommander-ac-1.9.0.jar -d build\BackToTheFuture.dsk pch.Uth.bin
// java -jar C:\UTILITY\AppleCommander\AppleCommander-ac-1.9.0.jar -as build\BackToTheFuture.dsk pch.Uth.bin < pch-Uth.bin
// 
//

// // ---------------------------------------------------------------------------
// // Nome: void print_ip(uint32_t ip)
// //
// // Descrizione: Funzione per stampare l'indirizzo IP attuale
// //
// //
// void print_ip(uint32_t ip) 
// {
//   unsigned char *bytes = (unsigned char *)&ip;  // Converte in array di 4 byte
//   printf("%d.%d.%d.%d\n", bytes[0], bytes[1], bytes[2], bytes[3]);
// }

// // ---------------------------------------------------------------------------
// // Nome: int check_uthernet()
// // Return: 
// // -1 se non è stata trovata una scheda uthernet
// // un intero maggiore di 1 che rappresenta lo slot dove è installata la scheda
// //
// //
// unsigned int check_uthernet()
// {
//   unsigned int results[7];
//   unsigned int n;
//   unsigned int slot;

//   slot = -1;

//   for (n=0; n < 7; n++) {
//     results[n] = ip65_init(n);
//     if (results[n] == 0) {
//       slot = n;
//       break;
//     }
//   }

//   return slot;
// }

// ------------
// Main
//
// Descrizione: questo programma verifica se una scheda Uthernet è installata
// e nel caso positivo recupera un indirizzo DCHP e lo stampa a video
//
void main(void)
{
  unsigned int slot;

  printf("\n");

  printf("Start Uthernet Init\n");
  slot = check_uthernet();
  if (slot == -1)
  {
    printf("ERROR Uthernet NO FOUND!\n");
  }
  else
  {
    printf("Uthernet FOUND at slot: %d\n",slot);
  }

  printf("Obtaining IP address \n");
  if (dhcp_init())
  {
    printf("ERROR DHCP init failed!\n");
    return;
  }
  else
  {
    printf("DHCP init OK\n");
  }

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

  printf("Copyright 2025 - Viscoli Giuseppe\n");
}
