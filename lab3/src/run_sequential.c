#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
    pid_t pid = fork();

    if (pid == 0) {
        execl("./sequential_min_max", "sequential_min_max", "42", "100", NULL);
        perror("exec failed");
        return 1;
    } else if (pid > 0) {
        wait(NULL);
        printf("Child finished\n");
    } else {
        perror("fork failed");
        return 1;
    }

    return 0;
}