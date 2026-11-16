#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define SADDR struct sockaddr
#define SLEN sizeof(struct sockaddr_in)

int main(int argc, char **argv) {
  int sockfd, n;
  struct sockaddr_in servaddr;

  // так же ожидаем три аргумента
  if (argc < 4) {
    printf("Usage: %s <ip> <port> <bufsize>\n", argv[0]);
    exit(1);
  }

  int port = atoi(argv[2]);
  int bufsize = atoi(argv[3]);
  
  // выделяем память. для recvline берем bufsize + 1, чтобы гарантированно поставить \0 в конце.
  char *sendline = malloc(bufsize);
  char *recvline = malloc(bufsize + 1);
  if (!sendline || !recvline) exit(1);

  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(port);

  if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
    perror("inet_pton problem");
    exit(1);
  }
  
  // SOCK_DGRAM означает UDP
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket problem");
    exit(1);
  }

  (void)write(1, "Enter string\n", 13);

  while ((n = read(0, sendline, bufsize)) > 0) {
    // sendto отправляет пакет на конкретный адрес
    if (sendto(sockfd, sendline, n, 0, (SADDR *)&servaddr, SLEN) == -1) {
      perror("sendto problem");
      exit(1);
    }

    // recvfrom ждет ответный пакет
    if (recvfrom(sockfd, recvline, bufsize, 0, NULL, NULL) == -1) {
      perror("recvfrom problem");
      exit(1);
    }
    
    recvline[n] = '\0'; // зануляем строку, чтобы printf не вывел мусор
    printf("REPLY FROM SERVER= %s\n", recvline);
  }
  
  close(sockfd);
  free(sendline);
  free(recvline);
  return 0;
}