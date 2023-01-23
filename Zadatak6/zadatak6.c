#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#define SHARED_MEMORY_SIZE 4096

union semun {
    int val;
    struct semid_ds* buf;
    unsigned short* array;
    struct seminfo* __buf; 
};

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "bad usage: %s\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    FILE* originalFilePointer = fopen(argv[1], "r");
    if (originalFilePointer == NULL) {
        int pid = fork();
        if (pid == 0) {
            if (execl("./generate_random_txtfile", "generate_random_txtfile", argv[1], NULL) == -1) {
                perror("execl");
                exit(EXIT_FAILURE);
            }
        }
        wait(NULL); // MORA DA SE ZAVRSI PROCES DETE PRE NEGO STO POKUSAMO DA PRISTUPIMO POKAZIVACU (JER JE NULL)
        originalFilePointer = fopen(argv[1], "r");
    }
    key_t sharedMemoryKey = ftok("/tmp", 'a');
    if (sharedMemoryKey == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    int sharedMemoryId = shmget(sharedMemoryKey, SHARED_MEMORY_SIZE * sizeof(char), IPC_CREAT | 0666);
    if (sharedMemoryId == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }
    key_t mutexKey = ftok("/tmp", 'b');
    if (mutexKey == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    key_t writeMutexKey = ftok("/tmp", 'c');
    if (writeMutexKey == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    key_t readSemaphoreKey = ftok("/tmp", 'd');
    if (readSemaphoreKey == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    int mutexId = semget(mutexKey, 1, IPC_CREAT | 0666);
    if (mutexId == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }
    int writeMutexId = semget(writeMutexKey, 1, IPC_CREAT | 0666);
    if (writeMutexId == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }
    int readSemaphoreId = semget(readSemaphoreKey, 1, IPC_CREAT | 0666);
    if (readSemaphoreId == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }
    union semun semaphoreUnion;
    fseek(originalFilePointer, 0, SEEK_END);
    int originalFileSize = ftell(originalFilePointer);
    fseek(originalFilePointer, 0, SEEK_SET);
    semaphoreUnion.val = 1;
    if (semctl(mutexId, 0, SETVAL, semaphoreUnion) == -1) {
        perror("semctl");
        exit(EXIT_FAILURE);
    }
    semaphoreUnion.val = originalFileSize;
    if (semctl(writeMutexId, 0, SETVAL, semaphoreUnion) == -1) {
        perror("semctl");
        exit(EXIT_FAILURE);
    }
    semaphoreUnion.val = 0;
    if (semctl(readSemaphoreId, 0, SETVAL, semaphoreUnion) == -1) {
        perror("semctl");
        exit(EXIT_FAILURE);
    }
    struct sembuf semaphoreWait = { 0, -1, 0 }; struct sembuf semaphorePost = { 0, 1, 0 };
    int pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        void* sharedMemoryPointer = (void*)shmat(sharedMemoryId, NULL, 0);
        if (sharedMemoryPointer == (void*)-1) {
            perror("shmat");
            exit(EXIT_FAILURE);
        }
        ((int*)sharedMemoryPointer)[0] = 0;
        char characterBuffer;
        while ((characterBuffer = fgetc(originalFilePointer)) != EOF) {
            semop(writeMutexId, &semaphoreWait, 1);
            semop(mutexId, &semaphoreWait, 1);
            int currentPosition = ((int*)sharedMemoryPointer)[0];
            // printf("Writing [ %c ] @ [ %d ]\n", characterBuffer, ++currentPosition); fflush(stdout);
            ++currentPosition;
            ((char*)sharedMemoryPointer)[currentPosition + sizeof(int)] = characterBuffer;
            ((int*)sharedMemoryPointer)[0] = currentPosition;
            semop(readSemaphoreId, &semaphorePost, 1);
            semop(mutexId, &semaphorePost, 1);
        }
        shmdt(sharedMemoryPointer);
        wait(NULL);
        // printf("The parent process is now exiting...\n");
    }
    else {
        char* sharedMemoryPointer = (char*)shmat(sharedMemoryId, NULL, 0);
        if (sharedMemoryPointer == (void*)-1) {
            perror("shmat");
            exit(EXIT_FAILURE);
        }
        FILE* copyFilePointer = fopen(argv[2], "w");
        if (copyFilePointer == NULL) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }
        int readAt = 1;
        char characterBuffer;
        while (readAt <= originalFileSize) {
            semop(readSemaphoreId, &semaphoreWait, 1);
            semop(mutexId, &semaphoreWait, 1);
            characterBuffer = sharedMemoryPointer[sizeof(int) + readAt];
            readAt++;
            // printf("Reading [ %c ] @ [ %d ]\n", characterBuffer, readAt++); fflush(stdout);
            fputc(characterBuffer, copyFilePointer);
            semop(writeMutexId, &semaphorePost, 1);
            semop(mutexId, &semaphorePost, 1);
        }
        fclose(copyFilePointer);
        shmdt(sharedMemoryPointer);
        // printf("The child process is now exiting...\n");
    }
    semctl(mutexId, 0, IPC_RMID);
    semctl(writeMutexId, 0, IPC_RMID);
    semctl(readSemaphoreId, 0, IPC_RMID);
    shmctl(sharedMemoryId, IPC_RMID, NULL);
    return 0;
}