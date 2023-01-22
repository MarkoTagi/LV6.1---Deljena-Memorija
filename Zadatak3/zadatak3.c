#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#define MAX_LENGTH 1024

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *__buf;
};

typedef enum State {
    eating,
    hungry,
    thinking
} State;

typedef struct Philosopher {
    char name[MAX_LENGTH];
    char quote[MAX_LENGTH];
    int seat;
    State currentState;
} Philosopher;

void initializePhilosopher(Philosopher* philosopher, char* name, char* quote, int seat, State currentState);

int main() {
    key_t sharedMemoryKey = ftok("/tmp", 'a');
    if (sharedMemoryKey == -1) {
        fprintf(stderr, "Error! [ ftok() has failed ]\n");
        exit(EXIT_FAILURE);
    }
    int sharedMemoryID = shmget(sharedMemoryKey, 5 * sizeof(int), IPC_CREAT | 0666);
    if (sharedMemoryID == -1) {
        fprintf(stderr, "Error! [ shmget() has failed ]\n");
        exit(EXIT_FAILURE);
    }
    key_t chopstickLockKey = ftok("/tmp", 'b');
    if (chopstickLockKey == -1) {
        fprintf(stderr, "Error! [ ftok() has failed ]\n");
        exit(EXIT_FAILURE);
    }
    int chopstickLockID = semget(chopstickLockKey, 5, IPC_CREAT | 0666);
    if (chopstickLockID == -1) {
        fprintf(stderr, "Error! [ semget() has failed ]\n");
        exit(EXIT_FAILURE);
    }
    union semun semaphoreUnion;
    struct sembuf lock = { 0, -1, 0}; struct sembuf unlock = { 0, 1, 0};
    semaphoreUnion.val = 1;
    for (int i = 0; i < 5; ++i) semctl(chopstickLockID, i, SETVAL, semaphoreUnion);
    int* sharedMemoryPointer = (int*)shmat(sharedMemoryID, NULL, 0);
    for (int i = 0; i < 5; ++i) sharedMemoryPointer[i] = 1; // Stapici( inicijalno svi dostupni )
    Philosopher* philosophers[5];
    for (int i = 0; i < 5; ++i) philosophers[i] = (Philosopher*)malloc(sizeof(Philosopher));
    initializePhilosopher(philosophers[0], "Aristotle", "Knowing yourself is the beginning of all wisdom", 0, thinking);
    initializePhilosopher(philosophers[1], "Confucius", "It does not matter how slowly you go as long as you do not stop.",  1, thinking);
    initializePhilosopher(philosophers[2], "Immanuel Kant", "In law a man is guilty when he violates the rights of others.", 2, thinking);
    initializePhilosopher(philosophers[3], "Plato", "The measure of a man is what he does with power.", 3, thinking);
    initializePhilosopher(philosophers[4], "Rene Decartes", "Conquer yourself rather than the world.", 4, thinking);
    srand(time(NULL));
    int pid1 = fork();
    if (pid1 == -1) {
        fprintf(stderr, "Error! [ fork() has failed ]\n");
        exit(EXIT_FAILURE);
    }
    if (pid1 == 0) {
        while (1) {
            if (philosophers[0]->currentState == thinking) {
                printf("%s: %s\n", philosophers[0]->name, philosophers[0]->quote);
                int thinkingSeconds = rand() % 9 + 1;
                sleep(thinkingSeconds);
                philosophers[0]->currentState = hungry;
                printf("%s: (I'm really hungry...)\n", philosophers[0]->name); fflush(stdout);  
            }
            if (sharedMemoryPointer[0] == 1 && sharedMemoryPointer[4] == 1) {
                lock.sem_num = 0;
                semop(chopstickLockID, &lock, 1);
                lock.sem_num = 4;
                semop(chopstickLockID, &lock, 1);
                sharedMemoryPointer[0] = 0;
                sharedMemoryPointer[4] = 0;
                philosophers[0]->currentState = eating;
                printf("%s: (So tasty...)\n", philosophers[0]->name); fflush(stdout);
                int eatingSeconds = rand() % 9 + 1;
                sleep(eatingSeconds);
                sharedMemoryPointer[0] = 1;
                sharedMemoryPointer[4] = 1;
                philosophers[0]->currentState = thinking;
                unlock.sem_num = 0;
                semop(chopstickLockID, &unlock, 1);
                unlock.sem_num = 4;
                semop(chopstickLockID, &unlock, 1);
            }
            else {
                printf("%s: (Damn, hes using the chopsticks...)\n", philosophers[0]->name); 
                sleep(2);
                philosophers[0]->currentState = thinking;
            }
        }
        shmdt(sharedMemoryPointer);
    }
    else {
        int pid2 = fork();
        if (pid2 == -1) {
            fprintf(stderr, "Error! [ fork() has failed ]\n");
            exit(EXIT_FAILURE);
        }
        if (pid2 == 0) {
            while (1) {
                if (philosophers[1]->currentState == thinking) {
                    printf("%s: %s\n", philosophers[1]->name, philosophers[1]->quote);
                    int thinkingSeconds = rand() % 9 + 1;
                    sleep(thinkingSeconds);
                    philosophers[1]->currentState = hungry;
                    printf("%s: (I'm really hungry...)\n", philosophers[1]->name);  
                }
                if (sharedMemoryPointer[1] == 1 && sharedMemoryPointer[0] == 1) {
                    lock.sem_num = 1;
                    semop(chopstickLockID, &lock, 1);
                    lock.sem_num = 0;
                    semop(chopstickLockID, &lock, 1);
                    sharedMemoryPointer[1] = 0;
                    sharedMemoryPointer[0] = 0;
                    philosophers[1]->currentState = eating;
                    printf("%s: (So tasty...)\n", philosophers[1]->name);
                    int eatingSeconds = rand() % 9 + 1;
                    sleep(eatingSeconds);
                    sharedMemoryPointer[1] = 1;
                    sharedMemoryPointer[0] = 1;
                    philosophers[1]->currentState = thinking;
                    unlock.sem_num = 1;
                    semop(chopstickLockID, &unlock, 1);
                    unlock.sem_num = 0;
                    semop(chopstickLockID, &unlock, 1);
                }
                else {
                    printf("%s: (Damn, hes using the chopsticks...)\n", philosophers[1]->name);
                    sleep(2);
                    philosophers[1]->currentState = thinking;
                }
            }
            shmdt(sharedMemoryPointer);
        }
        else {
            int pid3 = fork();
             if (pid3 == -1) {
                fprintf(stderr, "Error! [ fork() has failed ]\n");
                exit(EXIT_FAILURE);
            }
            if (pid3 == 0) {
                while (1) {
                    if (philosophers[2]->currentState == thinking) {
                        printf("%s: %s\n", philosophers[2]->name, philosophers[2]->quote);
                        int thinkingSeconds = rand() % 9 + 1;
                        sleep(thinkingSeconds);
                        philosophers[2]->currentState = hungry;
                        printf("%s: (I'm really hungry...)\n", philosophers[2]->name);  
                    }
                    if (sharedMemoryPointer[2] == 1 && sharedMemoryPointer[1] == 1) {
                        lock.sem_num = 2;
                        semop(chopstickLockID, &lock, 1);
                        lock.sem_num = 1;
                        semop(chopstickLockID, &lock, 1);
                        sharedMemoryPointer[2] = 0;
                        sharedMemoryPointer[1] = 0;
                        philosophers[2]->currentState = eating;
                        printf("%s: (So tasty...)\n", philosophers[2]->name);
                        int eatingSeconds = rand() % 9 + 1;
                        sleep(eatingSeconds);
                        sharedMemoryPointer[2] = 1;
                        sharedMemoryPointer[1] = 1;
                        philosophers[2]->currentState = thinking;
                        unlock.sem_num = 2;
                        semop(chopstickLockID, &unlock, 1);
                        unlock.sem_num = 1;
                        semop(chopstickLockID, &unlock, 1);
                    }
                    else {
                        printf("%s: (Damn, hes using the chopsticks...)\n", philosophers[2]->name);
                        sleep(2);
                        philosophers[2]->currentState = thinking;
                    }
                }
                shmdt(sharedMemoryPointer);
            }
            else {
                int pid4 = fork();
                if (pid4 == -1) {
                    fprintf(stderr, "Error! [ fork() has failed ]\n");
                    exit(EXIT_FAILURE);
                }
                if (pid4 == 0) {
                    while (1) {
                        if (philosophers[3]->currentState == thinking) {
                            printf("%s: %s\n", philosophers[3]->name, philosophers[3]->quote);
                            int thinkingSeconds = rand() % 9 + 1;
                            sleep(thinkingSeconds);
                            philosophers[3]->currentState = hungry;
                            printf("%s: (I'm really hungry...)\n", philosophers[3]->name);  
                        }
                        if (sharedMemoryPointer[3] == 1 && sharedMemoryPointer[2] == 1) {
                            lock.sem_num = 3;
                            semop(chopstickLockID, &lock, 1);
                            lock.sem_num = 2;
                            semop(chopstickLockID, &lock, 1);
                            sharedMemoryPointer[3] = 0;
                            sharedMemoryPointer[2] = 0;
                            philosophers[3]->currentState = eating;
                            printf("%s: (So tasty...)\n", philosophers[3]->name);
                            int eatingSeconds = rand() % 9 + 1;
                            sleep(eatingSeconds);
                            sharedMemoryPointer[3] = 1;
                            sharedMemoryPointer[2] = 1;
                            philosophers[3]->currentState = thinking;
                            unlock.sem_num = 3;
                            semop(chopstickLockID, &unlock, 1);
                            unlock.sem_num = 2;
                            semop(chopstickLockID, &unlock, 1);
                        }
                        else {
                            printf("%s: (Damn, hes using the chopsticks...)\n", philosophers[3]->name);
                            sleep(2);
                            philosophers[3]->currentState = thinking;
                        }
                    }
                    shmdt(sharedMemoryPointer);
                }
                else {
                    while (1) {
                        if (philosophers[4]->currentState == thinking) {
                            printf("%s: %s\n", philosophers[4]->name, philosophers[4]->quote);
                            int thinkingSeconds = rand() % 9 + 1;
                            sleep(thinkingSeconds);
                            philosophers[4]->currentState = hungry;
                            printf("%s: (I'm really hungry...)\n", philosophers[4]->name);  
                        }
                        if (sharedMemoryPointer[4] == 1 && sharedMemoryPointer[3] == 1) {
                            lock.sem_num = 4;
                            semop(chopstickLockID, &lock, 1);
                            lock.sem_num = 3;
                            semop(chopstickLockID, &lock, 1);
                            sharedMemoryPointer[4] = 0;
                            sharedMemoryPointer[3] = 0;
                            philosophers[4]->currentState = eating;
                            printf("%s: (So tasty...)\n", philosophers[4]->name);
                            int eatingSeconds = rand() % 9 + 1;
                            sleep(eatingSeconds);
                            sharedMemoryPointer[4] = 1;
                            sharedMemoryPointer[3] = 1;
                            philosophers[4]->currentState = thinking;
                            unlock.sem_num = 4;
                            semop(chopstickLockID, &unlock, 1);
                            unlock.sem_num = 3;
                            semop(chopstickLockID, &unlock, 1);
                        }
                        else {
                            printf("%s: (Damn, hes using the chopsticks...)\n", philosophers[4]->name);
                            sleep(2);
                            philosophers[4]->currentState = thinking;
                        }
                    }
                    shmdt(sharedMemoryPointer);
                }
            }
        }
    }
    for (int i = 0; i < 5; ++i) {
        semctl(chopstickLockID, i, IPC_RMID, NULL);
        free(philosophers[i]);
    }
    shmctl(sharedMemoryID, IPC_RMID, NULL);
    return 0;
}

void initializePhilosopher(Philosopher* philosopher, char* name, char* quote, int seat, State currentState) {
    strcpy(philosopher->name, name);
    strcpy(philosopher->quote, quote);
    philosopher->seat = seat;
    philosopher->currentState = currentState;
}