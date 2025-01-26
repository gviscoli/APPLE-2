#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include "E:\SVILUPPO\APPLE 2\00.LIBRERIE\CC65\inc\apple2.h"


// Both pragmas are obligatory to have cc65 generate code
// suitable to access the W5100 auto-increment registers.
#pragma optimize      (on)
#pragma static-locals (on)


//#ifdef __APPLE2ENH__
//#include "../demo/stream.h"
//#else
#include "stream3.h"
//#endif

// Compilazione
//
// cl65 -t apple2 test.c stream3.c -o test.bin -O  -m test.map -vm ../00.LIBRERIE/IP65/lib/ip65.lib ../00.LIBRERIE/IP65/drivers/ip65_apple2.lib
//

#define MIN(a,b) (((a)<(b))?(a):(b))

byte ip_addr[4] = {192, 100,   1, 211};
byte submask[4] = {255, 255, 255, 0};
byte gateway[4] = {192, 100,   1, 1};

byte server[4] = {192, 100,   1, 222};

byte buffer[0x1000];


int main()
{
    clrscr();

    printf("\aInit\n");
    if (!stream_init(0xC0B4, ip_addr,
                            submask,
                            gateway))
    {
        printf("No Hardware Found\n");
        return 0;
    }
    printf("Connect\n");
    if (!stream_connect(server, 6502))
    {
        printf("Faild To Connect To %d.%d.%d.%d\n", server[0],
                                                    server[1],
                                                    server[2],
                                                    server[3]);
        return 0;
    }
    printf("Connected To %d.%d.%d.%d\n", server[0],
                                        server[1],
                                        server[2],
                                        server[3]);


    printf("END\n");

    return 1;
}