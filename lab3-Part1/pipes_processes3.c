#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <grep-argument>\n", argv[0]);
        exit(1);
    }

    int pipefd1[2]; // Pipe between cat and grep
    int pipefd2[2]; // Pipe between grep and sort
    int pid1, pid2;

    // Create the first pipe
    pipe(pipefd1);

    // Fork first child to handle "grep <argument>"
    pid1 = fork();

    if (pid1 == 0)
    {
        // Inside the first child (P2): Execute grep <argument>

        // Create second pipe
        pipe(pipefd2);

        pid2 = fork();
        if (pid2 == 0)
        {
            // Inside second child (P3): Execute sort

            // Replace standard input with input part of pipe 2
            dup2(pipefd2[0], 0);

            // Close unused pipe ends
            close(pipefd2[1]);
            close(pipefd1[0]);
            close(pipefd1[1]);

            // Execute sort
            char *sort_args[] = {"sort", NULL};
            execvp("sort", sort_args);
        }
        else
        {
            // Inside first child (P2): Execute grep <argument>

            // Replace standard input with input part of pipe 1
            dup2(pipefd1[0], 0);

            // Replace standard output with output part of pipe 2
            dup2(pipefd2[1], 1);

            // Close unused pipe ends
            close(pipefd1[1]);
            close(pipefd2[0]);
            close(pipefd2[1]);

            // Execute grep <argument>
            char *grep_args[] = {"grep", argv[1], NULL};
            execvp("grep", grep_args);
        }
    }
    else
    {
        // Inside parent (P1): Execute cat scores

        // Replace standard output with output part of pipe 1
        dup2(pipefd1[1], 1);

        // Close unused pipe ends
        close(pipefd1[0]);
        close(pipefd1[1]);

        // Execute cat scores
        char *cat_args[] = {"cat", "scores", NULL};
        execvp("cat", cat_args);
    }

    return 0;
}