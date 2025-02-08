#include "../00.LIBRERIE/CC65/inc/cc65.h"
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../00.LIBRERIE/IP65/inc/ip65.h"

// Compilazione
//
// cl65 -t apple2 pch-tcp.c -o pch-tcp.bin -O  -m pch-tcp.map -vm ../00.LIBRERIE/IP65/lib/ip65.lib ../00.LIBRERIE/IP65/lib/ip65_tcp.lib ../00.LIBRERIE/IP65/drivers/ip65_apple2.lib ../00.LIBRERIE/CC65/lib/apple2.lib
//

//
// java -jar C:\UTILITY\AppleCommander\AppleCommander-win32-x86_64-1.9.0.jar
// 
//


#define LEN 500
#define SRV "192.100.1.211"

unsigned char buf[1500];
int len;

void error_exit(void)
{
  printf("Error $%X\n", ip65_error);
  if (doesclrscrafterexit())
  {
    printf("Press any key\n");
    cgetc();
  }
  exit(1);
}

void tcp_recv(const unsigned char* tcp_buf, int tcp_len)
{
  if (len)
  {
    return;
  }
  len = tcp_len;
  if (len != -1)
  {
    memcpy(buf, tcp_buf, len);
  }
}

void main(void)
{
  unsigned i;
  unsigned n;
  unsigned long srv;
  char key;

  printf("PCH (c)\n");

  if(!(srv = parse_dotted_quad(SRV)))
  {
    error_exit();
  }

  printf("Init\n");

  printf("Slots loop\n");
  
  for (n= 0; n < 8; n++) {
      printf("Check slot: %d, risultato: %d\n",n, ip65_init(n));
  }

  // printf("ETH_INIT_DEFAULT: %d\n", ETH_INIT_DEFAULT);

  // if (ip65_init(ETH_INIT_DEFAULT))
  // {
  //   error_exit();
  // }

  printf("DHCP\n");
  if (dhcp_init())
  {
    error_exit();
  }

  printf("IP Addr: %s\n", dotted_quad(cfg_ip));
  printf("Netmask: %s\n", dotted_quad(cfg_netmask));
  printf("Gateway: %s\n", dotted_quad(cfg_gateway));
  printf("DNS Srv: %s\n", dotted_quad(cfg_dns));

  printf("Connect to %s\n", SRV);
  if (tcp_connect(srv, 6502, tcp_recv))
  {
    error_exit();
  }

  printf("(T)CP or e(X)it\n");
  do
  {
    if (kbhit())
    {
      key = cgetc();
    }
    else
    {
      key = '\0';
    }

    if (key == 't')
    {
      printf("Send Len %d", LEN);
      for (i = 0; i < LEN; ++i)
      {
        buf[i] = i;
      }
      if (tcp_send(buf, LEN))
      {
        printf("!\n");
      }
      else
      {
        printf(".\n");
      }
    }

    ip65_process();

    if (len == -1)
    {
      printf("Disconnect\n");
    }
    else if (len)
    {
      printf("Recv Len %d", len);
      for (i = 0; i < len; ++i)
      {
        if ((i % 11) == 0)
        {
          ip65_process();

          printf("\n$%04X:", i);
        }
        printf(" %02X", buf[i]);
      }
      len = 0;
      printf(".\n");
    }
  }
  while (key != 'x' && len != -1);

  printf("Close\n");
  if (tcp_close())
  {
    error_exit();
  }

  printf("Done\n");
}
