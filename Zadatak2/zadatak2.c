/*Korišćenjem programskog jezika C kreirati tri Linux procesa koji za međusobnu komunikaciju
koriste deljenu memoriju veličine 1024 bajtova. Prvi proces popunjava prvih 512 bajtova deljive
memorije slučajno izabranim slovima (u opsegu a-z). Nakon toga, drugi proces popunjava
poslednjih 512 bajtova deljene memorije, proizvoljno izabranim ciframa. Pošto i drugi proces
završi generisanje podataka, treći proces kompletan sadržaj deljene memorije upisuje u datoteku.
Ova sekvenca akcija se periodično ponavlja svakih 15 s.*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#define SHMEM_SIZE 1024

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short int *array;
    struct seminfo *__buf;
};

int main() {
    key_t shmkey = ftok("/tmp", 'a');
    int shmid = shmget(shmkey, SHMEM_SIZE * sizeof(char), IPC_CREAT | 0666);
    if (shmid == -1) {
        fprintf(stderr, "Error! [ shmget() has failed ]\n");
        exit(EXIT_FAILURE);
    }
    union semun sem;
    struct sembuf wait = { 0, -1, 0}; struct sembuf post = { 0, 1, 0};
    key_t lock1_key = ftok("/tmp", 'b'); key_t lock2_key = ftok("/tmp", 'c'); key_t lock3_key = ftok("/tmp", 'd');
    int lockid1 = semget(lock1_key, 1, IPC_CREAT | 0666);
    if (lockid1 == -1) {
        fprintf(stderr, "Error! [ semget() has failed ]\n");
        exit(EXIT_FAILURE);
    }
    sem.val = 1;
    semctl(lockid1, 0, SETVAL, sem);
    int lockid2 = semget(lock2_key, 1, IPC_CREAT | 0666);
    if (lockid2 == -1) {
        fprintf(stderr, "Error! [ semget() has failed ]\n");
        exit(EXIT_FAILURE);
    }
    sem.val = 0;
    semctl(lockid2, 0, SETVAL, sem);
    int lockid3 = semget(lock3_key, 1, IPC_CREAT | 0666);
    if (lockid3 == -1) {
        fprintf(stderr, "Error! [ semget() has failed ]\n");
        exit(EXIT_FAILURE);
    }
    semctl(lockid3, 0, SETVAL, sem);
    int pid1 = fork();
    if (pid1 == -1) {
        fprintf(stderr, "Error! [ fork() has failed ]\n");
        exit(EXIT_FAILURE);
    }
    if (pid1 == 0) { // Prvi proces 
        char* shmptr = (char*)shmat(shmid, NULL, 0);
        if (shmptr == NULL) {
            fprintf(stderr, "Error! [ shmat() has failed ]\n");
            exit(EXIT_FAILURE);
        }
        semop(lockid1, &wait, 1);
        char randomChar;
        srand(time(0));
        for (int i = 0; i < (SHMEM_SIZE >> 1); ++i) {
            randomChar = (rand() % 26) + 'a';
            shmptr[i] = randomChar;
        }
        semop(lockid1, &post, 1);
        semop(lockid2, &post, 1);
        shmdt(shmptr);
    }
    else {
        int pid2 = fork();
        if (pid2 == -1) {
            fprintf(stderr, "Error! [ fork() has failed ]\n");
            exit(EXIT_FAILURE);
        }
        if (pid2 == 0) { // Drugi proces 
            char* shmptr = (char*)shmat(shmid, NULL, 0);
            if (shmptr == NULL) {
                fprintf(stderr, "Error! [ shmat() has failed ]\n");
                exit(EXIT_FAILURE);
            }
            semop(lockid2, &wait, 1);
            char crandomNumber;
            srand(time(0));
            for (int i = 0; i < (SHMEM_SIZE >> 1); ++i) {
                crandomNumber = rand() % 10 + '0';
                shmptr[i + (SHMEM_SIZE >> 1)] = crandomNumber;
            }
            semop(lockid2, &post, 1);
            semop(lockid3, &post, 1);
            shmdt(shmptr);
        }
        else { // Treci proces
            char* shmptr = (char*)shmat(shmid, NULL, 0);
            if (shmptr == NULL) {
                fprintf(stderr, "Error! [ shmat() has failed ]\n");
                exit(EXIT_FAILURE);
            }
            semop(lockid3, &wait, 1);
            FILE* filep = fopen("result.txt", "w");
            if (filep == NULL) {
                fprintf(stderr, "Error [ fopen() has failed ]\n");
                exit(EXIT_FAILURE);
            }
            char aCharacter;
            for (int i = 0; i < SHMEM_SIZE; ++i) {
                aCharacter = shmptr[i];
                fputc(aCharacter, filep); fputc('\n', filep);
            }
            semop(lockid3, &post, 1);
            fclose(filep);
            shmdt(shmptr);
        }
    }
    semctl(lockid1, 0, IPC_RMID);
    semctl(lockid2, 0, IPC_RMID);
    semctl(lockid3, 0, IPC_RMID);
    shmctl(shmid, IPC_RMID, NULL);
    return 0;
}