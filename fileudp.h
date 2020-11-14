#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define IP_PROTOCOL 0
#define NET_BUF_SIZE 512
#define cipherKey 'S'
#define sendrecvflag 0
#define nofile "File Not Found!"

#define RRQ 1
#define DATA 2
#define ACK 3
#define ERROR 4

struct args {
  char *timeout_arr;
  int index;
};

typedef struct __attribute__((__packed__)) FileUDPPacket {
  uint8_t type;
  uint8_t win_size;
  uint8_t seq_no;
  char data[NET_BUF_SIZE - 3];
} FileUDPPacket;
