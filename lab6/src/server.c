#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <getopt.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "common.h"



//как в клиенте. еденица работы для одного потока
struct FactorialArgs {
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
};


//произведение на отрезке по модулю
uint64_t Factorial(const struct FactorialArgs *args) {
  if (args->mod == 0) {
    return 0;
  }

  uint64_t ans = 1 % args->mod;

  if (args->begin > args->end) {
    return ans;
  }

  for (uint64_t i = args->begin;; i++) {
    ans = MultModulo(ans, i, args->mod);
    if (i == args->end)
      break;
  }

  return ans;
}



//функция, которую исполнят ВНУТРЕННИЙ поток сервера. (берет честь части массива и уже с ним работает)
void *ThreadFactorial(void *args) {
  struct FactorialArgs *fargs = (struct FactorialArgs *)args;
  uint64_t result = Factorial(fargs);
  return (void *)(uintptr_t)result;
}



int main(int argc, char **argv) {


  //получаем аргументы. гетоптаем их
  int tnum = -1;
  int port = -1;

  while (true) {
    int current_optind = optind ? optind : 1;
    (void)current_optind;

    static struct option options[] = {{"port", required_argument, 0, 0},
                                      {"tnum", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        port = atoi(optarg);
        break;
      case 1:
        tnum = atoi(optarg);
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':
      printf("Unknown argument\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (port < 1 || port > 65535 || tnum < 1) {
    fprintf(stderr, "Using: %s --port 20001 --tnum 4\n", argv[0]);
    return 1;
  }




  //создание сокета
  //создаем сокет
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    fprintf(stderr, "Can not create server socket!\n");
    return 1;
  }
  //структура для "соединения"
  struct sockaddr_in server;
  memset(&server, 0, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_port = htons((uint16_t)port);
  server.sin_addr.s_addr = htonl(INADDR_ANY);

  int opt_val = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));


  //привязка сокета к порту
  int err = bind(server_fd, (struct sockaddr *)&server, sizeof(server));
  if (err < 0) {
    fprintf(stderr, "Can not bind to socket!\n");
    close(server_fd);
    return 1;
  }
  


  //сокет слушает 
  err = listen(server_fd, 128);
  if (err < 0) {
    fprintf(stderr, "Could not listen on socket\n");
    close(server_fd);
    return 1;
  }

  printf("Server listening at %d\n", port);
  //бесконечный цикл ожидания клиентов. как только один подключился -
  // возвращаем новый сокет для общения с этим клиентом
  while (true) {
    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);
    int client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len);

    if (client_fd < 0) {
      fprintf(stderr, "Could not establish new connection\n");
      continue;
    }
    

    //внутри уже принятого соединения читаем запросы от клиента. 
    while (true) {
      const unsigned int buffer_size = sizeof(uint64_t) * 3;
      char from_client[buffer_size];

      int rr = RecvAll(client_fd, from_client, buffer_size);
      if (rr == 0) {
        break;
      }
      if (rr < 0) {
        fprintf(stderr, "Client read failed\n");
        break;
      }

      uint64_t begin = 0;
      uint64_t end = 0;
      uint64_t mod = 0;

      memcpy(&begin, from_client, sizeof(uint64_t));
      memcpy(&end, from_client + sizeof(uint64_t), sizeof(uint64_t));
      memcpy(&mod, from_client + 2 * sizeof(uint64_t), sizeof(uint64_t));

      if (mod == 0) {
        fprintf(stderr, "mod must be greater than zero\n");
        break;
      }

      fprintf(stdout, "Receive: %llu %llu %llu\n",
              (unsigned long long)begin, (unsigned long long)end,
              (unsigned long long)mod);
      
      

      //деление массива на tnum потоков и запуск
      pthread_t threads[tnum];
      struct FactorialArgs args[tnum];

      uint64_t total_elems = 0;
      if (end >= begin) {
        total_elems = end - begin + 1;
      }

      uint64_t base = total_elems / (uint64_t)tnum;
      uint64_t rem = total_elems % (uint64_t)tnum;

      int created = 0;
      for (int i = 0; i < tnum; i++) {
        if (total_elems == 0) {
          args[i].begin = 1;
          args[i].end = 0;
        } else {
          uint64_t len = base + ((uint64_t)i < rem ? 1 : 0);
          if (len == 0) {
            args[i].begin = 1;
            args[i].end = 0;
          } else {
            uint64_t start =
                begin + (uint64_t)i * base + ((uint64_t)i < rem ? (uint64_t)i : rem);
            args[i].begin = start;
            args[i].end = start + len - 1;
          }
        }

        args[i].mod = mod;

        if (pthread_create(&threads[i], NULL, ThreadFactorial,
                           (void *)&args[i]) != 0) {
          fprintf(stderr, "Error: pthread_create failed!\n");
          break;
        }
        created++;
      }

      if (created != tnum) {
        for (int i = 0; i < created; i++) {
          pthread_join(threads[i], NULL);
        }
        break;
      }


      //ждем все потоки, потом перемножаем и отправляем уже
      uint64_t total = 1 % mod;

      for (int i = 0; i < tnum; i++) {
        void *thread_result = NULL;
        pthread_join(threads[i], &thread_result);
        uint64_t result = (uint64_t)(uintptr_t)thread_result;
        total = MultModulo(total, result, mod);
      }

      printf("Total: %llu\n", (unsigned long long)total);

      if (SendAll(client_fd, &total, sizeof(total)) < 0) {
        fprintf(stderr, "Can't send data to client\n");
        break;
      }
    }

    shutdown(client_fd, SHUT_RDWR);
    close(client_fd);
  }

  close(server_fd);
  return 0;
}