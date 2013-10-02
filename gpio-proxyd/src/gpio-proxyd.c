//
// gpio-proxyd
// 
// This is part of gpio-proxy, a mechanism for developing device
// driver code in high-level languages.
//
// Use the '-d' flag to get debug output.  It is expected this 
// program will go in a startup script as:
//
//   /sbin/gpio-proxyd &
//
// (c) bifferos@yahoo.co.uk 2007
//

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include <stdlib.h>


struct NetworkPacket
{
  char operation;         // (G)et, (S)et, (R)equest, (F)ree
  unsigned char error;    // 0 == success, otherwise error number 0xff== request
  unsigned char rval;     // value read back   
  unsigned char wval;     // value to write
  unsigned long gpio;     // gpio pin
  char text[16];          // block of zero or more values
} NetworkPacket;


int g_Debug = 0;

void Log(char* format, ...)
{
  va_list ap;
  if (g_Debug)
  {
    printf("gpio-proxyd: ");
    vprintf(format, ap);
  }
  va_end(ap);
}


void LogError(char *context)
{
  if (g_Debug)
  {
    fprintf(stderr,"gpio-proxyd: ");
    perror(context);  
  }
  exit(-1);
}


void PrintPacket(struct NetworkPacket* pkt)
{
  Log("Op: %c\n", pkt->operation);
  Log("pin: 0x%x\n", pkt->gpio);
}


// Send response packet.
void Respond(int s, const struct sockaddr_in* dest, socklen_t salen, 
                                  struct NetworkPacket* pkt)
{
  ssize_t n;
  n = sendto(s, pkt, sizeof(pkt), 0, (const struct sockaddr*) dest, salen);
  if (n != sizeof(pkt))
  {
    Log("WARNING: Error sending response packet\n");
  }
}


int main(int argc, char* argv[])
{
  int sock;
  struct sockaddr_in peer, sender;
  int n;
  socklen_t salen;
  int fd;
  int written;
  struct NetworkPacket pkt;
  char qerr;
  int io_res;
  
  if (argc>1)
  {
    if (strcmp(argv[1],"-d")==0)
    {
      g_Debug = 1;
      Log("Entering 'debug' mode.\n");
    }
  }

  // check if communication device exists
  if (access("/dev/gpio_proxy", F_OK))
  {
    // if not create it.
    Log("/dev/gpio_proxy does not exist, attempting to create it.\n");
    if (mknod("/dev/gpio_proxy", S_IFCHR, makedev(10,152)))
    {
      LogError("Error creating /dev/gpio_proxy");
    }
  }

  // open the gpio device
  fd = open("/dev/gpio_proxy",O_RDWR);

  if (!fd)
  {
    LogError("Unable to open /dev/gpio_proxy\n");
  }

  peer.sin_family = AF_INET;
  peer.sin_port = htons(5122);
  peer.sin_addr.s_addr = htonl(INADDR_ANY);
  
  sock = socket(AF_INET, SOCK_DGRAM, 0);
  bind(sock, (struct sockaddr *)&peer,sizeof(peer));
  salen = sizeof(sender);
  Log("Waiting for requests on port 5122...\n");
  while (1)
  {
    n = recvfrom(sock, (char*)&pkt, sizeof(pkt), 0, (struct sockaddr *)&sender, &salen);
    // check if received data is consistent, and error represents a request.
    if (pkt.error==0xff && (n>=8))
    {
      pkt.error = 0;  // no error/response.
      PrintPacket(&pkt);
      qerr = 0;  // no error.
      
      io_res = ioctl(fd, 1, (char*)&pkt );
      if (io_res)
      { 
        Log("Error calling ioctl for read %d\n", io_res);
        qerr = 4;
      }

      if (qerr) 
      {
        pkt.error = qerr;
      }
      
      Respond(sock, &sender, salen, &pkt);
    }
    else
    {
      Log("Received invalid network packet, discarding.\n");
    }
  }
}


