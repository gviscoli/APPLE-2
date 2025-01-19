#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Compilazione
//
// cl65 -t apple2 helloworld.c -o helloworld.bin -O  -m helloworld.bin.map -vm ../00.IP65/ip65/ip65.lib ../00.IP65/drivers/ip65_apple2.lib
//

void prinPCH()
{
  printf(" #####   #####  #   #  \n");
  printf(" #    #  #      #   #  \n");
  printf(" #####   #      #####  \n");
  printf(" #       #      #   #  \n");
  printf(" #       #####  #   #  \n");

}


void main()
{

  printf("\n\n");

  prinPCH();

  printf("\n\n");

  printf("Copyright 2025 - Viscoli Giuseppe\n");
  printf("=================================\n");
}

