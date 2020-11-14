

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
  tv.tv_sec = 0;      // 30 Secs Timeout
  tv.tv_usec = 30000; // Not init'ing this can cause strange errors
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
    int last_received_ack = -5;
    int last_to_last_ack = -5;
    int initial_ack = 0;
    int partindex = 0;
    fseek(fp, 0, SEEK_END); // seek to end of file
    int max_parts =
        ceil(ftell(fp) / (NET_BUF_SIZE - 3)) + 1; // get current file pointer
    printf("parts of file %d \n", max_parts);
    fseek(fp, 0, SEEK_SET); // seek back to beginning of file

    FileUDPPacket *sendData = calloc(1, sizeof(FileUDPPacket));
    FileUDPPacket *recAck = calloc(1, sizeof(FileUDPPacket));
    int ii = 0;
    do {
      printf("in loop");
      for (int i = 0; i < WIN_SIZE - 1; i++) {
        printf("seq no %d \n", seq_no);
        sendData->type = DATA;
        sendData->win_size = WIN_SIZE;
        // strcpy(sendData->data, "Hello, World!");
        // memset(sendData->data, 0, NET_BUF_SIZE - 3);
        // sendData->data = buffer;
        sendFile(fp, sendData->data, NET_BUF_SIZE - 3);
        sendData->seq_no = seq_no;

        sendto(sockfd, (char *)sendData, NET_BUF_SIZE, sendrecvflag,
               (struct sockaddr *)&addr_con, addrlen);
        partindex++;
        seq_no = (seq_no + 1) % WIN_SIZE;
        printf("in inner loop");
        recAck->type = -1;
        recAck->seq_no = -1;
        recvfrom(sockfd, (char *)recAck, NET_BUF_SIZE, sendrecvflag,
                 (struct sockaddr *)&addr_con, &addrlen);
        if (recAck->type == ACK && recAck->seq_no >= 0) {
          printf("ack received %d", recAck->seq_no);
          last_received_ack = recAck->seq_no;
          if (last_received_ack == 0) {
            initial_ack = 1;
          }
        } else if (recAck->type == RRQ) {
          goto startlow;
        } else {
          // printf("timed out ack receive");
        }
      }

      int rewind =
          WIN_SIZE - 1 - ((last_received_ack - last_to_last_ack) % WIN_SIZE);

      if (last_received_ack == last_to_last_ack) {
        if (initial_ack == 0 && last_received_ack == 0 &&
            last_to_last_ack == 0) {
          fseek(fp, -1 * (WIN_SIZE - 1) * (NET_BUF_SIZE - 3), SEEK_CUR);
          partindex -= (WIN_SIZE - 1);
        } else if (initial_ack == 1 && last_received_ack == 0 &&
                   last_to_last_ack == 0) {
          fseek(fp, -1 * (WIN_SIZE - 2) * (NET_BUF_SIZE - 3), SEEK_CUR);
          partindex -= (WIN_SIZE - 2);
        } else {
          fseek(fp, -1 * (WIN_SIZE - 1) * (NET_BUF_SIZE - 3), SEEK_CUR);
          partindex -= (WIN_SIZE - 1);
        }

        printf("seeked back by %d", WIN_SIZE);
      } else {
        fseek(fp, -1 * (rewind) * (NET_BUF_SIZE - 3), SEEK_CUR);
        partindex -= (rewind);
        printf("seeked back by %d", rewind);
      }

      seq_no = (last_received_ack + 1) % WIN_SIZE;
      last_to_last_ack = last_received_ack;
      if (ii > 500) {
        // limit to 1000 sends only
        break;
      }
      ii++;
      // free(threadi);
    } while (partindex <= max_parts && !feof(fp));
    free(sendData);
    free(recAck);
  }
  free(Readreq);
  free(RRQreply);

  return 0;
}
