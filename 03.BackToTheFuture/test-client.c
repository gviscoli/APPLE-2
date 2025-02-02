// Both pragmas are obligatory to have cc65 generate code
// suitable to access the W5100 auto-increment registers.
#pragma optimize      (on)
#pragma static-locals (on)

#include <stdio.h>

#ifdef __APPLE2ENH__
#include "../demo/stream.h"
#else
#include "stream3.h"
#endif

// Compilazione
//
// cl65 -t apple2 test-client.c stream3.c -o test-client.bin -O  -m test-client.map -vm ../00.LIBRERIE/IP65/lib/ip65.lib ../00.LIBRERIE/IP65/drivers/ip65_apple2.lib
//
//
// java -jar C:\UTILITY\AppleCommander\AppleCommander-win32-x86_64-1.9.0.jar
// 
//

#define MIN(a,b) (((a)<(b))?(a):(b))

byte ip_addr[4] = {192, 100,   1, 222};
byte submask[4] = {255, 255, 255, 0};
byte gateway[4] = {192, 100,   1, 1};

byte server[4] = {192, 100,   1, 211};

byte buffer[0x1000];

void main(void)
{
  printf("\aInit\n");
  if (!stream_init(0xC0B4, ip_addr,
                           submask,
                           gateway))
  {
    printf("No Hardware Found\n");
    return;
  }
  printf("Connect\n");
  if (!stream_connect(server, 6502))
  {
    printf("Faild To Connect To %d.%d.%d.%d\n", server[0],
                                                server[1],
                                                server[2],
                                                server[3]);
    return;
  }
  printf("Connected To %d.%d.%d.%d\n", server[0],
                                       server[1],
                                       server[2],
                                       server[3]);

  for (;;)
  {
    word rcv, snd, pos, i;

    if (!stream_connected())
    {
      printf("Disconnected\n\a");
      return;
    }

    if (!(rcv = stream_receive_request()))
    {
      continue;
    }
    rcv = MIN(rcv, sizeof(buffer));
    printf("Recv Length: %d\n", rcv);
    for (i = 0; i < rcv; ++i)
    {
      buffer[i] = *stream_data;
    }
    stream_receive_commit(rcv);

    pos = 0;
    while (rcv)
    {
      while (!(snd = stream_send_request()))
      {
        printf("Send Window Empty\n");
      }
      snd = MIN(snd, rcv);
      printf("Send Length: %d\n", snd);
      for (i = 0; i < snd; ++i)
      {
        *stream_data = buffer[pos + i];
      }
      stream_send_commit(snd);
      pos += snd;
      rcv -= snd;
    }
  }
}
