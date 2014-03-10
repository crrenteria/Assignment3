#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>

#define L_SELLERS 6 //Number l sellers
#define M_SELLERS 3 //Number M sellers
#define H_SELLERS 1 //Number H sellers
#define ALL_SELLERS 10 //Total number of sellers
#define MINUTES 60 //selling duration

int l_Total = 0; //Tickets sold from l
int m_Total = 0; //Tickets sold form m
int h_Total = 0; //TIckets sold from h
int all_Total = 0; //Total tickets sold

int closed = 0; // boolean seller closed or open

int queue[10][99];

int lNum = 30;
int mNum = 20;
int hNum = 10;
int numCustomers; //Command line argument

pthread_mutex_t l, m, h; //mutexes for l m and h sellers
pthread_mutex_t printLock, customerLock; //mutexes for print and customer

struct itimerval sellerTimer;
time_t startTime;

int lCount[6];
int mCount[3];
int hCount = 0;

int seat[10][10] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
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

void print() {
    time_t now;
    time(&now);
    
    int min = (int) difftime(now, startTime);
    int hour = 0;
	
    while (min >= MINUTES) {
        min -= 60;
        hour++;
    }
    
    
    printf("\n\n----------------------------------------------------------------------------\n");
    printf("Timer- %1d:%02d\n", hour, min);
    
    int i, j;
    for (j = 0; j < 10; j++) {
        printf("\n");
        for (i = 0; i < 10; i++) {
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
                printf("----\t");
            }
        }
    }
}

void l_Seller(void *ptr) {
    pthread_mutex_lock(&l);
    int id = lNum + 1;
    lNum++;
    pthread_mutex_unlock(&l);
    
    int count = 0;
    int current = -2;
    int i, col;
    
    while (!closed) {
        if(current == -2) {
            current = findHeadtwo(id);
        }
        else if(current == -1) {
            break;
        }
        else {
            time_t now;
            time(&now);
            int min = (int) difftime(now, startTime);
            
            if (min >= queue[H_SELLERS + M_SELLERS + (id % 30) - 1][current]) {
                
                queue[H_SELLERS + M_SELLERS + (id % 30) - 1][current] += 61;
                
                current = -2;
                
                pthread_mutex_lock(&printLock);
                printf("\nTimer- %1d:%02d, Customer L%d%02d is now being served", min / 60, min % 60, id % 10, count + 1);
                pthread_mutex_unlock(&printLock);
                
                sleep((rand() % 4) + 4);
                
                pthread_mutex_lock(&printLock);
                count++;
                
                for (i = 9; i >= 0; i--) {
                    col = seatFinder(i);
                    
                    if (col != -1) {
                        break;
                    }
                }
                
                if (col == -1) {
                    time_t after;
                    time(&after);
                    int aftertime = (int) difftime(after, startTime);
                    printf("\nTimer- %1d:%02d, Tickets are sold out. Customer L%d%02d has left", aftertime / 60, aftertime % 60, id % 30, count);
                    pthread_mutex_unlock(&printLock);
                    break;
                }
                else {
                    seat[col][i] = (id * 100) + count;
                    time_t after;
                    time(&after);
                    int aftertime = (int) difftime(after, startTime);
                    printf("\nTimer- %1d:%02d, Customer L%d%02d purchased a ticket and left", aftertime / 60, aftertime % 60, id % 30, count);
                    l_Total++;
                    print();
                    pthread_mutex_unlock(&printLock);
                }
            }
        }
    }
}

//M seller
void m_Seller(void *ptr) {
    pthread_mutex_lock(&m);
    int id = mNum + 1;
    mNum++;
    pthread_mutex_unlock(&m);
    
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
            
            if (min >= queue[H_SELLERS + (id % 20) - 1][current]) {
                queue[H_SELLERS + (id % 20) - 1][current] += 61;
                current = -2;
                
                pthread_mutex_lock(&printLock);
                printf("\nTimer- %1d:%02d, Customer M%d%02d is now being served", min / 60, min % 60, id % 20, count + 1);
                pthread_mutex_unlock(&printLock);
                
                sleep((rand() % 3) + 2);
                
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
                    printf("\nTimer- %1d:%02d, Tickets are sold out. Customer M%d%02d has left", aftertime / 60, aftertime % 60, id % 20, count);
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
                    printf("\nTimer- %1d:%02d, Customer M%d%02d purchased a ticket and left", aftertime / 60, aftertime % 60, id % 20, count);
                    m_Total++;
                    print();
                    pthread_mutex_unlock(&printLock);
                }
            }
        }
    }
}

//H seller
void h_Seller(void *ptr) {
    pthread_mutex_lock(&h);
    int id = hNum;
    hNum++;
    pthread_mutex_unlock(&h);
    
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
                printf("\nTimer- %1d:%02d, Customer H%d%02d is now being served", min / 60, min % 60, id % 10, count + 1);
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
                    printf("\nTimer- %1d:%02d, Tickets are sold out. Customer H%d%02d has left", aftertime / 60, aftertime % 60, id % 10, count);
                    pthread_mutex_unlock(&printLock);
                    break;
                }
                else {
                    seat[col][i] = (id * 100) + count;
                    time_t after;
                    time(&after);
                    int aftertime = (int) difftime(after, startTime);
                    printf("\nTimer- %1d:%02d, Customer H%d%02d purchased a ticket and left", aftertime / 60, aftertime % 60, id % 10, count);
                    h_Total++;
                    print();
                    pthread_mutex_unlock(&printLock);
                }
            }
        }
    }
}

int seatFinder(int row) {
    int i;
    for(i = 0; i < 10; i++) {
        if (seat[i][row] == 0) {
            return i;
        }
    }
    return -1;
}

void timerHandler(int signal) {
	closed = 1;
}

void customer(void *ptr) {
    int id = (int) ptr;
    int arrival = (rand() % MINUTES);
    sleep(arrival);
    
    if (id / 1000 == 3) {
        pthread_mutex_lock(&l);
        queue[H_SELLERS + M_SELLERS + ((id / 100) % 30) - 1][lCount[(id / 100) % 30]] = arrival;
        lCount[(id / 100) % 30]++;
        pthread_mutex_lock(&printLock);
        time_t now;
        time(&now);
        int sec = (int) difftime(now, startTime);
        printf("\nTimer- %d:%02d, Customer L%d%02d arrived to L%d's station", (int) (sec / 60), (int) sec % 60,(id / 100) % 30, lCount[(id / 100) % 30], (id / 100) % 30);
        pthread_mutex_unlock(&printLock);
        pthread_mutex_unlock(&l);
    }
    else if (id / 1000 == 2) {
        pthread_mutex_lock(&m);
        queue[H_SELLERS + ((id / 100) % 20) - 1][mCount[(id / 100) % 20]] = arrival;
        mCount[(id / 100) % 20]++;
        pthread_mutex_lock(&printLock);
        time_t now;
        time(&now);
        int sec = (int) difftime(now, startTime);
        printf("\nTimer- %d:%02d, Customer M%d%02d arrived to M%d's station", (int) (sec / 60), (int) sec % 60,(id / 100) % 20, mCount[(id / 100) % 20], (id / 100) % 20);
        pthread_mutex_unlock(&printLock);
        pthread_mutex_unlock(&m);
	}
    else {
        pthread_mutex_lock(&h);
        queue[((id / 100) % 10)][hCount] = arrival;
        hCount++;
        pthread_mutex_lock(&printLock);
        time_t now;
        time(&now);
        int sec = (int) difftime(now, startTime);
        printf("\nTimer- %d:%02d, Customer H%d%02d arrived to H%d's station", (int) (sec / 60), (int) sec % 60, (id / 100) % 10, hCount, (id / 100) % 10);
        pthread_mutex_unlock(&printLock);
        pthread_mutex_unlock(&h);
    }
}



int findHeadtwo(int id) {
    int head = -1;
    int min = 999;
    int i;
    
    if (id / 10 == 3){
        for (i = 0; i < numCustomers; i++) {
            if (min > queue[H_SELLERS + M_SELLERS + (id % 30) - 1][i] && queue[H_SELLERS + M_SELLERS + (id % 30) - 1][i] <= 60) {
                if (queue[H_SELLERS + M_SELLERS + (id % 30) - 1][i] == 0 && (head == -1 || head == -2)) {
                    head = -2;
                }
                else if (queue[H_SELLERS + M_SELLERS + (id % 30) - 1][i] != 0) {
                    min = queue[H_SELLERS + M_SELLERS + (id % 30) - 1][i];
                    head = i;
                }
            }
        }
        return head;
    }
    else if (id / 10 == 2) {
        for (i = 0; i < numCustomers; i++) {
            if (min > queue[H_SELLERS + (id % 20) - 1][i] && queue[H_SELLERS + (id % 20) - 1][i] <= 60) {
                if (queue[H_SELLERS + (id % 20) - 1][i] == 0 && (head == -1 || head == -2)) {
                    head = -2;
                }
                else if(queue[H_SELLERS+(id%20)-1][i] != 0) {
                    min = queue[H_SELLERS+(id%20)-1][i];
                    head = i;
                }
            }
        }
        return head;
    }
    else {
        for (i = 0; i < numCustomers; i++) {
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

int i, j;


int main(int argc, char *argv[]) {
    
    if (argc == 2) {
        numCustomers = atoi(argv[1]);
    }
    else {
        printf("Usage: requires 1 integer\n");
        exit(-1);
    }
    
    int customerNum, sellerNum;
    pthread_t customerThreads[ALL_SELLERS * atoi(argv[1])];
    
    printf("Ticket seller opens.\n");
    
    for (i = 0; i < 10; i++) {
        for (j = 0; j < atoi(argv[1]); j++) {
            queue[i][j] = 0;
        }
    }
    
    for (i = 0; i < 6; i++) {
        lCount[i] = 0;
    }
    for (i = 0; i < 3; i++) {
        mCount[i] = 0;
    }
    
    srand(time(NULL));
    sellerTimer.it_value.tv_sec = MINUTES;
    setitimer(ITIMER_REAL, &sellerTimer, NULL);
    time(&startTime);
    
    //Seller's threads
    pthread_t l_Threads[L_SELLERS];
    for (i = 0; i < L_SELLERS; i++) {
        pthread_create(&l_Threads[i], NULL, l_Seller, NULL);
    }
    pthread_t m_Threads[M_SELLERS];
    for (i = 0; i < M_SELLERS; i++) {
        pthread_create(&m_Threads[i], NULL, m_Seller, NULL);
    }
    pthread_t h_Threads[H_SELLERS];
    for (i = 0; i < H_SELLERS; i++) {
        pthread_create(&h_Threads[i], NULL, h_Seller, NULL);
    }
    
    int arrCount = 0;
    
    //Customer threads
    for (sellerNum = 0; sellerNum < L_SELLERS * 100; sellerNum += 100) {
        for (customerNum = 101; customerNum <= 100 + atoi(argv[1]); customerNum++) {
            int id = 3000 + sellerNum + customerNum;
            pthread_create(&customerThreads[arrCount], NULL, customer, id);
            arrCount++;
        }
    }
    for (sellerNum = 0; sellerNum < M_SELLERS * 100; sellerNum += 100) {
        for (customerNum = 101; customerNum <= 100 + atoi(argv[1]); customerNum++) {
            int id = 2000 + sellerNum + customerNum;
            pthread_create(&customerThreads[arrCount], NULL, customer, id);
            arrCount++;
        }
    }
    for (sellerNum = 0; sellerNum < H_SELLERS * 100; sellerNum += 100) {
        for (customerNum = 1; customerNum <= atoi(argv[1]); customerNum++) {
            int id = 1000 + sellerNum + customerNum;
            pthread_create(&customerThreads[arrCount], NULL, customer, id);
            arrCount++;
        }
    }
    
    signal(SIGALRM, timerHandler);
    
    
    for (i = 0; i < L_SELLERS; i++) {
        pthread_join(l_Threads[i], NULL);
    }
    for (i = 0; i < M_SELLERS; i++) {
        pthread_join(m_Threads[i], NULL);
    }
    for (i = 0; i < H_SELLERS; i++) {
        pthread_join(h_Threads[i], NULL);
    }
    
    all_Total = h_Total + m_Total + l_Total;
    
    printf("\n\n");
    printf("L's customers who got seats: %d\n", l_Total);
    printf("M's customers who got seats: %d\n", m_Total);
    printf("H's customers who got seats: %d\n", h_Total);
    printf("Customers turned away:     %d\n", ((ALL_SELLERS * atoi(argv[1])) - all_Total));
    
    return 0;
}
