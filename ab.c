#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/time.h>


//declarations.
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

int SELLERS_L = 6; // sellers low
int SELLERS_M = 3; // sellers med
int SELLERS_H = 1; // sellers high
int SELLERS_A = 10; // sellers all

int CUSTOMERS = 15; // n is 5, 10, 15
int queue[10][99]; // 0->99 = 100 seats

int lowNum = 30;
int medNum = 20;
int highNum = 10;

int total_l = 0; // ticket sold low
int total_m = 0; // ticket sold med
int total_h = 0; // ticket sold high
int total_a = 0; // ticket sold all

int closed = 0; // boolean seller closed or open
int MINUTES = 60; // 60 minutes --> 60 seconds

// threads
pthread_mutex_t printLock, customerLock;
pthread_mutex_t low;
pthread_mutex_t med;
pthread_mutex_t high;

struct itimerval profTimer;
time_t startTime;

int lowcount[6];
int medcount[3];
int hicount = 0;

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
   SELLERS_A = SELLERS_L + SELLERS_M + SELLERS_H;
   
   //make customers: id is in the form of ZXYY where Z is H/M/L and X is the seller id and YY is the customer id.
   int customernum;
   int sellernum;
   pthread_t customerthreads[SELLERS_A * CUSTOMERS];
   
   printf("Ticket seller opens.\n");
   for (i = 0; i < 10; i++) {
      for (j = 0; j < CUSTOMERS; j++) {
         queue[i][j] = 0;
      }
   }
   
   for (i = 0; i < 6; i++) {
      lowcount[i] = 0;
   }
   
   for (i = 0; i < 3; i++) {
      medcount[i] = 0;
   }
   
   srand(time(NULL));
   profTimer.it_value.tv_sec = MINUTES;
   setitimer(ITIMER_REAL, &profTimer, NULL);
   time(&startTime);
   
   // Make the ticket sellers
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
   
   total_a = total_h + total_m + total_l;
   
   printf("\n\n");
   printf("Total Customers rejected:        %d\n", ((SELLERS_A * CUSTOMERS) - total_a)); // print out how many customers left without seats
   printf("Total H Customers who got seats: %d\n", total_h); // how many H customers got their seat
   printf("Total M Customers who got seats: %d\n", total_m);
   printf("Total L Customers who got seats: %d\n", total_l);
   
   exit(0);
}

void lowSeller(void *ptr) {
   pthread_mutex_lock(&low); // lock while seller creates an ID
   int id = lowNum + 1;
   lowNum++;
   pthread_mutex_unlock(&low); // unlock after ID is obtained
   
   int count = 0; // counter to remember how many customers we have served
   int current = -2;
   int i, col;
   
   while (!closed) { // run while 60 sec havent passed yet
      if(current == -2) { // finds the first customer that arrived
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
            
            sleep((rand() % 4) + 4); // sleeps 4-7 seconds
            
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
           
            sleep((rand() % 3) + 2); // time taken to complete ticket sales
            
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
            
            sleep((rand() % 2) + 1);
            
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

int seatFinder(int row) { // find available seats in the given row and return its column index. return -1 if not found
   for(int i = 0; i < 10; i++) {
      if (seat[i][row] == 0) {
         return i;
      }
   }
   return -1;
}

// Timer signal handler.
void timerHandler(int signal) {
	closed = 1;  // office hour is over
}

/*
 * customer function.
 * RECEIVES: pointer to character that indicates the type of customer,
 * whether H, M, or L.
 */
void customer(void *ptr) {
   //get customer id
   int id = (int) ptr;
   int arrival = (rand()%MINUTES);//generate a random arrival time
   sleep(arrival); //sleeps the amount of arrival time
   
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
   else if (id / 1000 == 2) { //check if the customer is med
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
   else { //check if the customer is hi
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
   
   if (id / 10 == 3){ // check which seller requested, lowseller
      for (int i = 0; i < CUSTOMERS; i++) {
         if (min > queue[SELLERS_H + SELLERS_M + (id % 30) - 1][i] && queue[SELLERS_H + SELLERS_M + (id % 30) - 1][i] <= 60) { // check the queue if the value is less than current min and less than 60sec
            if (queue[SELLERS_H + SELLERS_M + (id % 30) - 1][i] == 0 && (head == -1 || head == -2)) {
               head = -2; // check that the location in the queue is not equals 0 since arrival time will never be 0, if equals 0 it means that there are still customes that havent arrived
            }
            else if (queue[SELLERS_H + SELLERS_M + (id % 30) - 1][i] != 0) {
               min = queue[SELLERS_H + SELLERS_M + (id % 30) - 1][i];
               head = i;
            }
         }
      }
      return head;
   }
	
   if (id / 10 == 2) { // check which seller requested, medseller
      for (int i = 0; i < CUSTOMERS; i++) {
         if (min > queue[SELLERS_H + (id % 20) - 1][i] && queue[SELLERS_H + (id % 20) - 1][i] <= 60) {
            if (queue[SELLERS_H + (id % 20) - 1][i] == 0 && (head == -1 || head == -2)) {
               head = -2;
            }
            else if(queue[SELLERS_H+(id%20)-1][i] != 0){
               min = queue[SELLERS_H+(id%20)-1][i];
               head = i;
            }
         }
      }
      return head;
   }
   else { // check which seller requested, hiseller
      for (int i = 0; i < CUSTOMERS; i++) {
         if (min > queue[(id % 10)][i] && queue[(id % 10)][i] <= 60) {
            if(queue[(id % 10)][i] == 0 && (head == -1 || head == -2)) {
               head = -2;
            }
            else if(queue[(id % 10)][i] != 0){
               min = queue[(id % 10)][i];
               head = i;
            }
         }
      }
      return head;
   }
   return -2;
}


void print() {
   time_t now;
   time(&now);
   
   int min = (int) difftime(now, startTime);
   int hour = 0;
	
   while (min >= 60) {
      min -= 60;
      hour++;
   }
   
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
            printf("____\t");
         }
      }
   }
   printf("\n============================================================================\n");
}