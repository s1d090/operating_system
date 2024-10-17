#define _DEFAULT_SOURCE
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_COMMAND_LINE_LEN 1024
#define MAX_COMMAND_LINE_ARGS 128

extern char **environ;  // Access to environment variables

pid_t fg_process_pid = -1;  // Track the foreground process PID

// Function prototypes
bool is_builtin_command(char *cmd);
void execute_builtin_commands(char *arguments[]);
void execute_command(char *arguments[], bool is_background);

// Function to get the prompt with the current working directory
char *get_prompt() {
    static char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        strcat(cwd, "> ");
        return cwd;
    } else {
        perror("getcwd error");
        exit(1);
    }
}

// Signal handler for SIGINT (Ctrl-C) to prevent the shell from quitting
void handle_sigint(int sig) {
    printf("\nCaught Ctrl-C. Returning to shell prompt...\n");
    printf("%s", get_prompt());
    fflush(stdout);
}

// Signal handler for SIGALRM to kill long-running foreground processes
void handle_sigalrm(int sig) {
    if (fg_process_pid > 0) {
        printf("\nForeground process timed out. Killing PID: %d\n", fg_process_pid);
        kill(fg_process_pid, SIGKILL);
        fg_process_pid = -1;
    }
}

// Function to tokenize the input command line into arguments
void tokenize_command(char *command_line, char *arguments[]) {
    int arg_index = 0;
    char *token = strtok(command_line, " \t\r\n");
    while (token != NULL && arg_index < MAX_COMMAND_LINE_ARGS - 1) {
        arguments[arg_index++] = token;
        token = strtok(NULL, " \t\r\n");
    }
    arguments[arg_index] = NULL;
}

// Check if the command is a built-in command
bool is_builtin_command(char *cmd) {
    return strcmp(cmd, "cd") == 0 || strcmp(cmd, "pwd") == 0 ||
           strcmp(cmd, "echo") == 0 || strcmp(cmd, "env") == 0 ||
           strcmp(cmd, "setenv") == 0 || strcmp(cmd, "exit") == 0;
}

// Execute built-in commands
void execute_builtin_commands(char *arguments[]) {
    if (strcmp(arguments[0], "cd") == 0) {
        if (chdir(arguments[1]) != 0) perror("cd error");
    } else if (strcmp(arguments[0], "pwd") == 0) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) printf("%s\n", cwd);
        else perror("pwd error");
    } else if (strcmp(arguments[0], "echo") == 0) {
        for (int i = 1; arguments[i] != NULL; i++) {
            if (arguments[i][0] == '$') {
                char *env_var = getenv(arguments[i] + 1);
                printf("%s ", env_var ? env_var : "");
            } else {
                printf("%s ", arguments[i]);
            }
        }
        printf("\n");
    } else if (strcmp(arguments[0], "env") == 0) {
        for (int i = 0; environ[i] != NULL; i++) printf("%s\n", environ[i]);
    } else if (strcmp(arguments[0], "setenv") == 0) {
        if (arguments[1] && arguments[2]) {
            setenv(arguments[1], arguments[2], 1);
        } else {
            printf("Usage: setenv <VAR> <VALUE>\n");
        }
    } else if (strcmp(arguments[0], "exit") == 0) {
        exit(0);
    }
}

// Handle output redirection with `>`
void handle_output_redirection(char *arguments[]) {
    for (int i = 0; arguments[i] != NULL; i++) {
        if (strcmp(arguments[i], ">") == 0) {
            int fd = open(arguments[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("open error");
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            arguments[i] = NULL;
            break;
        }
    }
}

// Handle input redirection with `<`
void handle_input_redirection(char *arguments[]) {
    for (int i = 0; arguments[i] != NULL; i++) {
        if (strcmp(arguments[i], "<") == 0) {
            int fd = open(arguments[i + 1], O_RDONLY);
            if (fd < 0) {
                perror("open error");
                exit(1);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
            arguments[i] = NULL;
            break;
        }
    }
}

// Handle piping with `|`
void handle_pipe(char *arguments[]) {
    int i = 0;
    while (arguments[i] != NULL) {
        if (strcmp(arguments[i], "|") == 0) {
            arguments[i] = NULL;

            int pipefd[2];
            pipe(pipefd);

            if (fork() == 0) {
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[0]);
                execvp(arguments[0], arguments);
                perror("execvp error");
                exit(1);
            }

            if (fork() == 0) {
                dup2(pipefd[0], STDIN_FILENO);
                close(pipefd[1]);
                execvp(arguments[i + 1], &arguments[i + 1]);
                perror("execvp error");
                exit(1);
            }

            close(pipefd[0]);
            close(pipefd[1]);
            wait(NULL);
            wait(NULL);
            return;
        }
        i++;
    }
}

// Execute external commands
void execute_command(char *arguments[], bool is_background) {
    if (handle_pipe(arguments)) return;

    pid_t pid = fork();
    if (pid == 0) {
        signal(SIGINT, SIG_DFL);
        handle_output_redirection(arguments);
        handle_input_redirection(arguments);
        execvp(arguments[0], arguments);
        perror("execvp error");
        exit(1);
    } else {
        if (!is_background) {
            fg_process_pid = pid;
            alarm(10);
            waitpid(pid, NULL, 0);
            alarm(0);
            fg_process_pid = -1;
        } else {
            printf("Process running in background with PID: %d\n", pid);
        }
    }
}

// Main function to run the shell
int main() {
    char command_line[MAX_COMMAND_LINE_LEN];
    char *arguments[MAX_COMMAND_LINE_ARGS];

    signal(SIGINT, handle_sigint);
    signal(SIGALRM, handle_sigalrm);

    while (true) {
        printf("%s", get_prompt());
        fflush(stdout);

        if (fgets(command_line, MAX_COMMAND_LINE_LEN, stdin) == NULL) break;

        command_line[strlen(command_line) - 1] = '\0';
        tokenize_command(command_line, arguments);

        bool is_background = (strcmp(arguments[0], "&") == 0);
        if (is_builtin_command(arguments[0])) {
            execute_builtin_commands(arguments);
        } else {
            execute_command(arguments, is_background);
        }
    }
    return 0;
}


// task 5
// #define _DEFAULT_SOURCE  // Enables certain functions like setenv() on some systems
// #include <stdbool.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <sys/wait.h>
// #include <fcntl.h>
// #include <signal.h>

// #define MAX_COMMAND_LINE_LEN 1024
// #define MAX_COMMAND_LINE_ARGS 128

// extern char **environ;  // Access to environment variables

// pid_t fg_process_pid = -1;  // Track the foreground process PID

// // Function to get the current working directory and format the prompt
// char *get_prompt() {
//     static char cwd[1024];  // Buffer to hold the current directory
//     if (getcwd(cwd, sizeof(cwd)) != NULL) {
//         strcat(cwd, "> ");  // Add "> " to the prompt
//         return cwd;
//     } else {
//         perror("getcwd error");  // Print error if unable to get directory
//         exit(1);
//     }
// }

// // Signal handler for SIGINT (Ctrl-C) to prevent the shell from quitting
// void handle_sigint(int sig) {
//     printf("\nCaught Ctrl-C. Returning to shell prompt...\n");
//     printf("%s", get_prompt());  // Print the prompt again
//     fflush(stdout);
// }

// // Signal handler for SIGALRM to kill long-running foreground processes
// void handle_sigalrm(int sig) {
//     if (fg_process_pid > 0) {
//         printf("\nForeground process timed out. Killing PID: %d\n", fg_process_pid);
//         kill(fg_process_pid, SIGKILL);  // Kill the foreground process
//         fg_process_pid = -1;  // Reset the foreground process PID
//     }
// }

// // Function to tokenize the input command line into arguments
// void tokenize_command(char *command_line, char *arguments[]) {
//     int arg_index = 0;
//     char *token = strtok(command_line, " \t\r\n");  // Split by whitespace
//     while (token != NULL && arg_index < MAX_COMMAND_LINE_ARGS - 1) {
//         arguments[arg_index++] = token;  // Store tokens in arguments array
//         token = strtok(NULL, " \t\r\n");
//     }
//     arguments[arg_index] = NULL;  // Null-terminate the argument array
// }

// // Function to check if a command is a built-in command
// bool is_builtin_command(char *cmd) {
//     return strcmp(cmd, "cd") == 0 || strcmp(cmd, "pwd") == 0 ||
//            strcmp(cmd, "echo") == 0 || strcmp(cmd, "env") == 0 ||
//            strcmp(cmd, "setenv") == 0 || strcmp(cmd, "exit") == 0;
// }

// // Function to execute built-in commands
// void execute_builtin_commands(char *arguments[]) {
//     if (strcmp(arguments[0], "cd") == 0) {
//         if (chdir(arguments[1]) != 0) perror("cd error");
//     } 
//     else if (strcmp(arguments[0], "pwd") == 0) {
//         char cwd[1024];
//         if (getcwd(cwd, sizeof(cwd)) != NULL) printf("%s\n", cwd);
//         else perror("pwd error");
//     } 
//     else if (strcmp(arguments[0], "echo") == 0) {
//         for (int i = 1; arguments[i] != NULL; i++) {
//             if (arguments[i][0] == '$') {  // Handle environment variables
//                 char *env_var = getenv(arguments[i] + 1);
//                 printf("%s ", env_var ? env_var : "");
//             } else {
//                 printf("%s ", arguments[i]);
//             }
//         }
//         printf("\n");
//     } 
//     else if (strcmp(arguments[0], "env") == 0) {
//         for (int i = 0; environ[i] != NULL; i++) printf("%s\n", environ[i]);
//     } 
//     else if (strcmp(arguments[0], "setenv") == 0) {
//         if (arguments[1] && arguments[2]) {
//             setenv(arguments[1], arguments[2], 1);
//         } else {
//             printf("Usage: setenv <VAR> <VALUE>\n");
//         }
//     } 
//     else if (strcmp(arguments[0], "exit") == 0) {
//         exit(0);  // Terminate the shell
//     }
// }

// // Function to check if the command should run in the background
// bool is_background_command(char *arguments[], int *arg_count) {
//     if (*arg_count > 0 && strcmp(arguments[*arg_count - 1], "&") == 0) {
//         arguments[*arg_count - 1] = NULL;  // Remove the '&' from the argument list
//         (*arg_count)--;  // Reduce argument count since '&' is removed
//         return true;
//     }
//     return false;
// }

// // Function to execute external commands using child processes (with background support)
// void execute_command(char *arguments[], bool is_background) {
//     pid_t pid = fork();  // Create a child process
//     if (pid < 0) {
//         perror("fork error");
//         exit(1);
//     } 
//     else if (pid == 0) {
//         // Restore default behavior for SIGINT in the child process
//         signal(SIGINT, SIG_DFL);

//         // Child process executes the command
//         if (execvp(arguments[0], arguments) == -1) {
//             perror("execvp error");  // Error if command fails
//             printf("An error occurred.\n");
//         }
//         exit(1);  // Exit child process after execution
//     } 
//     else {
//         if (!is_background) {
//             fg_process_pid = pid;  // Track the foreground process PID
//             alarm(10);  // Set a 10-second timer
//             waitpid(pid, NULL, 0);  // Wait for the child process to complete
//             alarm(0);  // Cancel the alarm if the process finishes in time
//             fg_process_pid = -1;  // Reset the foreground process PID
//         } else {
//             printf("Process running in background with PID: %d\n", pid);
//         }
//     }
// }

// int main() {
//     char command_line[MAX_COMMAND_LINE_LEN];  // Buffer for user input
//     char *arguments[MAX_COMMAND_LINE_ARGS];   // Array for command arguments

//     // Set up signal handlers for SIGINT (Ctrl-C) and SIGALRM (timer)
//     signal(SIGINT, handle_sigint);
//     signal(SIGALRM, handle_sigalrm);

//     while (true) {
//         // Print the shell prompt with the current working directory
//         printf("%s", get_prompt());
//         fflush(stdout);

//         // Read input from the user
//         if (fgets(command_line, MAX_COMMAND_LINE_LEN, stdin) == NULL) {
//             if (ferror(stdin)) {
//                 perror("fgets error");
//                 exit(1);
//             }
//         }

//         // Remove the trailing newline character
//         command_line[strlen(command_line) - 1] = '\0';

//         // If EOF is detected, exit the shell
//         if (feof(stdin)) {
//             printf("\n");
//             exit(0);
//         }

//         // Tokenize the input into arguments
//         tokenize_command(command_line, arguments);

//         // Count the number of arguments
//         int arg_count = 0;
//         while (arguments[arg_count] != NULL) arg_count++;

//         // Check if the command should run in the background
//         bool is_background = is_background_command(arguments, &arg_count);

//         // Execute built-in commands or other commands
//         if (arguments[0] != NULL) {
//             if (is_builtin_command(arguments[0])) {
//                 execute_builtin_commands(arguments);
//             } else {
//                 execute_command(arguments, is_background);
//             }
//         }
//     }
//     return 0;  // This line should never be reached
// }


// task 4
// #define _DEFAULT_SOURCE  // Enables certain functions like setenv() on some systems
// #include <stdbool.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <sys/wait.h>
// #include <fcntl.h>
// #include <signal.h>

// #define MAX_COMMAND_LINE_LEN 1024
// #define MAX_COMMAND_LINE_ARGS 128

// extern char **environ;  // Access to environment variables

// // Function to get the current working directory and format the prompt
// char *get_prompt() {
//     static char cwd[1024];  // Buffer to hold the current directory
//     if (getcwd(cwd, sizeof(cwd)) != NULL) {
//         strcat(cwd, "> ");  // Add "> " to the prompt
//         return cwd;
//     } else {
//         perror("getcwd error");  // Print error if unable to get directory
//         exit(1);
//     }
// }

// // Function to handle SIGINT (Ctrl-C) signal in the shell process
// void handle_sigint(int sig) {
//     printf("\nCaught Ctrl-C. Returning to shell prompt...\n");
//     printf("%s", get_prompt());  // Print the prompt again
//     fflush(stdout);
// }

// // Function to tokenize the input command line into arguments
// void tokenize_command(char *command_line, char *arguments[]) {
//     int arg_index = 0;
//     char *token = strtok(command_line, " \t\r\n");  // Split by whitespace
//     while (token != NULL && arg_index < MAX_COMMAND_LINE_ARGS - 1) {
//         arguments[arg_index++] = token;  // Store tokens in arguments array
//         token = strtok(NULL, " \t\r\n");
//     }
//     arguments[arg_index] = NULL;  // Null-terminate the argument array
// }

// // Function to check if a command is a built-in command
// bool is_builtin_command(char *cmd) {
//     return strcmp(cmd, "cd") == 0 || strcmp(cmd, "pwd") == 0 ||
//            strcmp(cmd, "echo") == 0 || strcmp(cmd, "env") == 0 ||
//            strcmp(cmd, "setenv") == 0 || strcmp(cmd, "exit") == 0;
// }

// // Function to execute built-in commands
// void execute_builtin_commands(char *arguments[]) {
//     if (strcmp(arguments[0], "cd") == 0) {
//         if (chdir(arguments[1]) != 0) perror("cd error");
//     } 
//     else if (strcmp(arguments[0], "pwd") == 0) {
//         char cwd[1024];
//         if (getcwd(cwd, sizeof(cwd)) != NULL) printf("%s\n", cwd);
//         else perror("pwd error");
//     } 
//     else if (strcmp(arguments[0], "echo") == 0) {
//         for (int i = 1; arguments[i] != NULL; i++) {
//             if (arguments[i][0] == '$') {  // Handle environment variables
//                 char *env_var = getenv(arguments[i] + 1);
//                 printf("%s ", env_var ? env_var : "");
//             } else {
//                 printf("%s ", arguments[i]);
//             }
//         }
//         printf("\n");
//     } 
//     else if (strcmp(arguments[0], "env") == 0) {
//         for (int i = 0; environ[i] != NULL; i++) printf("%s\n", environ[i]);
//     } 
//     else if (strcmp(arguments[0], "setenv") == 0) {
//         if (arguments[1] && arguments[2]) {
//             setenv(arguments[1], arguments[2], 1);
//         } else {
//             printf("Usage: setenv <VAR> <VALUE>\n");
//         }
//     } 
//     else if (strcmp(arguments[0], "exit") == 0) {
//         exit(0);  // Terminate the shell
//     }
// }

// // Function to check if the command should run in the background
// bool is_background_command(char *arguments[], int *arg_count) {
//     if (*arg_count > 0 && strcmp(arguments[*arg_count - 1], "&") == 0) {
//         arguments[*arg_count - 1] = NULL;  // Remove the '&' from the argument list
//         (*arg_count)--;  // Reduce argument count since '&' is removed
//         return true;
//     }
//     return false;
// }

// // Function to execute external commands using child processes (with background support)
// void execute_command(char *arguments[], bool is_background) {
//     pid_t pid = fork();  // Create a child process
//     if (pid < 0) {
//         perror("fork error");
//         exit(1);
//     } 
//     else if (pid == 0) {
//         // Restore default behavior for SIGINT in the child process
//         signal(SIGINT, SIG_DFL);

//         // Child process executes the command
//         if (execvp(arguments[0], arguments) == -1) {
//             perror("execvp error");  // Error if command fails
//             printf("An error occurred.\n");
//         }
//         exit(1);  // Exit child process after execution
//     } 
//     else {
//         if (!is_background) {
//             // Parent process waits for the child to complete unless it's a background process
//             wait(NULL);
//         } else {
//             printf("Process running in background with PID: %d\n", pid);
//         }
//     }
// }

// int main() {
//     char command_line[MAX_COMMAND_LINE_LEN];  // Buffer for user input
//     char *arguments[MAX_COMMAND_LINE_ARGS];   // Array for command arguments

//     // Set up the signal handler for SIGINT (Ctrl-C)
//     signal(SIGINT, handle_sigint);

//     while (true) {
//         // Print the shell prompt with the current working directory
//         printf("%s", get_prompt());
//         fflush(stdout);

//         // Read input from the user
//         if (fgets(command_line, MAX_COMMAND_LINE_LEN, stdin) == NULL) {
//             if (ferror(stdin)) {
//                 perror("fgets error");
//                 exit(1);
//             }
//         }

//         // Remove the trailing newline character
//         command_line[strlen(command_line) - 1] = '\0';

//         // If EOF is detected, exit the shell
//         if (feof(stdin)) {
//             printf("\n");
//             exit(0);
//         }

//         // Tokenize the input into arguments
//         tokenize_command(command_line, arguments);

//         // Count the number of arguments
//         int arg_count = 0;
//         while (arguments[arg_count] != NULL) arg_count++;

//         // Check if the command should run in the background
//         bool is_background = is_background_command(arguments, &arg_count);

//         // Execute built-in commands or other commands
//         if (arguments[0] != NULL) {
//             if (is_builtin_command(arguments[0])) {
//                 execute_builtin_commands(arguments);
//             } else {
//                 execute_command(arguments, is_background);
//             }
//         }
//     }
//     return 0;  // This line should never be reached
// }


// Task 3
// #define _DEFAULT_SOURCE  // Enables certain functions like setenv() on some systems
// #include <stdbool.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <sys/wait.h>
// #include <fcntl.h>
// #include <signal.h>

// #define MAX_COMMAND_LINE_LEN 1024
// #define MAX_COMMAND_LINE_ARGS 128

// extern char **environ;  // Access to environment variables

// // Function to get the current working directory and format the prompt
// char *get_prompt() {
//     static char cwd[1024];  // Buffer to hold the current directory
//     if (getcwd(cwd, sizeof(cwd)) != NULL) {
//         strcat(cwd, "> ");  // Add "> " to the prompt
//         return cwd;
//     } else {
//         perror("getcwd error");  // Print error if unable to get directory
//         exit(1);
//     }
// }

// // Function to tokenize the input command line into arguments
// void tokenize_command(char *command_line, char *arguments[]) {
//     int arg_index = 0;
//     char *token = strtok(command_line, " \t\r\n");  // Split by whitespace
//     while (token != NULL && arg_index < MAX_COMMAND_LINE_ARGS - 1) {
//         arguments[arg_index++] = token;  // Store tokens in arguments array
//         token = strtok(NULL, " \t\r\n");
//     }
//     arguments[arg_index] = NULL;  // Null-terminate the argument array
// }

// // Function to check if a command is a built-in command
// bool is_builtin_command(char *cmd) {
//     return strcmp(cmd, "cd") == 0 || strcmp(cmd, "pwd") == 0 ||
//            strcmp(cmd, "echo") == 0 || strcmp(cmd, "env") == 0 ||
//            strcmp(cmd, "setenv") == 0 || strcmp(cmd, "exit") == 0;
// }

// // Function to execute built-in commands
// void execute_builtin_commands(char *arguments[]) {
//     if (strcmp(arguments[0], "cd") == 0) {
//         if (chdir(arguments[1]) != 0) perror("cd error");
//     } 
//     else if (strcmp(arguments[0], "pwd") == 0) {
//         char cwd[1024];
//         if (getcwd(cwd, sizeof(cwd)) != NULL) printf("%s\n", cwd);
//         else perror("pwd error");
//     } 
//     else if (strcmp(arguments[0], "echo") == 0) {
//         for (int i = 1; arguments[i] != NULL; i++) {
//             if (arguments[i][0] == '$') {  // Handle environment variables
//                 char *env_var = getenv(arguments[i] + 1);
//                 printf("%s ", env_var ? env_var : "");
//             } else {
//                 printf("%s ", arguments[i]);
//             }
//         }
//         printf("\n");
//     } 
//     else if (strcmp(arguments[0], "env") == 0) {
//         for (int i = 0; environ[i] != NULL; i++) printf("%s\n", environ[i]);
//     } 
//     else if (strcmp(arguments[0], "setenv") == 0) {
//         if (arguments[1] && arguments[2]) {
//             setenv(arguments[1], arguments[2], 1);
//         } else {
//             printf("Usage: setenv <VAR> <VALUE>\n");
//         }
//     } 
//     else if (strcmp(arguments[0], "exit") == 0) {
//         exit(0);  // Terminate the shell
//     }
// }

// // Function to check if the command should run in the background
// bool is_background_command(char *arguments[], int *arg_count) {
//     if (*arg_count > 0 && strcmp(arguments[*arg_count - 1], "&") == 0) {
//         arguments[*arg_count - 1] = NULL;  // Remove the '&' from the argument list
//         (*arg_count)--;  // Reduce argument count since '&' is removed
//         return true;
//     }
//     return false;
// }

// // Function to execute external commands using child processes (with background support)
// void execute_command(char *arguments[], bool is_background) {
//     pid_t pid = fork();  // Create a child process
//     if (pid < 0) {
//         perror("fork error");
//         exit(1);
//     } 
//     else if (pid == 0) {
//         // Child process executes the command
//         if (execvp(arguments[0], arguments) == -1) {
//             perror("execvp error");  // Error if command fails
//             printf("An error occurred.\n");
//         }
//         exit(1);  // Exit child process after execution
//     } 
//     else {
//         if (!is_background) {
//             // Parent process waits for the child to complete unless it's a background process
//             wait(NULL);
//         } else {
//             printf("Process running in background with PID: %d\n", pid);
//         }
//     }
// }

// int main() {
//     char command_line[MAX_COMMAND_LINE_LEN];  // Buffer for user input
//     char *arguments[MAX_COMMAND_LINE_ARGS];   // Array for command arguments

//     while (true) {
//         // Print the shell prompt with the current working directory
//         printf("%s", get_prompt());
//         fflush(stdout);

//         // Read input from the user
//         if (fgets(command_line, MAX_COMMAND_LINE_LEN, stdin) == NULL) {
//             if (ferror(stdin)) {
//                 perror("fgets error");
//                 exit(1);
//             }
//         }

//         // Remove the trailing newline character
//         command_line[strlen(command_line) - 1] = '\0';

//         // If EOF is detected, exit the shell
//         if (feof(stdin)) {
//             printf("\n");
//             exit(0);
//         }

//         // Tokenize the input into arguments
//         tokenize_command(command_line, arguments);

//         // Count the number of arguments
//         int arg_count = 0;
//         while (arguments[arg_count] != NULL) arg_count++;

//         // Check if the command should run in the background
//         bool is_background = is_background_command(arguments, &arg_count);

//         // Execute built-in commands or other commands
//         if (arguments[0] != NULL) {
//             if (is_builtin_command(arguments[0])) {
//                 execute_builtin_commands(arguments);
//             } else {
//                 execute_command(arguments, is_background);
//             }
//         }
//     }
//     return 0;  // This line should never be reached
// }

// Task 2
// #define _DEFAULT_SOURCE  // Enables certain functions like setenv() on some systems
// #include <stdbool.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <sys/wait.h>
// #include <fcntl.h>
// #include <signal.h>

// #define MAX_COMMAND_LINE_LEN 1024
// #define MAX_COMMAND_LINE_ARGS 128

// extern char **environ;  // Access to environment variables

// // Function to get the current working directory and format the prompt
// char *get_prompt() {
//     static char cwd[1024];  // Buffer to hold the current directory
//     if (getcwd(cwd, sizeof(cwd)) != NULL) {
//         strcat(cwd, "> ");  // Add "> " to the prompt
//         return cwd;
//     } else {
//         perror("getcwd error");  // Print error if unable to get directory
//         exit(1);
//     }
// }

// // Function to tokenize the input command line into arguments
// void tokenize_command(char *command_line, char *arguments[]) {
//     int arg_index = 0;
//     char *token = strtok(command_line, " \t\r\n");  // Split by whitespace
//     while (token != NULL && arg_index < MAX_COMMAND_LINE_ARGS - 1) {
//         arguments[arg_index++] = token;  // Store tokens in arguments array
//         token = strtok(NULL, " \t\r\n");
//     }
//     arguments[arg_index] = NULL;  // Null-terminate the argument array
// }

// // Function to check if a command is a built-in command
// bool is_builtin_command(char *cmd) {
//     return strcmp(cmd, "cd") == 0 || strcmp(cmd, "pwd") == 0 ||
//            strcmp(cmd, "echo") == 0 || strcmp(cmd, "env") == 0 ||
//            strcmp(cmd, "setenv") == 0 || strcmp(cmd, "exit") == 0;
// }

// // Function to execute built-in commands
// void execute_builtin_commands(char *arguments[]) {
//     if (strcmp(arguments[0], "cd") == 0) {
//         if (chdir(arguments[1]) != 0) perror("cd error");
//     } 
//     else if (strcmp(arguments[0], "pwd") == 0) {
//         char cwd[1024];
//         if (getcwd(cwd, sizeof(cwd)) != NULL) printf("%s\n", cwd);
//         else perror("pwd error");
//     } 
//     else if (strcmp(arguments[0], "echo") == 0) {
//         for (int i = 1; arguments[i] != NULL; i++) {
//             if (arguments[i][0] == '$') {  // Handle environment variables
//                 char *env_var = getenv(arguments[i] + 1);
//                 printf("%s ", env_var ? env_var : "");
//             } else {
//                 printf("%s ", arguments[i]);
//             }
//         }
//         printf("\n");
//     } 
//     else if (strcmp(arguments[0], "env") == 0) {
//         for (int i = 0; environ[i] != NULL; i++) printf("%s\n", environ[i]);
//     } 
//     else if (strcmp(arguments[0], "setenv") == 0) {
//         if (arguments[1] && arguments[2]) {
//             setenv(arguments[1], arguments[2], 1);
//         } else {
//             printf("Usage: setenv <VAR> <VALUE>\n");
//         }
//     } 
//     else if (strcmp(arguments[0], "exit") == 0) {
//         exit(0);  // Terminate the shell
//     }
// }

// // Function to execute external commands using child processes
// void execute_command(char *arguments[]) {
//     pid_t pid = fork();  // Create a child process
//     if (pid < 0) {
//         perror("fork error");
//         exit(1);
//     } 
//     else if (pid == 0) {
//         // Child process executes the command
//         if (execvp(arguments[0], arguments) == -1) {
//             perror("execvp error");  // Error if command fails
//             printf("An error occurred.\n");
//         }
//         exit(1);  // Exit child process after execution
//     } 
//     else {
//         // Parent process waits for the child to complete
//         wait(NULL);
//     }
// }

// int main() {
//     char command_line[MAX_COMMAND_LINE_LEN];  // Buffer for user input
//     char *arguments[MAX_COMMAND_LINE_ARGS];   // Array for command arguments

//     while (true) {
//         // Print the shell prompt with the current working directory
//         printf("%s", get_prompt());
//         fflush(stdout);

//         // Read input from the user
//         if (fgets(command_line, MAX_COMMAND_LINE_LEN, stdin) == NULL) {
//             if (ferror(stdin)) {
//                 perror("fgets error");
//                 exit(1);
//             }
//         }

//         // Remove the trailing newline character
//         command_line[strlen(command_line) - 1] = '\0';

//         // If EOF is detected, exit the shell
//         if (feof(stdin)) {
//             printf("\n");
//             exit(0);
//         }

//         // Tokenize the input into arguments
//         tokenize_command(command_line, arguments);

//         // Execute built-in commands or other commands
//         if (arguments[0] != NULL) {
//             if (is_builtin_command(arguments[0])) {
//                 execute_builtin_commands(arguments);
//             } else {
//                 execute_command(arguments);
//             }
//         }
//     }
//     return 0;  // This line should never be reached
// }


// Task 1

// #define _DEFAULT_SOURCE  // Enables certain functions like setenv() on some systems
// #include <stdbool.h>
// #include <stdio.h>
// #include <stdlib.h>  // Required for setenv()
// #include <string.h>
// #include <unistd.h>
// #include <sys/wait.h>
// #include <fcntl.h>
// #include <signal.h>

// #define MAX_COMMAND_LINE_LEN 1024
// #define MAX_COMMAND_LINE_ARGS 128

// extern char **environ;  // Access to environment variables

// // Function to get the current working directory and format the prompt
// char *get_prompt() {
//     static char cwd[1024];  // Buffer to hold the current directory
//     if (getcwd(cwd, sizeof(cwd)) != NULL) {
//         strcat(cwd, "> ");  // Add "> " to the prompt
//         return cwd;
//     } else {
//         perror("getcwd error");  // Print error if unable to get directory
//         exit(1);
//     }
// }

// // Function to tokenize the input command line into arguments
// void tokenize_command(char *command_line, char *arguments[]) {
//     int arg_index = 0;
//     char *token = strtok(command_line, " \t\r\n");  // Split by whitespace
//     while (token != NULL && arg_index < MAX_COMMAND_LINE_ARGS - 1) {
//         arguments[arg_index++] = token;  // Store tokens in arguments array
//         token = strtok(NULL, " \t\r\n");
//     }
//     arguments[arg_index] = NULL;  // Null-terminate the argument array
// }

// // Function to check if a command is a built-in command
// bool is_builtin_command(char *cmd) {
//     return strcmp(cmd, "cd") == 0 || strcmp(cmd, "pwd") == 0 ||
//            strcmp(cmd, "echo") == 0 || strcmp(cmd, "env") == 0 ||
//            strcmp(cmd, "setenv") == 0 || strcmp(cmd, "exit") == 0;
// }

// // Function to execute built-in commands
// void execute_builtin_commands(char *arguments[]) {
//     if (strcmp(arguments[0], "cd") == 0) {
//         if (chdir(arguments[1]) != 0) perror("cd error");
//     } 
//     else if (strcmp(arguments[0], "pwd") == 0) {
//         char cwd[1024];
//         if (getcwd(cwd, sizeof(cwd)) != NULL) printf("%s\n", cwd);
//         else perror("pwd error");
//     } 
//     else if (strcmp(arguments[0], "echo") == 0) {
//         for (int i = 1; arguments[i] != NULL; i++) {
//             if (arguments[i][0] == '$') {  // Handle environment variables
//                 char *env_var = getenv(arguments[i] + 1);
//                 printf("%s ", env_var ? env_var : "");
//             } else {
//                 printf("%s ", arguments[i]);
//             }
//         }
//         printf("\n");
//     } 
//     else if (strcmp(arguments[0], "env") == 0) {
//         for (int i = 0; environ[i] != NULL; i++) printf("%s\n", environ[i]);
//     } 
//     else if (strcmp(arguments[0], "setenv") == 0) {
//         if (arguments[1] && arguments[2]) {
//             setenv(arguments[1], arguments[2], 1);
//         } else {
//             printf("Usage: setenv <VAR> <VALUE>\n");
//         }
//     } 
//     else if (strcmp(arguments[0], "exit") == 0) {
//         exit(0);  // Terminate the shell
//     }
// }

// // Function to execute external commands using child processes
// void execute_command(char *arguments[]) {
//     pid_t pid = fork();  // Create a child process
//     if (pid < 0) {
//         perror("fork error");
//         exit(1);
//     } 
//     else if (pid == 0) {
//         // Child process executes the command
//         if (execvp(arguments[0], arguments) == -1) {
//             perror("execvp error");  // Error if command fails
//         }
//         exit(1);  // Exit child process after execution
//     } 
//     else {
//         // Parent process waits for child to complete
//         wait(NULL);
//     }
// }

// int main() {
//     char command_line[MAX_COMMAND_LINE_LEN];  // Buffer for user input
//     char *arguments[MAX_COMMAND_LINE_ARGS];   // Array for command arguments

//     while (true) {
//         // Print the shell prompt with the current working directory
//         printf("%s", get_prompt());
//         fflush(stdout);

//         // Read input from the user
//         if (fgets(command_line, MAX_COMMAND_LINE_LEN, stdin) == NULL) {
//             if (ferror(stdin)) {
//                 perror("fgets error");
//                 exit(1);
//             }
//         }

//         // Remove the trailing newline character
//         command_line[strlen(command_line) - 1] = '\0';

//         // If EOF is detected, exit the shell
//         if (feof(stdin)) {
//             printf("\n");
//             exit(0);
//         }

//         // Tokenize the input into arguments
//         tokenize_command(command_line, arguments);

//         // Execute built-in commands or other commands
//         if (arguments[0] != NULL) {
//             if (is_builtin_command(arguments[0])) {
//                 execute_builtin_commands(arguments);
//             } else {
//                 execute_command(arguments);
//             }
//         }
//     }
//     return 0;  // This line should never be reached
// }
