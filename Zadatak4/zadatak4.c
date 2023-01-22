/*Korišćenjem programskog jezika C kreirati dva Linux procesa. Prvi proces kreira u deljenom
memorijskom segmentu matricu dimenzija MxN. Proces 2 u svakoj koloni matrice pronalazi
maksimalni element i štampa ga na ekranu.*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#define SHAREDMEMORY_SIZE 1024

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *__buf;
};

int main() {
    int rowCount, columnCount;
    printf("Enter the number of rows: ");
    scanf("%d", &rowCount);
    printf("Enter the number of columns: ");
    scanf("%d", &columnCount);
    key_t sharedMemoryKey = ftok("/tmp", 'a');
    if (sharedMemoryKey == -1) {
        fprintf(stderr, "Error! [ ftok() has failed ]\n");
        exit(EXIT_FAILURE);
    }
    int sharedMemoryID = shmget(sharedMemoryKey, SHAREDMEMORY_SIZE * sizeof(int), IPC_CREAT | 0666);
    if (sharedMemoryID == -1) {
        perror("shmget()");
        fprintf(stderr, "Error number [ %d ]\n", errno);
        exit(EXIT_FAILURE);
    }
    key_t writeMutexKey = ftok("/tmp", 'b');
    if (writeMutexKey == -1) {
        fprintf(stderr, "Error! [ ftok() has failed ]\n");
        exit(EXIT_FAILURE);
    }
    int writeMutexID = semget(writeMutexKey, 1, IPC_CREAT | 0666);
    if (writeMutexID == -1) {
        fprintf(stderr, "Error! [ semget() has failed ]\n");
        exit(EXIT_FAILURE);
    }
    union semun semaphoreUnion;
    semaphoreUnion.val = 1;
    semctl(writeMutexID, 0, SETVAL, semaphoreUnion);
    key_t readMutexKey = ftok("/tmp", 'c');
    if (readMutexKey == -1) {
        fprintf(stderr, "Error! [ ftok() has failed ]\n");
        exit(EXIT_FAILURE);
    }
    int readMutexID = semget(readMutexKey, 1, IPC_CREAT | 0666);
    semaphoreUnion.val = 0;
    semctl(readMutexID, 0, SETVAL, semaphoreUnion);
    struct sembuf lock = { 0, -1, 0 }; struct sembuf unlock = { 0, 1, 0 };
    int pid = fork();
    if (pid == -1) {
        fprintf(stderr, "Error! [ fork() has failed ]\n");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        int* matrix = (int*)shmat(sharedMemoryID, NULL, 0);
        if (matrix == NULL) {
            fprintf(stderr, "Error! [ shmat() has failed ]\n");
            exit(EXIT_FAILURE);
        }
        semop(writeMutexID, &lock, 1);
        int base = 0;
        for (int rowIndex = 0; rowIndex < rowCount; ++rowIndex) {
            for (int columnIndex = 0; columnIndex < columnCount; ++columnIndex) {
                printf("Enter the value of element [ %d ] [ %d ]: ", rowIndex, columnIndex);
                scanf("%d", &matrix[base] + columnIndex);
            }
            base += columnCount;
        }
        base = 0;
        printf("Matrix:\n");
        for (int rowIndex = 0; rowIndex < rowCount; ++rowIndex) {
            printf("\t");
            for (int columnIndex = 0; columnIndex < (columnCount - 1); ++columnIndex) {
                printf("[ %d ] ", *(&matrix[base] + columnIndex));
            }
            printf("[ %d ]\n", *(&matrix[base] + columnCount - 1));
            base += columnCount;
        }
        printf("\n");
        semop(writeMutexID, &unlock, 1);
        semop(readMutexID, &unlock, 1);
        shmdt(matrix);
        wait(NULL);
    }
    else {
        int* matrix = (int*)shmat(sharedMemoryID, NULL, 0);
        if (matrix == NULL) {
            fprintf(stderr, "Error! [ shmat() has failed ]\n");
            exit(EXIT_FAILURE);
        }
        semop(readMutexID, &lock, 1);
        int base = columnCount;
        for (int columnIndex = 0; columnIndex < columnCount; ++columnIndex) {
            int maxIndex = 0;
            for (int rowIndex = 1; rowIndex < rowCount; ++rowIndex) {
                if (*(&matrix[base] + columnIndex) > *(&matrix[maxIndex] + columnIndex)) {
                    maxIndex = base;
                }
                base += columnCount;
            }
            base = columnCount;
            printf("Column #%d max element: %d.\n", columnIndex, *(&matrix[columnIndex] + maxIndex));
        }
    }
    shmctl(sharedMemoryID, IPC_RMID, NULL);
    semctl(writeMutexID, 0, IPC_RMID);
    semctl(readMutexID, 0, IPC_RMID);
    return 0;
}