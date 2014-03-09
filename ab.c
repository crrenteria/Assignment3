#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>


// function declarations.
void print();
void lowSeller(void *ptr);
void medSeller(void *ptr);
void hiSeller(void *ptr);
void customer(void *ptr);
void timerHandler(int signal);
void customerGenerator(int *queue);
void timerHandler(int signal);
int findHead(int *queue);
int seatFinder(int row);
int findHeadtwo(int id);

// ticket sellers
int SELLERS_L = 6; // low
int SELLERS_M = 3; // med
int SELLERS_H = 1; // high
int SELLERS_A = 10; // all

// tickets sold
int total_l = 0; // low
int total_m = 0; // med
int total_h = 0; // high
int total_a = 0; // all

int CUSTOMERS = 15; // adjustable n, set as 5, 10, 15 via command line
int queue[10][99]; // queue of customers. adjustable?

// base IDs
int lowNum = 30;
int medNum = 20;
int highNum = 10;

int closed = 0; // boolean seller closed or open
int MINUTES = 60; // 1 second real time = 1 minute sim time (60 minutes --> 60 seconds)

// threads
pthread_mutex_t low, med, high; // thread for type
pthread_mutex_t printLock, customerLock; // thread locks

struct itimerval sellerTimer;
time_t startTime;

int lowcount[6]; // 6 sellers for L
int medcount[3]; // 3 sellers for M
int hicount = 0; // 1 seller for H, 0 tickets sold for each high ticket seller

// the concert seats
int seat[10][10] = { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                     { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
                    };

int i, j;

/**
 * Creates and executes threads.
 */
main(int argc, char *argv[]) {
   SELLERS_A = SELLERS_L + SELLERS_M + SELLERS_H; // set the total number of sellers
   
   // make customers: id is in form [H/M/L][seller id][customer id]
   int customernum, sellernum;
   pthread_t customerthreads[SELLERS_A * CUSTOMERS];
   
   printf("Ticket seller opens.\n");
   
   // initialize at 0 customers
   for (i = 0; i < 10; i++) {
      for (j = 0; j < CUSTOMERS; j++) {
         queue[i][j] = 0;
      }
   }
   
   // set the number of tickets sold to 0 for each seller in the L and M types
   for (i = 0; i < 6; i++) {
      lowcount[i] = 0;
   }
   for (i = 0; i < 3; i++) {
      medcount[i] = 0;
   }
   
   srand(time(NULL));
   sellerTimer.it_value.tv_sec = MINUTES;
   setitimer(ITIMER_REAL, &sellerTimer, NULL);
   time(&startTime);
   
   // make the ticket sellers
   pthread_t lthreads[SELLERS_L];
   for (i = 0; i < SELLERS_L; i++) {
      pthread_create(&lthreads[i], NULL, lowSeller, NULL);
   }
   pthread_t mthreads[SELLERS_M];
   for (i = 0; i < SELLERS_M; i++) {
      pthread_create(&mthreads[i], NULL, medSeller, NULL);
   }
   pthread_t hthreads[SELLERS_H];
   for (i = 0; i < SELLERS_H; i++) {
      pthread_create(&hthreads[i], NULL, hiSeller, NULL);
   }
   
   int arraycount = 0;
   
   // make customers
   for (sellernum = 0; sellernum < SELLERS_L * 100; sellernum += 100) { // 3 digit number
      for (customernum = 101; customernum <= 100 + CUSTOMERS; customernum++) {
         int id = 3000 + sellernum + customernum;
         pthread_create(&customerthreads[arraycount], NULL, customer, id);
         arraycount++;
      }
   }
   for (sellernum = 0; sellernum < SELLERS_M * 100; sellernum += 100) { // 3 digit number
      for (customernum = 101; customernum <= 100 + CUSTOMERS; customernum++) {
         int id = 2000 + sellernum + customernum;
         pthread_create(&customerthreads[arraycount], NULL, customer, id);
         arraycount++;
      }
   }
   for (sellernum = 0; sellernum < SELLERS_H * 100; sellernum += 100) { // 3 digit number
      for (customernum = 1; customernum <= CUSTOMERS; customernum++) {
         int id = 1000 + sellernum + customernum;
         pthread_create(&customerthreads[arraycount], NULL, customer, id);
         arraycount++;
      }
   }
   
   signal(SIGALRM, timerHandler);
   
   // wait for threads to complete
   for (i = 0; i < SELLERS_L; i++) {
      pthread_join(lthreads[i], NULL);
   }
   for (i = 0; i < SELLERS_M; i++) {
      pthread_join(mthreads[i], NULL);
   }
   for (i = 0; i < SELLERS_H; i++) {
      pthread_join(hthreads[i], NULL);
   }
   
   total_a = total_h + total_m + total_l; // set total customers who got seats
   
   printf("\n\n");
   printf("Total L Customers who got seats: %d\n", total_l); // number L customers who got a seat
   printf("Total M Customers who got seats: %d\n", total_m);
   printf("Total H Customers who got seats: %d\n", total_h);
   printf("Total Customers turned away:     %d\n", ((SELLERS_A * CUSTOMERS) - total_a)); // print out how many customers left without seats
   
   exit(0);
}

/**
  * Function for low ticket seller.
  */
void lowSeller(void *ptr) {
   pthread_mutex_lock(&low); // lock while seller creates an ID
   int id = lowNum + 1;
   lowNum++;
   pthread_mutex_unlock(&low); // unlock after ID is obtained
   
   int count = 0; // counter to remember how many customers we have served
   int current = -2;
   int i, col;
   
   while (!closed) { // run while 60 sec havent passed yet
      if(current == -2) { // find the first customer that arrived
         current = findHeadtwo(id);
      }
      else if(current == -1) {  // if -1, all customers have arrived and served (thread can finish running)
         break;
      }
      else {
         time_t now;
         time(&now);
         int min = (int) difftime(now, startTime);
         
         if (min >= queue[SELLERS_H + SELLERS_M + (id % 30) - 1][current]) { // make sure the arrival time in the queue is not in the "future", to prevent serving a customer that hasn't arrived yet
            queue[SELLERS_H + SELLERS_M + (id % 30) - 1][current] += 61; // customer is being served, remove arrival time from the queue
            current = -2;
            
            pthread_mutex_lock(&printLock); // create lock
            printf("\nTime %1d:%02d, Customer L%d%02d is now being served", min / 60, min % 60, id % 10, count + 1);
            pthread_mutex_unlock(&printLock); // unlock thread
            
            sleep((rand() % 4) + 4); // sleeps 4-7 minutes
            
            pthread_mutex_lock(&printLock); // create lock
            count++;
            
            for (i = 9; i >= 0; i--) { // find a seat for the customer
               col = seatFinder(i);
               
               if (col != -1) {
                  break;
               }
            }
            
            if (col == -1) { // if -1, all seats are taken, tickets sold out
               time_t after;
               time(&after);
               int aftertime = (int) difftime(after, startTime);
               printf("\nTime %1d:%02d, Tickets are sold out. Customer L%d%02d has left", aftertime / 60, aftertime % 60, id % 30, count);
               pthread_mutex_unlock(&printLock); // unlock thread
               break;
            }
            else {
               seat[col][i] = (id * 100) + count;
               time_t after;
               time(&after);
               int aftertime = (int) difftime(after, startTime);
               printf("\nTime %1d:%02d, Customer L%d%02d purchased a ticket and left", aftertime / 60, aftertime % 60, id % 30, count);
               total_l++; // add customer to the total of L customers that got tickets
               print();
               pthread_mutex_unlock(&printLock); // unlock thread
            }
         }
      }
   }
}

/**
  * Function for med ticket seller.
  */
void medSeller(void *ptr) {
   pthread_mutex_lock(&med);
   int id = medNum + 1;
   medNum++;
   pthread_mutex_unlock(&med);
   
   int count = 0;
   int current = -2;
   int i, col;
   
   while (!closed) {
      if (current == -2) {
         current = findHeadtwo(id);
      }
      else if (current == -1) {
         break;
      }
      else {
         time_t now;
         time(&now);
         int min = (int) difftime(now, startTime);
         
         if (min >= queue[SELLERS_H + (id % 20) - 1][current]) {
            queue[SELLERS_H + (id % 20) - 1][current] += 61;
            current = -2;
            
            pthread_mutex_lock(&printLock);
            printf("\nTime %1d:%02d, Customer M%d%02d is now being served", min / 60, min % 60, id % 20, count + 1);
            pthread_mutex_unlock(&printLock);
           
            sleep((rand() % 3) + 2); // time taken to complete ticket sales; sleeps 2-4 minutes
            
            pthread_mutex_lock(&printLock);
            count++;
            int skip = 0;
            
            for (i = 4; i >= 0; i--) {
               col = seatFinder(i);
               if (col != -1) {
                  break;
               }
               
               col = seatFinder(i + 1 + (skip * 2));
               if (col != -1) {
                  break;
               }
               
               skip++;
            }
            
            if (col == -1) {
               time_t after;
               time(&after);
               int aftertime = (int) difftime(after, startTime);
               printf("\nTime %1d:%02d, Tickets are sold out. Customer M%d%02d has left", aftertime / 60, aftertime % 60, id % 20, count);
               pthread_mutex_unlock(&printLock);
               break;
            }
            else {
               if(seat[col][i] == 0) {
                  seat[col][i] = (id * 100) + count;
               }
               else {
                  seat[col][i + 1 + (skip * 2)] = (id * 100) + count;
               }
                
               time_t after;
               time(&after);
               int aftertime = (int) difftime(after, startTime);
               printf("\nTime %1d:%02d, Customer M%d%02d purchased a ticket and left", aftertime / 60, aftertime % 60, id % 20, count);
               total_m++;
               print();
               pthread_mutex_unlock(&printLock);
            }
         }
      }
   }
}

/**
  * Function for high ticket seller.
  */
void hiSeller(void *ptr) {
   pthread_mutex_lock(&high);
   int id = highNum;
   highNum++;
   pthread_mutex_unlock(&high);
   
   int count = 0;
   int current = -2;
   int i, col;
	
   while (!closed) {
      if(current == -2) {
         current = findHeadtwo(id);
      }
      else if (current == -1) {
         break;
      }
      else {
         time_t now;
         time(&now);
         int min = (int) difftime(now, startTime);
         
         if (min >= queue[(id % 10)][current]) {
            queue[(id % 10)][current] += 61;
            current = -2;
            
            pthread_mutex_lock(&printLock);
            printf("\nTime %1d:%02d, Customer H%d%02d is now being served", min / 60, min % 60, id % 10, count + 1);
            pthread_mutex_unlock(&printLock);
            
            sleep((rand() % 2) + 1); // sleeps 1-2 minutes
            
            pthread_mutex_lock(&printLock);
            count++;
			
            for (i = 0; i < 10; i++) {
               col = seatFinder(i);
               if (col != -1) {
                  break;
               }
            }
            
            if (col == -1) {
               time_t after;
               time(&after);
               int aftertime = (int) difftime(after, startTime);
               printf("\nTime %1d:%02d, Tickets are sold out. Customer H%d%02d has left", aftertime / 60, aftertime % 60, id % 10, count);
               pthread_mutex_unlock(&printLock);
               break;
            }
            else {
               seat[col][i] = (id * 100) + count;
               time_t after;
               time(&after);
               int aftertime = (int) difftime(after, startTime);
               printf("\nTime %1d:%02d, Customer H%d%02d purchased a ticket and left", aftertime / 60, aftertime % 60, id % 10, count);
               total_h++;
               print();
               pthread_mutex_unlock(&printLock);
            }
         }
      }
   }
}

/**
  * Function to find available seats.
  */
int seatFinder(int row) { // find available seats in the given row and return its column index. return -1 if not found
   for(int i = 0; i < 10; i++) {
      if (seat[i][row] == 0) {
         return i;
      }
   }
   return -1;
}

/**
  * Timer signal handler.
  */
void timerHandler(int signal) {
	closed = 1;  // ticket seller is over
}

/*
 * Function for customer.
 * Takes pointer to character that is type of customer (L, M, H).
 */
void customer(void *ptr) {
   int id = (int) ptr; // get customer id
   int arrival = (rand() % MINUTES); // generate random arrival time
   sleep(arrival); // sleeps for the amount of arrival time
   
   if (id / 1000 == 3) { // check if the customer is low
      pthread_mutex_lock(&low);
      queue[SELLERS_H + SELLERS_M + ((id / 100) % 30) - 1][lowcount[(id / 100) % 30]] = arrival; // put arrival time in the queue
      lowcount[(id / 100) % 30]++;//counter to know which column of the queue is used
      pthread_mutex_lock(&printLock);
      time_t now;
      time(&now);
      int sec = (int) difftime(now, startTime);
      printf("\nTime %d:%02d, Customer L%d%02d arrived to L%d's station", (int) (sec / 60), (int) sec % 60,(id / 100) % 30, lowcount[(id / 100) % 30], (id / 100) % 30);
      pthread_mutex_unlock(&printLock);
      pthread_mutex_unlock(&low);
   }
   else if (id / 1000 == 2) { // check if the customer is med
      pthread_mutex_lock(&med);
      queue[SELLERS_H + ((id / 100) % 20) - 1][medcount[(id / 100) % 20]] = arrival;
      medcount[(id / 100) % 20]++;
      pthread_mutex_lock(&printLock);
      time_t now;
      time(&now);
      int sec = (int) difftime(now, startTime);
      printf("\nTime %d:%02d, Customer M%d%02d arrived to M%d's station", (int) (sec / 60), (int) sec % 60,(id / 100) % 20, medcount[(id / 100) % 20], (id / 100) % 20);
      pthread_mutex_unlock(&printLock);
      pthread_mutex_unlock(&med);
	}
   else { // check if the customer is high
      pthread_mutex_lock(&high);
      queue[((id / 100) % 10)][hicount] = arrival;
      hicount++;
      pthread_mutex_lock(&printLock);
      time_t now;
      time(&now);
      int sec = (int) difftime(now, startTime);
      printf("\nTime %d:%02d, Customer H%d%02d arrived to H%d's station", (int) (sec / 60), (int) sec % 60, (id / 100) % 10, hicount, (id / 100) % 10);
      pthread_mutex_unlock(&printLock);
      pthread_mutex_unlock(&high);
   }
}

int findHeadtwo(int id) {
   int head = -1;
   int min = 999;
   
   // check which seller requested
   if (id / 10 == 3){ // lowseller
      for (int i = 0; i < CUSTOMERS; i++) {
         if (min > queue[SELLERS_H + SELLERS_M + (id % 30) - 1][i] && queue[SELLERS_H + SELLERS_M + (id % 30) - 1][i] <= 60) { // check queue to see if value < current min and < 60 sec
            if (queue[SELLERS_H + SELLERS_M + (id % 30) - 1][i] == 0 && (head == -1 || head == -2)) {
               head = -2; // check that location in queue != 0 (arrival time is never 0). if = 0, there are customers that havent arrived
            }
            else if (queue[SELLERS_H + SELLERS_M + (id % 30) - 1][i] != 0) {
               min = queue[SELLERS_H + SELLERS_M + (id % 30) - 1][i];
               head = i;
            }
         }
      }
      return head;
   }
   else if (id / 10 == 2) { // medseller
      for (int i = 0; i < CUSTOMERS; i++) {
         if (min > queue[SELLERS_H + (id % 20) - 1][i] && queue[SELLERS_H + (id % 20) - 1][i] <= 60) {
            if (queue[SELLERS_H + (id % 20) - 1][i] == 0 && (head == -1 || head == -2)) {
               head = -2;
            }
            else if(queue[SELLERS_H+(id%20)-1][i] != 0) {
               min = queue[SELLERS_H+(id%20)-1][i];
               head = i;
            }
         }
      }
      return head;
   }
   else { // hiseller
      for (int i = 0; i < CUSTOMERS; i++) {
         if (min > queue[(id % 10)][i] && queue[(id % 10)][i] <= 60) {
            if(queue[(id % 10)][i] == 0 && (head == -1 || head == -2)) {
               head = -2;
            }
            else if(queue[(id % 10)][i] != 0) {
               min = queue[(id % 10)][i];
               head = i;
            }
         }
      }
      return head;
   }
   return -2;
}


/**
  * Print the concert seating chart in form of 10x10 matrix
  * H###, M###, L###, or ---- for unsold seats.
  */
void print() {
   time_t now;
   time(&now);
   
   int min = (int) difftime(now, startTime);
   int hour = 0;
	
   while (min >= 60) { // 60 mins passed, reset mins and increment hours for output
      min -= 60;
      hour++;
   }
   
   // display timestamp and matrix
   printf("\n\n============================================================================\n");
   printf("Info\n");
   printf("Time: %1d:%02d\n", hour, min);
   
   for (int j = 0; j < 10; j++) {
      printf("\n");
      for (int i = 0; i < 10; i++) {
         int id = seat[i][j];
         
         if (id >= 3000) {
            printf("L%d \t", id - 3000);
         }
         else if (id >= 2000) {
            printf("M%d \t", id - 2000);
         }
         else if (id >= 1000) {
            printf("H%03d \t", id - 1000);
         }
         else {
            printf("----\t"); // unsold seats
         }
      }
   }
   
   printf("\n============================================================================\n");
}