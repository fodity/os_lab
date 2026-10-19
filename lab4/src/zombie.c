#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();
    if (pid == 0) {
        // ребенок завершается и становится зомби, так как родитель пока не вызвал wait
        printf("Child (PID %d) finished. Becomes a zombie.\n", getpid());
        exit(0);
    } else if (pid > 0) {
        // родитель спит 30 секунд, не забирая статус ребенка
        printf("Parent (PID %d) sleeping 30s. Check 'ps aux | grep Z'.\n", getpid());
        sleep(30);
        
        // спустя 30 секунд родитель просыпается и убирает зомби
        wait(NULL);
        printf("Parent finished waiting. Zombie is gone.\n");
    }
    return 0;
}