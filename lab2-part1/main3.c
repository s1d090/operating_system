#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>

#define MAX_ITERATIONS 30
#define MAX_SLEEP_TIME 10

void create_child_process() {
    // Seed random number generator
    srandom(time(NULL) ^ (getpid() << 16));

    // Generate a random number of iterations between 1 and MAX_ITERATIONS
    int iterations = random() % MAX_ITERATIONS + 1;

    for (int i = 0; i < iterations; i++) {
        // Child prints PID and that it's going to sleep
        printf("Child Pid: %d is going to sleep!\n", getpid());

        // Sleep for a random time between 1 and MAX_SLEEP_TIME seconds
        int sleep_time = random() % MAX_SLEEP_TIME + 1;
        sleep(sleep_time);

        // Child wakes up and prints its PID and PPID
        printf("Child Pid: %d is awake! Where is my Parent: %d?\n", getpid(), getppid());
    }

    // Child process terminates
    exit(0);
}

int main() {
    pid_t child1, child2;
    int status;

    // Fork to create the first child
    child1 = fork();

    if (child1 < 0) {
        // Error occurred during fork
        perror("Fork failed");
        exit(1);
    } else if (child1 == 0) {
        // Child 1 process
        create_child_process();
    } else {
        // Fork to create the second child
        child2 = fork();

        if (child2 < 0) {
            // Error occurred during fork
            perror("Fork failed");
            exit(1);
        } else if (child2 == 0) {
            // Child 2 process
            create_child_process();
        } else {
            // Parent process waits for both children to complete
            pid_t terminated_pid;

            terminated_pid = wait(&status);
            printf("Child Pid: %d has completed\n", terminated_pid);

            terminated_pid = wait(&status);
            printf("Child Pid: %d has completed\n", terminated_pid);

            printf("Parent process (Pid: %d) has finished waiting.\n", getpid());
        }
    }

    return 0;
}