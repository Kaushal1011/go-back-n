#include "fileudp.h"

// function for decryption
char Cipher(char ch) {
  return ch; //^ cipherKey;
}

// function to receive file
int recvFile(char *buf, int s) {
  int i;
  char ch;
  for (i = 0; i < s; i++) {
    ch = buf[i];
    ch = Cipher(ch);
    if (ch == EOF)
      return 1;
    else
      printf("%c", ch);
  }
  return 0;
}

// driver code
int main(int argc, char *argv[]) {
  int sockfd, nBytes, PORT_NO, WIN_SIZE;
  char IP_ADDRESS[20];
  struct sockaddr_in addr_con;
  int addrlen = sizeof(addr_con);
  int timeout = 0;

  if (argc >= 2) {
    strcpy(IP_ADDRESS, argv[1]);
    PORT_NO = atoi(argv[2]);
    WIN_SIZE = atoi(argv[3]);
  }

  addr_con.sin_family = AF_INET;
  addr_con.sin_port = htons(PORT_NO);
  addr_con.sin_addr.s_addr = inet_addr(IP_ADDRESS);
  char net_buf[NET_BUF_SIZE];
  FILE *fp;

  // socket()
  sockfd = socket(AF_INET, SOCK_DGRAM, IP_PROTOCOL);

  if (sockfd < 0)
    printf("\nfile descriptor not received!!\n");
  else
    printf("\nfile descriptor %d received\n", sockfd);

  printf("\nPlease enter file name to receive:\n");
  scanf("%s", net_buf);
  // dont use goto, bare minimum implementation

  FileUDPPacket *Readreq = calloc(1, sizeof(FileUDPPacket));
  FileUDPPacket *Recvack = calloc(1, sizeof(FileUDPPacket));
start:
  Readreq->type = RRQ;
  Readreq->win_size = WIN_SIZE;
  Readreq->seq_no = 0;
  strcpy(Readreq->data, net_buf);
  // char sendbuf[NET_BUF_SIZE] = (char *)Readreq;
  sendto(sockfd, (char *)Readreq, NET_BUF_SIZE, sendrecvflag,
         (struct sockaddr *)&addr_con, addrlen);
  // printf("\n%s\n", (char *)Readreq);
  recvfrom(sockfd, (char *)Recvack, NET_BUF_SIZE, sendrecvflag,
           (struct sockaddr *)&addr_con, &addrlen);
  printf("%d %d %s", Recvack->seq_no, Recvack->type, (char *)Recvack);
  if (Recvack->seq_no == 0 && Recvack->type == ACK) {
    // start file transmission
    FileUDPPacket *recFile = calloc(1, sizeof(FileUDPPacket));
    FileUDPPacket *sendAck = calloc(1, sizeof(FileUDPPacket));
    printf("transmission started");
    int lrlowack = -1;
    while (1) {
      recvfrom(sockfd, (char *)recFile, NET_BUF_SIZE, sendrecvflag,
               (struct sockaddr *)&addr_con, &addrlen);
      // printf("%s", (char *)recFile);

      if (recFile->type == DATA && recFile->seq_no == (lrlowack + 1)) {
        recvFile(recFile->data, NET_BUF_SIZE - 3);
        strcpy(sendAck->data, "\0");
        sendAck->type = ACK;
        sendAck->win_size = WIN_SIZE;
        sendAck->seq_no = lrlowack + 1;
        lrlowack = (lrlowack + 1) % WIN_SIZE;
        sendto(sockfd, (char *)Readreq, NET_BUF_SIZE, sendrecvflag,
               (struct sockaddr *)&addr_con, addrlen);
      } else {
        // discard all out od order packet
        printf("discarded out of order packet");
      }
    }
  } else if (Recvack->seq_no == 0 && Recvack->type == ERROR) {
    // start file transmission
    printf("Server Errored");
  } else {
    printf("connect failed");
    // no
    goto start;
  }

  return 0;
}
