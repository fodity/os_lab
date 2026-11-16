#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SADDR struct sockaddr
#define SIZE sizeof(struct sockaddr_in)

int main(int argc, char *argv[]) {
  int fd;
  int nread;
  struct sockaddr_in servaddr;

  // проверяем, что передали 3 аргумента
  if (argc < 4) {
    printf("Usage: %s <ip> <port> <bufsize>\n", argv[0]);
    exit(1);
  }

  // выделяем память под буффер
  int bufsize = atoi(argv[3]);
  char *buf = malloc(bufsize);
  if (!buf) {
    perror("malloc");
    exit(1);
  }

  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket creating");
    exit(1);
  }
  

  //заполняем структуру адреса
  memset(&servaddr, 0, SIZE);
  servaddr.sin_family = AF_INET;

  // argv[1] - это айпи
  if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
    perror("bad address");
    exit(1);
  }

  // argv[2] - это порт 
  servaddr.sin_port = htons(atoi(argv[2]));
  

  //подключиться к серваку
  if (connect(fd, (SADDR *)&servaddr, SIZE) < 0) {
    perror("connect");
    exit(1);
  }
  

  //цикл отправки сообещний
  (void)write(1, "Input message to send\n", 22);
  
  // читаем с клавы и шлем в сокет
  while ((nread = read(0, buf, bufsize)) > 0) {
    if (write(fd, buf, nread) < 0) {
      perror("write");
      exit(1);
    }
  }

  close(fd);
  free(buf);
  exit(0);
}