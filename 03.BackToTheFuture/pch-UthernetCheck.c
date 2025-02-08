#include "../00.LIBRERIE/CC65/inc/cc65.h"
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../00.LIBRERIE/IP65/inc/ip65.h"

// Compilazione
//
// cl65 -t apple2 pch-UthernetCheck.c -o pch-Uth.bin -O  -m pch-UthernetCheck.map -vm ../00.LIBRERIE/IP65/lib/ip65.lib ../00.LIBRERIE/IP65/lib/ip65_tcp.lib ../00.LIBRERIE/IP65/drivers/ip65_apple2.lib ../00.LIBRERIE/CC65/lib/apple2.lib
//

//
// java -jar C:\UTILITY\AppleCommander\AppleCommander-win32-x86_64-1.9.0.jar
// 
//


void main(void)
{
  unsigned int n;
  unsigned int results[7];

  printf("PCH (c)\n");

  printf("Start - slots loop\n");

  for (n=0; n < 7; n++) {
      results[n] = ip65_init(n);
      if (results[n] != 0) {
        printf("Uthernet not present in slot %d\n", n);
      }
      else {
        printf("Uthernet found in slot %d\n", n);
      }
  }

  printf("Done\n");
}
