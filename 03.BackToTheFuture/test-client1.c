#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../00.LIBRERIE/IP65/inc/ip65.h"
#include "../00.LIBRERIE/PCH/pch_network.h"

// Compilazione
//
// cl65 -t apple2 test-client1.c stream3.c -o test-client1.bin -O  -m test-client1.map -vm ../00.LIBRERIE/IP65/lib/ip65.lib ../00.LIBRERIE/IP65/drivers/ip65_apple2.lib
//

//
// java -jar C:\UTILITY\AppleCommander\AppleCommander-win32-x86_64-1.9.0.jar
// 
//

// Both pragmas are obligatory to have cc65 generate code
// suitable to access the W5100 auto-increment registers.
#pragma optimize      (on)
#pragma static-locals (on)

#define A2_SLOT_SCAN 1

// #ifdef __APPLE2ENH__
// #include "../demo/stream.h"
// #else
#include "stream3.h"
// #endif

#define MIN(a,b) (((a)<(b))?(a):(b))

// byte ip_addr[4] = {192, 100,   1, 222};
// byte submask[4] = {255, 255, 255, 0};
// byte gateway[4] = {192, 100,   1, 1};

byte server[4] = {192, 100,   1, 211};

byte buffer[0x1000];

unsigned int slot;
unsigned char ip_addr[4];
unsigned char netmask[4];
unsigned char gateway[4];

void uint32_to_bytes(uint32_t value, unsigned char bytes[4]) {
  bytes[0] = (value >> 24) & 0xFF;  // Byte più significativo
  bytes[1] = (value >> 16) & 0xFF;
  bytes[2] = (value >> 8)  & 0xFF;
  bytes[3] = (value >> 0)  & 0xFF;  // Byte meno significativo
}

void main(void)
{
  slot = check_uthernet();
  printf("Slot: %d\n", slot);
  if (slot == 0xFF)
  {
    printf("No Uthernet Found\n");
    return;
  }
  else if (slot != A2_SLOT_SCAN)
  {
    printf("Uthernet Found On Slot %d\n", slot);
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

  print_ip(cfg_ip);
  print_ip(cfg_netmask);
  print_ip(cfg_gateway);

  uint32_to_bytes(cfg_ip, ip_addr);
  uint32_to_bytes(cfg_netmask, netmask);
  uint32_to_bytes(cfg_gateway, gateway);

  printf("Bytes: %02X %02X %02X %02X\n", 
    ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3]);

  printf("Bytes: %02X %02X %02X %02X\n", 
    netmask[0], netmask[1], netmask[2], netmask[3]);

  printf("Bytes: %02X %02X %02X %02X\n", 
    gateway[0], gateway[1], gateway[2], gateway[3]);


  // printf("\aInit\n");
  // if (!stream_init(0xC0B4, ip_addr,
  //                          submask,
  //                          gateway))
  // {
  //   printf("No Hardware Found\n");
  //   return;
  // }
  
  printf("\aInit\n");
  if (!stream_init(0xC0B4, ip_addr,
                            netmask,
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
