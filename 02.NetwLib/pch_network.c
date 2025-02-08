#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../00.LIBRERIE/IP65/inc/ip65.h"

// cc65 -Oirs pch-network-lib.c -o pch-network-lib.s
// ca65 pch-network-lib.s -o pch-network-lib.o
// ar65 a pch-network.lib pch-network-lib.o

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
