#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

volatile sig_atomic_t flag = 0;
time_t start_time;

void handle_alarm(int signum)
{
    printf("Hello World!\n");
    flag = 1;
}

void handle_sigint(int signum)
{
    time_t end_time;
    time(&end_time);
    printf("\nTotal system time: %ld seconds\n", end_time - start_time);
    exit(0);
}

int main(int argc, char *argv[])
{
    time(&start_time); // Record start time

    signal(SIGALRM, handle_alarm); // Register handler for SIGALRM
    signal(SIGINT, handle_sigint); // Register handler for SIGINT

    while (1) {
        alarm(1); // Set an alarm for every 1 second
        flag = 0; // Reset flag

        // Busy wait until the alarm signal is received
        while (!flag);
    }

    return 0;
}