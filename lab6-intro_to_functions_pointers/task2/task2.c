#include <stdio.h>
#include <stdlib.h>

/* Declare your functions here */
int add(int a, int b);
int subtract(int a, int b);
int multiply(int a, int b);
int divide(int a, int b);
void exit_program();

/* Main program */
int main(void)
{
    int a = 6, b = 3;  // Predefined integers
    char choice;

    // Array of function pointers
    int (*operations[4])(int, int) = { add, subtract, multiply, divide };
    void (*exit_op)() = exit_program;

    printf("Operand 'a' : %d | Operand 'b' : %d\n", a, b);
    printf("Specify the operation to perform (0 : add | 1 : subtract | 2 : Multiply | 3 : divide | 4 : exit): ");
    
    scanf(" %c", &choice);

    // Perform the selected operation using the function pointer array
    if (choice >= '0' && choice <= '3') {
        int result = operations[choice - '0'](a, b);  // Call the corresponding function
        printf("Result: %d\n", result);
    } else if (choice == '4') {
        exit_op();  // Exit the program
    }

    return 0;
}

/* Define your functions here */
int add(int a, int b) { 
    printf("Adding 'a' and 'b'\n"); 
    return a + b; 
}

int subtract(int a, int b) { 
    printf("Subtracting 'b' from 'a'\n"); 
    return a - b; 
}

int multiply(int a, int b) { 
    printf("Multiplying 'a' and 'b'\n"); 
    return a * b; 
}

int divide(int a, int b) { 
    if (b == 0) {
        printf("Error: Division by zero\n");
        return 0;
    }
    printf("Dividing 'a' by 'b'\n"); 
    return a / b; 
}

void exit_program() {
    printf("Exiting program\n");
    exit(0);
}