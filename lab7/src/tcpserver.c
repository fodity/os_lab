#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SADDR struct sockaddr

int main(int argc, char *argv[]) {
  const size_t kSize = sizeof(struct sockaddr_in);  //размер структуры(надо для функций сокетов)
  int lfd, cfd;  //сокеты
  int nread;
  struct sockaddr_in servaddr, cliaddr;
  

  //проверка передачи аргументов
  if (argc < 3) {
    printf("Usage: %s <port> <bufsize>\n", argv[0]);
    exit(1);
  }

  //преобразование аргументов
  int port = atoi(argv[1]);
  int bufsize = atoi(argv[2]);
  char *buf = malloc(bufsize);
  if (!buf) {
    perror("malloc");
    exit(1);
  }


  //создаем сокет
  if ((lfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    exit(1);
  }

  //заполняем структуру, описывающую наш адрес
  memset(&servaddr, 0, kSize);  //зануляем заполненные поля
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);  //слушать все ip
  servaddr.sin_port = htons(port);

  // чтобы порт освобождался после перзерапуска сервера 
  int optval = 1;
  setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));


  //привязываем сокет к порту
  if (bind(lfd, (SADDR *)&servaddr, kSize) < 0) {
    perror("bind");
    exit(1);
  }


  //переводим сокет в режим "слушать"
  if (listen(lfd, 5) < 0) {
    perror("listen");
    exit(1);
  }
  
  printf("TCP Server listening on port %d...\n", port);

  while (1) {
    unsigned int clilen = kSize;

    if ((cfd = accept(lfd, (SADDR *)&cliaddr, &clilen)) < 0) { //блок проги, пока не будет клиента
      perror("accept");
      exit(1);
    }
    printf("connection established\n");

    // читаем данные от клиента и печатаем их
    while ((nread = read(cfd, buf, bufsize)) > 0) {
      (void)write(1, buf, nread); 
    }

    if (nread == -1) {
      perror("read");
      exit(1);
    }
    close(cfd);
  }
  
  free(buf);
  return 0;
}