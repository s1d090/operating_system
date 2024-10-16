// C program to demonstrate two-way communication between processes using pipes 
#include<stdio.h> 
#include<stdlib.h> 
#include<unistd.h> 
#include<sys/types.h> 
#include<string.h> 
#include<sys/wait.h> 
  
int main() 
{ 
    int fd1[2];  // Pipe 1 to send input string from parent to child 
    int fd2[2];  // Pipe 2 to send concatenated string from child to parent 
  
    char fixed_str_child[] = "howard.edu"; 
    char fixed_str_parent[] = "gobison.org"; 
    char input_str[100]; 
    char input_str_child[100]; 
    pid_t p; 
  
    if (pipe(fd1) == -1) 
    { 
        fprintf(stderr, "Pipe Failed" ); 
        return 1; 
    } 
    if (pipe(fd2) == -1) 
    { 
        fprintf(stderr, "Pipe Failed" ); 
        return 1; 
    } 
  
    printf("Enter a string to concatenate: ");
    scanf("%s", input_str); 
  
    p = fork(); 
  
    if (p < 0) 
    { 
        fprintf(stderr, "fork Failed" ); 
        return 1; 
    } 
  
    // Parent process (P1)
    else if (p > 0) 
    { 
        close(fd1[0]);  // Close reading end of first pipe (parent writes)
        close(fd2[1]);  // Close writing end of second pipe (parent reads)
  
        // Write input string to the child and close writing end of first pipe
        write(fd1[1], input_str, strlen(input_str) + 1); 
        close(fd1[1]);
  
        // Wait for child to finish concatenating
        wait(NULL); 
  
        // Read concatenated string from child
        char concat_str_child[200];
        read(fd2[0], concat_str_child, 200);
        printf("Concatenated string from child: %s\n", concat_str_child);

        // Parent now concatenates "gobison.org" to the string received from child
        strcat(concat_str_child, fixed_str_parent);
        printf("Concatenated string with parent suffix: %s\n", concat_str_child);
  
        close(fd2[0]);  // Close reading end of second pipe
    } 
  
    // Child process (P2)
    else
    { 
        close(fd1[1]);  // Close writing end of first pipe (child reads)
        close(fd2[0]);  // Close reading end of second pipe (child writes)
  
        // Read string sent by parent
        char concat_str[100]; 
        read(fd1[0], concat_str, 100); 
        close(fd1[0]);
  
        // Concatenate "howard.edu"
        strcat(concat_str, fixed_str_child);
        printf("Concatenated string in child: %s\n", concat_str);
  
        // Send concatenated string back to parent
        write(fd2[1], concat_str, strlen(concat_str) + 1);
  
        // Prompt user for another input
        printf("Enter another string to send to parent: ");
        scanf("%s", input_str_child);

        // Send this string back to parent (parent will concatenate "gobison.org")
        write(fd2[1], input_str_child, strlen(input_str_child) + 1);
        close(fd2[1]);

        exit(0);
    } 
  
    return 0; 
}