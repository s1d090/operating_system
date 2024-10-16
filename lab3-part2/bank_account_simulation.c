#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

#define SHM_KEY 9876 // Shared memory key

void ParentProcess(int *BankAccount, int *Turn); // Parent (Dear Old Dad)
void ChildProcess(int *BankAccount, int *Turn);  // Child (Poor Student)

int main() {
    int shm_id;
    int *ShmPTR;
    pid_t pid;
    int status;

    // Create shared memory segment for two integers (BankAccount and Turn)
    shm_id = shmget(SHM_KEY, 2 * sizeof(int), IPC_CREAT | 0666);
    if (shm_id < 0) {
        printf("*** shmget error ***\n");
        exit(1);
    }
    printf("Shared memory created.\n");

    // Attach the shared memory
    ShmPTR = (int *) shmat(shm_id, NULL, 0);
    if (ShmPTR == (int *) -1) {
        printf("*** shmat error ***\n");
        exit(1);
    }
    printf("Shared memory attached.\n");

    // Initialize shared variables
    ShmPTR[0] = 0; // BankAccount starts at 0
    ShmPTR[1] = 0; // Turn starts at 0 (Parent's turn)

    // Fork a child process
    pid = fork();
    if (pid < 0) {
        printf("*** fork error ***\n");
        exit(1);
    }

    if (pid == 0) {
        // Child process
        ChildProcess(&ShmPTR[0], &ShmPTR[1]);
        exit(0);
    } else {
        // Parent process
        ParentProcess(&ShmPTR[0], &ShmPTR[1]);
        
        // Wait for child process to complete
        wait(&status);

        // Detach and remove shared memory
        shmdt(ShmPTR);
        shmctl(shm_id, IPC_RMID, NULL);
        printf("Shared memory detached and removed.\n");
    }

    return 0;
}

void ParentProcess(int *BankAccount, int *Turn) {
    int account, deposit;
    srand(time(NULL)); // Seed the random number generator

    for (int i = 0; i < 25; i++) {
        sleep(rand() % 6); // Sleep between 0-5 seconds

        account = *BankAccount;
        while (*Turn != 0); // Wait for Parent's turn

        // Check if the account has <= $100
        if (account <= 100) {
            deposit = rand() % 101; // Generate a random deposit between 0 and 100

            if (deposit % 2 == 0) { // If even, deposit
                account += deposit;
                printf("Dear old Dad: Deposits $%d / Balance = $%d\n", deposit, account);
            } else {
                printf("Dear old Dad: Doesn't have any money to give\n");
            }
        } else {
            printf("Dear old Dad: Thinks Student has enough Cash ($%d)\n", account);
        }

        // Update shared memory
        *BankAccount = account;
        *Turn = 1; // Pass turn to child
    }
}

void ChildProcess(int *BankAccount, int *Turn) {
    int account, withdrawal;
    srand(time(NULL) + 1); // Different seed for child process

    for (int i = 0; i < 25; i++) {
        sleep(rand() % 6); // Sleep between 0-5 seconds

        account = *BankAccount;
        while (*Turn != 1); // Wait for Child's turn

        // Generate a random withdrawal between 0 and 50
        withdrawal = rand() % 51;
        printf("Poor Student needs $%d\n", withdrawal);

        // Check if there is enough money to withdraw
        if (withdrawal <= account) {
            account -= withdrawal;
            printf("Poor Student: Withdraws $%d / Balance = $%d\n", withdrawal, account);
        } else {
            printf("Poor Student: Not Enough Cash ($%d)\n", account);
        }

        // Update shared memory
        *BankAccount = account;
        *Turn = 0; // Pass turn to parent
    }
}