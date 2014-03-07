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

#define H_ID_BASE 1
#define M1_ID_BASE 100
#define MAX_WAITING_DURATION 10     //If person waits for 10 mins leaves.
#define SELLING_DURATION 60     //Sell tickets for 60 mins

int seats[SEAT_ROWS][SEAT_COLUMNS];

struct itimerval hSellersTimer;
time_t startTime;

void customerArrivesAtH(int id) {
    char event[80];
    arrivalsCount++;
    
    // One customer at H at a time
    // Dont need to implement customer leaving if we dont want E.C.
    if (waitCount <= 1) {
        
        pthread_mutex_lock(/*figure out which mutex*/);
        
        // Find open seat in first row seats[1][i]
        int row, col;
        for (col = 0; col < SEAT_COLUMNS; col++) {
            if (seats[1][col] == -1) {
                //Assign seat to this customer
                seats[1][col] = id;
                //Seat assigned break out of loop
                break;
            }
        }
        
        // check the rest of the rows once row 1 is filled
        
        // Release the mutex lock
        pthread_mutex_unlock(/*same mutex used above*/);
        
        sprintf(event, "A customer arrives at the tail of H seller’s queue", id);
        print(event);
        
        // Signal the H seller
        sem_post(/*figure out a semaphore for this*/);
    }
    else {
        // Add next customer to a queue
    }
}

void *customer(void *param) {
    int custNum = *((int *) param);
    
    sleep(rand()%SELLING_DURATION);
    if (custNum >= 1 && custNum < 100) {
        customerArrivesAtH(custNum);
    }
    
    else if (custNum >= 100 && custNum < 200) {
        customerArrivesAtM1(custNum);
    }
    else {
        // rest of sellers
    }
    
    return NULL;
}

int timesUp = 0; //set to 1 when time is over

void hSellsTickets() {
    
    if (!timesUp) {
        
    }
}

void *hSeller(void *param) {
    time(&startTime);
    print("H ticket seller opens");
    
    //Set timer for ticket selling duration
    hSellersTimer.it_value.tv_sec = SELLING_DURATION;
    setitimer(ITIMER_REAL, &hSellersTimer, NULL);
    
    //Sell tickets until duration is over
    do {
        hSellsTickets();
    } while (!timesUp);
    
    print("H ticket seller closes");
    
    return NULL;
}

int main(int argc, char *argv[]) {
    int n;
    
    int hSellerId = 0;
    
    //Set each seat to -1 which means its empty
    int r, c;
    for (r = 0; r < SEAT_ROWS; r++) {
        for (c = 0; < SEAT_COLUMNS; c++) {
            seats[r][c] = -1;
        }
    }
    
    //Check for only one command-line argument
    if (argc != 2) { printf("Usage: requires 1 integer argument\n"); }
    else {
        n = atoi(argv[1]);
        int hCustomerIds[n];
    }
    
    pthread_t hSellerThreadId;
    pthread_attr_t hSellAttr;
    pthread_attr_init(&hSellAttr);
    pthread_create(&hSellerThreadId, &hSellAttr, hSeller, &hSellerId);
    
    int i;
    //Generate N customers for H
    for (i = 0; i < n; i++) {
        hCustomerIds[i] = H_ID_BASE + i;
        pthread_t hCustThreadId;
        pthread_attr_t hCustAttr;
        pthread_attr_init(&hCustAttr);
        pthread_create(&hCustThreadId, &hCustAttr, customer, &hCustomerIds[i]);
    }
    
    
    
    
    
    
    
    return 0;
}