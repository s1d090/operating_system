/* hello_signal.c */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

volatile sig_atomic_t flag = 0;

void handler(int signum)
{
    printf("Hello World!\n");
    flag = 1;
}

int main(int argc, char *argv[])
{
    signal(SIGALRM, handler); // Register handler for SIGALRM

    while (1) {
        alarm(5); // Set alarm for 5 seconds
        flag = 0; // Reset flag

        // Busy wait for the signal to be delivered
        while (!flag);

        printf("Turing was right!\n");
    }

    return 0;
}