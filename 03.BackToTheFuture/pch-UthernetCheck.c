#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../00.LIBRERIE/CC65/inc/cc65.h"
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


// ------------
// Main
//
// Descrizione: questo programma verifica se una scheda Uthernet è installata
// e nel caso positivo recupera un indirizzo DCHP e lo stampa a video
//
void main(void)
{
  unsigned int slot;
  //uint32_t ip_addr_destination[4] = {192, 100,   1, 211};

  uint32_t ip_addr_destination = ((uint32_t)192 << 24) | 
                                  ((uint32_t)100 << 16) | 
                                  ((uint32_t)1 << 8) | 
                                  (uint32_t)17;

  uint16_t result, i;

  uint16_t __fastcall__ icmp_ping(uint32_t dest);

  printf("\n");

  printf("Start Uthernet searching...\n");
  slot = check_uthernet();
  if (slot == -1)
  {
    printf("ERROR Uthernet NO FOUND!\n");
  }
  else
  {
    printf("Uthernet FOUND at slot: %d\n",slot);
  }

  // Inizializza la rete
  //
  if(ip65_init(slot))  
  {
    printf("ERROR init ip65 init failed!\n");
    return;
  }
  else
  {
    printf("ip65 init OK\n");
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

  // printf("Destination Ip Address: %02X %02X %02X %02X\n",
  //   ip_addr_destination[0], ip_addr_destination[1], ip_addr_destination[2], ip_addr_destination[3]);

  for (i = 0; i < 10; i++)
  {
    result = icmp_ping(ip_addr_destination);
    printf("Ping result: %d\n", result);  
  }
  


  printf("\nCopyright 2025 - Viscoli Giuseppe\n");
}
