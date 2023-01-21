/*Korišćenjem programskog jezika C napisati UNIX/Linux program koji se deli u dva procesa
(proizvođač - potrošač) koji komuniciraju koristeći ograničeni kružni bafer u deljivoj memoriji.
Kružni bafer je veličine 10 celih brojeva. Ovi procesi sinhronizuju svoje aktivnosti (upis u bafer i
čitanje iz bafera) koristeći semafore.*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#define SHM_ID 1337
#define SHM_SIZE 10
#define PRODUCER_ID 10001
#define CONSUMER_ID 10002

union semun {
    int val;
    struct semid_ds* buf;
    unsigned short* array;
    struct seminfo* __buf;
};

int main() {
    int shmkey = shmget(SHM_ID, SHM_SIZE * sizeof(int), IPC_CREAT | 0666);
    if (shmkey == -1) {
        fprintf(stderr, "Error! [ shmget() has failed ]\n");
        exit(EXIT_FAILURE);
    }
    struct sembuf wait = { 0, -1, 0};
    struct sembuf post = { 0, 1, 0};
    union semun sem;
    int producer_id = semget(PRODUCER_ID, 1, IPC_CREAT | 0666);
    if (producer_id == -1) {
        fprintf(stderr, "Error! [ Producer semget() has failed ]\n");
        exit(EXIT_FAILURE);
    }
    sem.val = 10;
    semctl(producer_id, 0, SETVAL, sem);
    int consumer_id = semget(CONSUMER_ID, 1, IPC_CREAT | 0666);
    if (consumer_id == -1) {
        fprintf(stderr, "Error! [ Consumer semget() has failed ]\n");
        exit(EXIT_FAILURE);
    }
    sem.val = 0;
    semctl(consumer_id, 0, SETVAL, sem);
    int pid = fork();
    if (pid == -1) {
        fprintf(stderr, "Error! [ fork() has failed ]\n");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) { // Procuder
        int* shmptr = (int*)shmat(shmkey, NULL, 0);
        if (shmptr == NULL) {
            fprintf(stderr, "Error! [ shmat() has failed ]\n");
            exit(EXIT_FAILURE);
        }
        int writeAt = 0;
        srand(time(0));
        while (1) {
            semop(producer_id, &wait, 1);
            int randomNumber = rand() % 99 + 1;
            printf("Generated: [ %d ].\n", randomNumber);
            *(shmptr + writeAt) = randomNumber;
            writeAt += 1 % SHM_SIZE;
            semop(consumer_id, &post, 1);
            sleep(1);
        }
        shmdt(shmptr);
    }
    else { // Consumer
        int* shmptr = (int*)shmat(shmkey, NULL, 0);
        int readFrom = 0;
        while (1) {
            semop(consumer_id, &wait, 1);
            printf("Read: [ %d ].\n", *(shmptr + readFrom));
            readFrom += 1 % SHM_SIZE;
            semop(producer_id, &post, 1);
            sleep(2);
        }
        shmdt(shmptr);
    }
    semctl(producer_id, 0, IPC_RMID);
    semctl(consumer_id, 0, IPC_RMID);
    shmctl(shmkey, IPC_RMID, 0);
    return 0;
}