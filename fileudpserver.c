

#include "fileudp.h"

void delay(unsigned int mseconds) {
  clock_t goal = mseconds + clock();
  while (goal > clock())
    ;
}
void *timeout(void *input) {
  // if packey not acked in 3ms time it out
  delay(3);
  if (((struct args *)input)->timeout_arr[((struct args *)input)->index] != 2) {
    ((struct args *)input)->timeout_arr[((struct args *)input)->index] = 1;
    return NULL;
  }
  // if packet acked return -1
  return NULL;
}

// function to encrypt
char Cipher(char ch) { return ch ^ cipherKey; }

// function sending file
int sendFile(FILE *fp, char *buf, int s) {
  int i, len;
  if (fp == NULL) {
    strcpy(buf, nofile);
    len = strlen(nofile);
    buf[len] = EOF;
    for (i = 0; i <= len; i++)
      buf[i] = Cipher(buf[i]);
    return 1;
  }

  char ch, ch2;
  for (i = 0; i < s; i++) {
    ch = fgetc(fp);
    ch2 = Cipher(ch);
    buf[i] = ch2;
    if (ch == EOF)
      return 1;
  }
  return 0;
}

// driver code
int main(int argc, char *argv[]) {

  int sockfd, nBytes, PORT;
  struct sockaddr_in addr_con;
  int addrlen = sizeof(addr_con);
  int WIN_SIZE;
  if (argc >= 2) {
    PORT = atoi(argv[1]);
  }

  addr_con.sin_family = AF_INET;
  addr_con.sin_port = htons(PORT);
  addr_con.sin_addr.s_addr = INADDR_ANY;
  char net_buf[512];
  FILE *fp;

  // socket()
  sockfd = socket(AF_INET, SOCK_DGRAM, IP_PROTOCOL);
  struct timeval tv;
  tv.tv_sec = 0;     // 30 Secs Timeout
  tv.tv_usec = 3000; // Not init'ing this can cause strange errors
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv,
             sizeof(struct timeval));

  if (sockfd < 0)
    printf("\nfile descriptor not received!!\n");
  else
    printf("\nfile descriptor %d received\n", sockfd);

  // bind()
  if (bind(sockfd, (struct sockaddr *)&addr_con, sizeof(addr_con)) == 0)
    printf("\nSuccessfully binded!\n");
  else
    printf("\nBinding Failed!\n");

  FileUDPPacket *Readreq = calloc(1, sizeof(FileUDPPacket));
  FileUDPPacket *RRQreply = calloc(1, sizeof(FileUDPPacket));

  printf("\nWaiting for file name...\n");

  // absolutely do not use goto, this is just bare minimum for testing and
  // learning
start:
  nBytes = recvfrom(sockfd, (char *)Readreq, NET_BUF_SIZE, sendrecvflag,
                    (struct sockaddr *)&addr_con, &addrlen);
startlow:
  WIN_SIZE = Readreq->win_size;
  fp = fopen(Readreq->data, "r");

  printf("\nFile Name Received: %s\n", Readreq->data);

  if (fp == NULL) {
    printf("\nFile open failed!\n");
    RRQreply->seq_no = 0;
    RRQreply->win_size = WIN_SIZE;
    RRQreply->type = ERROR;
    strcpy(RRQreply->data, "\0");

    sendto(sockfd, (char *)RRQreply, NET_BUF_SIZE, sendrecvflag,
           (struct sockaddr *)&addr_con, addrlen);
    goto start;
  } else {
    RRQreply->seq_no = 0;
    RRQreply->win_size = WIN_SIZE;
    RRQreply->type = ACK;
    strcpy(RRQreply->data, "\0");

    sendto(sockfd, (char *)RRQreply, NET_BUF_SIZE, sendrecvflag,
           (struct sockaddr *)&addr_con, addrlen);
    printf("%d", WIN_SIZE);
    printf("\nFile Successfully opened!\n");

    char *timeout_window = calloc(1, WIN_SIZE * sizeof(char *));

    int seq_no = 0;
    // pthread_t timeout_threads[WIN_SIZE];
    // struct args **threadi =
    //     (struct args **)malloc(WIN_SIZE * sizeof(struct args *));
    // for (int i = 0; i < WIN_SIZE; i++) {
    //   threadi[i] = (struct args *)malloc(sizeof(struct args));
    // }
    FileUDPPacket *sendData = calloc(1, sizeof(FileUDPPacket));
    FileUDPPacket *recAck = calloc(1, sizeof(FileUDPPacket));

    do {
      sendData->type = DATA;
      sendData->win_size = WIN_SIZE;
      // strcpy(sendData->data, "Hello, World!");
      sendFile(fp, sendData->data, NET_BUF_SIZE - 3);
      sendData->seq_no = seq_no;
      seq_no = (seq_no + 1) % WIN_SIZE;
      sendto(sockfd, (char *)sendData, NET_BUF_SIZE, sendrecvflag,
             (struct sockaddr *)&addr_con, addrlen);
      // printf("\n%s\n", (char *)Readreq);
      recvfrom(sockfd, (char *)recAck, NET_BUF_SIZE, sendrecvflag,
               (struct sockaddr *)&addr_con, &addrlen);
      // free(threadi);
    } while (1 == 1);
  }
  free(Readreq);
  free(RRQreply);

  return 0;
}
