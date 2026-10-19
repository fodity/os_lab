#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <getopt.h>
#include <sys/time.h>

#include "utils.h"
#include "sum.h"

struct SumArgs {
  int *array;
  int begin;
  int end;
};

void *ThreadSum(void *args) {
  struct SumArgs *sum_args = (struct SumArgs *)args;
  long long *result = malloc(sizeof(long long));
  *result = CalculateSum(sum_args->array, sum_args->begin, sum_args->end);
  return (void *)result;
}

int main(int argc, char **argv) {
  uint32_t threads_num = 0;
  uint32_t array_size = 0;
  uint32_t seed = 0;

  while (true) {
      static struct option options[] = {
          {"threads_num", required_argument, 0, 0},
          {"array_size", required_argument, 0, 0},
          {"seed", required_argument, 0, 0},
          {0, 0, 0, 0}
      };
      int option_index = 0;
      int c = getopt_long(argc, argv, "", options, &option_index);
      if (c == -1) break;
      switch (c) {
          case 0:
              switch (option_index) {
                  case 0: threads_num = atoi(optarg); break;
                  case 1: array_size = atoi(optarg); break;
                  case 2: seed = atoi(optarg); break;
              }
              break;
      }
  }

  if (threads_num <= 0 || array_size <= 0 || seed <= 0) {
      printf("Usage: %s --threads_num \"num\" --seed \"num\" --array_size \"num\"\n", argv[0]);
      return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);

  pthread_t *threads = malloc(sizeof(pthread_t) * threads_num);
  struct SumArgs *args = malloc(sizeof(struct SumArgs) * threads_num);

  struct timeval start_time, finish_time;
  gettimeofday(&start_time, NULL);

  for (uint32_t i = 0; i < threads_num; i++) {
    args[i].array = array;
    args[i].begin = i * array_size / threads_num;
    args[i].end = (i + 1) * array_size / threads_num;
    if (pthread_create(&threads[i], NULL, ThreadSum, (void *)&args[i])) {
      printf("Error: pthread_create failed!\n");
      return 1;
    }
  }

  long long total_sum = 0;
  for (uint32_t i = 0; i < threads_num; i++) {
    void *result;
    pthread_join(threads[i], &result);
    total_sum += *(long long *)result;
    free(result);
  }

  gettimeofday(&finish_time, NULL);
  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  printf("Total: %lld\n", total_sum);
  printf("Elapsed time: %f ms\n", elapsed_time);

  free(array);
  free(threads);
  free(args);
  return 0;
}