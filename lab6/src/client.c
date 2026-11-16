#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "common.h"


//структура сервера
struct Server {
  char ip[255];
  int port;
};

//еденица работы для одного потока
struct ClientTask {
  struct Server server;
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
  uint64_t result;
  int ok;  //флаг получилось ли
};

//эта функция, которую будет исполнять поток
static void *ClientWorker(void *arg) {
  struct ClientTask *task = (struct ClientTask *)arg;
  task->ok = 0; //изначально не выполнено 


  //превращаем ip в бинарную структуру адреса
  struct addrinfo hints;
  struct addrinfo *res = NULL;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  char port_str[16];
  snprintf(port_str, sizeof(port_str), "%d", task->server.port);

  int gai = getaddrinfo(task->server.ip, port_str, &hints, &res);
  if (gai != 0) {
    fprintf(stderr, "getaddrinfo failed for %s: %s\n", task->server.ip,
            gai_strerror(gai));
    return NULL;
  }

 // устанавливаем TPC соединение

  //задаем сокет
  int sck = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (sck < 0) {
    fprintf(stderr, "Socket creation failed!\n");
    freeaddrinfo(res);
    return NULL;
  }

  //устанавливаем соединение
  if (connect(sck, res->ai_addr, res->ai_addrlen) < 0) {
    fprintf(stderr, "Connection failed to %s:%d\n", task->server.ip,
            task->server.port);
    close(sck);
    freeaddrinfo(res);
    return NULL;
  }

  freeaddrinfo(res);

  //отправка запроса
  uint64_t request[3];
  request[0] = task->begin;
  request[1] = task->end;
  request[2] = task->mod;

  if (SendAll(sck, request, sizeof(request)) < 0) {
    fprintf(stderr, "Send failed\n");
    close(sck);
    return NULL;
  }



  // прием ответа. принимаем одно число 8 байт
  uint64_t response = 0;
  int rr = RecvAll(sck, &response, sizeof(response));
  if (rr != 1) {
    fprintf(stderr, "Recieve failed\n");
    close(sck);
    return NULL;
  }


  //записываем результат и закрываем сокет
  task->result = response;
  task->ok = 1;

  close(sck);
  return NULL;
}

int main(int argc, char **argv) {

  //читаем входные параметры. записываем все в структуру
  uint64_t k = 0;
  uint64_t mod = 0;
  bool k_set = false;
  bool mod_set = false;
  char servers_path[PATH_MAX] = {'\0'};

  while (true) {
    int current_optind = optind ? optind : 1;
    (void)current_optind;

    static struct option options[] = {{"k", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {"servers", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        if (!ConvertStringToUI64(optarg, &k)) {
          fprintf(stderr, "Invalid --k value\n");
          return 1;
        }
        k_set = true;
        break;
      case 1:
        if (!ConvertStringToUI64(optarg, &mod)) {
          fprintf(stderr, "Invalid --mod value\n");
          return 1;
        }
        mod_set = true;
        break;
      case 2:
        snprintf(servers_path, sizeof(servers_path), "%s", optarg);
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':
      printf("Arguments error\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }




  if (!k_set || !mod_set || servers_path[0] == '\0') {
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
            argv[0]);
    return 1;
  }

  if (mod == 0) {
    fprintf(stderr, "mod must be greater than zero\n");
    return 1;
  }


  //для чтения файла
  FILE *f = fopen(servers_path, "r");
  if (f == NULL) {
    perror("fopen servers file");
    return 1;
  }

  struct Server *servers = NULL;
  size_t servers_num = 0;
  size_t capacity = 0;
  char line[1024];

  while (fgets(line, sizeof(line), f) != NULL) {
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
      line[--len] = '\0';
    }

    if (len == 0)
      continue;

    char *colon = strrchr(line, ':');
    if (colon == NULL) {
      fprintf(stderr, "Bad server line: %s\n", line);
      continue;
    }

    *colon = '\0';
    char *ip = line;
    char *port_str = colon + 1;

    char *endptr = NULL;
    errno = 0;
    long port = strtol(port_str, &endptr, 10);

    if (errno != 0 || endptr == port_str || *endptr != '\0' || port < 1 ||
        port > 65535) {
      fprintf(stderr, "Bad port in line: %s:%s\n", ip, port_str);
      continue;
    }

    if (strlen(ip) >= sizeof(servers[0].ip)) {
      fprintf(stderr, "IP/hostname is too long: %s\n", ip);
      continue;
    }

    if (servers_num == capacity) {
      size_t new_capacity = capacity == 0 ? 4 : capacity * 2;
      struct Server *tmp =
          realloc(servers, new_capacity * sizeof(struct Server));
      if (tmp == NULL) {
        fprintf(stderr, "realloc failed\n");
        fclose(f);
        free(servers);
        return 1;
      }
      servers = tmp;
      capacity = new_capacity;
    }

    snprintf(servers[servers_num].ip, sizeof(servers[servers_num].ip), "%s",
             ip);
    servers[servers_num].port = (int)port;
    servers_num++;
  }

  fclose(f);
  if (servers_num == 0) {
    fprintf(stderr, "No servers found in %s\n", servers_path);
    free(servers);
    return 1;
  }



  //если к = 0, отвечаем сразу
  if (k == 0) {
    uint64_t answer = 1 % mod;
    printf("answer: %llu\n", (unsigned long long)answer);
    free(servers);
    return 0;
  }

  struct ClientTask *tasks =
      calloc(servers_num, sizeof(struct ClientTask));
  pthread_t *threads = malloc(servers_num * sizeof(pthread_t));

  if (tasks == NULL || threads == NULL) {
    fprintf(stderr, "Memory allocation failed\n");
    free(tasks);
    free(threads);
    free(servers);
    return 1;
  }



  //деление на блоки и запусск потоков
  uint64_t base = k / (uint64_t)servers_num;
  uint64_t rem = k % (uint64_t)servers_num;

  for (size_t i = 0; i < servers_num; i++) {
    uint64_t len = base + (i < rem ? 1 : 0);

    tasks[i].server = servers[i];
    tasks[i].mod = mod;
    tasks[i].result = 0;
    tasks[i].ok = 0;

    if (len == 0) {
      tasks[i].begin = 1;
      tasks[i].end = 0;
    } else {
      uint64_t start = 1 + (uint64_t)i * base + (i < rem ? i : rem);
      tasks[i].begin = start;
      tasks[i].end = start + len - 1;
    }

    if (pthread_create(&threads[i], NULL, ClientWorker, &tasks[i]) != 0) {
      fprintf(stderr, "pthread_create failed\n");
      free(tasks);
      free(threads);
      free(servers);
      return 1;
    }
  }


  //финальное перемножение
  for (size_t i = 0; i < servers_num; i++) {
    pthread_join(threads[i], NULL);
  }

  int all_ok = 1;
  for (size_t i = 0; i < servers_num; i++) {
    if (!tasks[i].ok) {
      all_ok = 0;
      break;
    }
  }

  if (!all_ok) {
    fprintf(stderr, "Some servers failed\n");
    free(tasks);
    free(threads);
    free(servers);
    return 1;
  }

  //вот уут финальное перемножение
  uint64_t answer = 1 % mod;
  for (size_t i = 0; i < servers_num; i++) {
    answer = MultModulo(answer, tasks[i].result, mod);
  }

  printf("answer: %llu\n", (unsigned long long)answer);

  free(tasks);
  free(threads);
  free(servers);

  return 0;
}