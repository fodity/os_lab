#include "common.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


//функция для подсчета модульного факториала
uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod) {
  if (mod == 0) {
    return 0;
  }

  uint64_t result = 0;
  a = a % mod;
  while (b > 0) {
    if (b % 2 == 1)
      result = (result + a) % mod;
    a = (a * 2) % mod;
    b /= 2;
  }

  return result % mod;
}


//конвертация строки в 64-ти ричное число
bool ConvertStringToUI64(const char *str, uint64_t *val) {
  char *end = NULL;
  errno = 0;
  unsigned long long i = strtoull(str, &end, 10);

  if (errno == ERANGE) {
    fprintf(stderr, "Out of uint64_t range: %s\n", str);
    return false;
  }

  if (errno != 0 || end == str || *end != '\0') {
    return false;
  }

  *val = (uint64_t)i;
  return true;
}


//сенд алл может отправить не все 24 байта, а меньше, чем len. это нормально. например
//если буфер сокета переполнен
int SendAll(int fd, const void *buf, size_t len) {
  const char *p = (const char *)buf;

  while (len > 0) {
    ssize_t n = send(fd, p, len, 0);
    if (n < 0) {
      if (errno == EINTR)
        continue;
      return -1;
    }
    if (n == 0)
      return -1;

    p += n;
    len -= (size_t)n;
  }

  return 0;
}


//функция приема 
// 1 все гуд, 0 соед. закрыто до того как начали читать, -1 ошибка
int RecvAll(int fd, void *buf, size_t len) {
  char *p = (char *)buf;
  size_t left = len;

  while (left > 0) {
    ssize_t n = recv(fd, p, left, 0);
    if (n == 0) {
      return (left == len) ? 0 : -1; //клиент закрыл соединение
    }
    if (n < 0) {
      if (errno == EINTR)
        continue;
      return -1;
    }

    p += n;
    left -= (size_t)n;
  }

  return 1;
}