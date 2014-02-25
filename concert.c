#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/time.h>

#define SEAT_COUNT 100
#define SEAT_ROWS 10
#define SEAT_COLUMNS 10

#define H_SELLERS 1
#define M_SELLERS 3
#define L_SELLERS 6

#define MAX_WAITING_DURATION 10     //If person waits for 10 mins leaves.
#define MAX_SELLING_DURATION 60     //Sell tickets for 60 mins

char seats[SEAT_ROWS][SEAT_COLUMNS]; //Matrix of the auditorium

pthread_mutex_t seatMutex;  //mutex protects the seats a (shared resource)
sem_t filledH;              //seller H waits on this
sem_t filledM;              //sellers M wait on this
sem_t filledL;              //sellers L wait on this
 
time_t startTime;

int n;

int main(int argc, char *argv[]) {
    
    //Check for only one command-line argument
    if (argc != 2) { printf("Usage: requires 1 integer argument\n"); }
    else { 
        n = atoi(argv[1]); 
        printf("%d\n", n);
    }
    
    
    return 0;
}