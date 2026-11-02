#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <getopt.h>

long long result = 1;
int k_global, pnum_global, mod_global;
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

void *thread_func(void *arg) {
    int thread_idx = *(int *)arg;
    
    // Вычисляем границы для каждого потока
    int begin = thread_idx * k_global / pnum_global + 1;
    int end = (thread_idx + 1) * k_global / pnum_global;
    
    long long local_res = 1;
    for (int i = begin; i <= end; i++) {
        local_res = (local_res * i) % mod_global;
    }
    
    // Критическая секция: обновляем глобальный результат под мьютексом
    pthread_mutex_lock(&mut);
    result = (result * local_res) % mod_global;
    pthread_mutex_unlock(&mut);
    
    return NULL;
}

int main(int argc, char **argv) {
    int k = -1, pnum = -1, mod = -1;
    
    while (1) {
        static struct option options[] = {
            {"k", required_argument, 0, 'k'},
            {"pnum", required_argument, 0, 'p'},
            {"mod", required_argument, 0, 'm'},
            {0, 0, 0, 0}
        };
        int option_index = 0;
        int c = getopt_long(argc, argv, "k:p:m:", options, &option_index);
        if (c == -1) break;
        switch (c) {
            case 'k': k = atoi(optarg); break;
            case 'p': pnum = atoi(optarg); break;
            case 'm': mod = atoi(optarg); break;
        }
    }
    
    if (k <= 0 || pnum <= 0 || mod <= 0) {
        printf("Usage: %s -k <num> --pnum=<num> --mod=<num>\n", argv[0]);
        return 1;
    }

    k_global = k; pnum_global = pnum; mod_global = mod;
    pthread_t *threads = malloc(sizeof(pthread_t) * pnum);
    int *thread_ids = malloc(sizeof(int) * pnum);

    for (int i = 0; i < pnum; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, thread_func, &thread_ids[i]);
    }
    for (int i = 0; i < pnum; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Factorial of %d mod %d is: %lld\n", k, mod, result);
    
    free(threads);
    free(thread_ids);
    return 0;
}