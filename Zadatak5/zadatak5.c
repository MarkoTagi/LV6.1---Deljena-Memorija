/*Korišćenjem programskog jezika C kreirati program koji se deli u dva procesa. Proces roditelj čita
sa tastature stringove i koristeći deljenu memoriju prosleđuje ih procesu detetu. Proces dete
primljene stringove upisuje u datoteku stringovi.txt. Kada korisnik unese string “KRAJ”
komunikacija se prekida i procesi se završavaju.*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#define SHARED_MEMORY_SIZE 1024
#define MAX_LENGTH 255

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short* array;
    struct seminfo* __buff;
};


int main() {
    key_t sharedMemoryKey = ftok("/tmp", 'a');
    if (sharedMemoryKey == -1) {
        perror("Failed to generate shared memory key");
        exit(EXIT_FAILURE);
    }
    int sharedMemoryId = shmget(sharedMemoryKey, SHARED_MEMORY_SIZE * sizeof(char), IPC_CREAT | 0666);
    if (sharedMemoryId == -1) {
        perror("Failed to create shared memory");
        exit(EXIT_FAILURE);
    }
    union semun semaphoreUnion;
    key_t writeMutexKey = ftok("/tmp", 'b');
    if (writeMutexKey == -1) {
        perror("Failed to generate lock1 key");
        exit(EXIT_FAILURE);
    }
    int writeMutexId = semget(writeMutexKey, 1, IPC_CREAT | 0666);
    if (writeMutexId == -1) {
        perror("Failed to create lock1");
        exit(EXIT_FAILURE);
    }
    semaphoreUnion.val = 1;
    semctl(writeMutexId, 0, SETVAL, semaphoreUnion);
    key_t readSemaphoreKey = ftok("/tmp", 'c');
    if (readSemaphoreKey == -1) {
        perror("Failed to generate lock2 key");
        exit(EXIT_FAILURE);
    }
    int readSemaphoreId = semget(readSemaphoreKey, 1, IPC_CREAT | 0666);
    if (readSemaphoreId == -1) {
        perror("Failed to create lock2");
        exit(EXIT_FAILURE);
    }
    semaphoreUnion.val = 0;
    semctl(readSemaphoreId, 0, SETVAL, semaphoreUnion);
    struct sembuf lock = { 0, -1, 0 }; struct sembuf unlock = { 0, 1, 0 };
    int pid = fork();
    if (pid == -1) {
        perror("Failed to fork");
        exit(EXIT_FAILURE);
    }  
    if (pid > 0) {
        char* sharedMemoryPointer = (char*)shmat(sharedMemoryId, NULL, 0);
        if (sharedMemoryPointer == NULL) {
            perror("Failed to get shared memory pointer");
            exit(EXIT_FAILURE);
        }
        char userInput[MAX_LENGTH];
        do {
            semop(writeMutexId, &lock, 1);
            printf("Send a message: ");
            fgets(userInput, MAX_LENGTH, stdin);
            int inputLength = strlen(userInput);
            userInput[--inputLength] = '\0';
            strcpy(sharedMemoryPointer, userInput);
            semop(readSemaphoreId, &unlock, 1);
        } while (strcmp(userInput, "QUIT") != 0);
        shmdt(sharedMemoryPointer);
        wait(NULL);
    }
    else {
        char* sharedMemoryPointer = (char*)shmat(sharedMemoryId, NULL, 0);
        if (sharedMemoryPointer == NULL) {
            perror("Failed to get shared memory pointer");
            exit(EXIT_FAILURE);
        }
        char buffer[MAX_LENGTH];
        FILE* filePointer = fopen("stringovi.txt", "w");
        do {
            semop(readSemaphoreId, &lock, 1);
            if (strcmp(sharedMemoryPointer, "QUIT") != 0) {
                strcpy(buffer, sharedMemoryPointer);
                fprintf(filePointer, "%s\n", buffer);
            }
            semop(writeMutexId, &unlock, 1);
        } while (strcmp(sharedMemoryPointer, "QUIT") != 0);
        fclose(filePointer);
        shmdt(sharedMemoryPointer);
    }
    semctl(writeMutexId, 0, IPC_RMID);
    semctl(readSemaphoreId, 0, IPC_RMID);
    shmctl(sharedMemoryId, IPC_RMID, NULL);
    return 0;
}