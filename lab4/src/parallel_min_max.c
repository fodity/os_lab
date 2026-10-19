#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <getopt.h>
#include "find_min_max.h"
#include "utils.h"

pid_t *child_pids;
int pnum_global;

void timeout_handler(int sig) {
    (void)sig;
    for (int i = 0; i < pnum_global; i++) {
        if (child_pids[i] > 0) kill(child_pids[i], SIGKILL);
    }
}

int main(int argc, char **argv) {
  int seed = -1, array_size = -1, pnum = -1, timeout = -1;
  bool with_files = false;

  while (true) {
    static struct option options[] = {
        {"seed", required_argument, 0, 0},
        {"array_size", required_argument, 0, 0},
        {"pnum", required_argument, 0, 0},
        {"timeout", required_argument, 0, 0},
        {"by_files", no_argument, 0, 'f'},
        {0, 0, 0, 0}
    };
    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);
    if (c == -1) break;
    switch (c) {
      case 0:
        switch (option_index) {
          case 0: seed = atoi(optarg); break;
          case 1: array_size = atoi(optarg); break;
          case 2: pnum = atoi(optarg); break;
          case 3: timeout = atoi(optarg); break;
        }
        break;
      case 'f': with_files = true; break;
    }
  }

  if (seed == -1 || array_size == -1 || pnum == -1) return 1;

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  child_pids = malloc(sizeof(pid_t) * pnum);
  pnum_global = pnum;

  if (timeout > 0) {
    signal(SIGALRM, timeout_handler);
    alarm(timeout);
  }

  int active_child_processes = 0;
  struct timeval start_time, finish_time;
  gettimeofday(&start_time, NULL);

  int (*pipes)[2] = NULL;
  if (!with_files) {
    pipes = malloc(sizeof(int[2]) * pnum);
    for (int i = 0; i < pnum; i++) pipe(pipes[i]);
  }

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      active_child_processes += 1;
      if (child_pid == 0) {
        int begin = i * array_size / pnum;
        int end = (i + 1) * array_size / pnum;
        struct MinMax local = GetMinMax(array, begin, end);

        if (with_files) {
          char filename[64];
          snprintf(filename, sizeof(filename), "minmax_%d.txt", i);
          FILE *f = fopen(filename, "w");
          fprintf(f, "%d %d\n", local.min, local.max);
          fclose(f);
        } else {
          close(pipes[i][0]);
          write(pipes[i][1], &local.min, sizeof(int));
          write(pipes[i][1], &local.max, sizeof(int));
          close(pipes[i][1]);
        }
        return 0;
      }
      child_pids[i] = child_pid;
      if (!with_files) close(pipes[i][1]);
    }
  }

  while (active_child_processes > 0) {
    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG);
    if (pid > 0) active_child_processes -= 1;
    else if (pid == 0) usleep(10000);
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX, max = INT_MIN;
    if (with_files) {
      char filename[64];
      snprintf(filename, sizeof(filename), "minmax_%d.txt", i);
      FILE *f = fopen(filename, "r");
      if (f) {
        fscanf(f, "%d %d", &min, &max);
        fclose(f);
        remove(filename);
      }
    } else {
      if (read(pipes[i][0], &min, sizeof(int)) <= 0) continue;
      read(pipes[i][0], &max, sizeof(int));
      close(pipes[i][0]);
    }
    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  if (pipes) free(pipes);
  free(child_pids);
  free(array);

  gettimeofday(&finish_time, NULL);
  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  return 0;
}