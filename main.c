#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "JobQueue.h"

int stock[] = {11, 20, 5, 1, 8, 12};
pthread_mutex_t stockLock[6];
pthread_mutex_t all = PTHREAD_MUTEX_INITIALIZER;
int incompleteJobs = 0;
int bumpedJobs = 0;
struct timespec Start;

double get_ms() {
    // func keeps track of ms elapsed
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    double ms = (now.tv_sec - Start.tv_sec) * 1000 + (now.tv_nsec - Start.tv_nsec) / 1.0e6;
    return ms;
}

void updateStock(JobQueue *jQueue, JobNode *j) {
    if (j == NULL) {
        return;
    }
    pthread_mutex_lock(&stockLock[j->stockId]);
    // add to stock
    if (j->type == INPUT) {
        pthread_mutex_lock(&all);
        printf("job %.2d BEFORE | stock quantity %i - input val %i\n", j->jid, stock[j->stockId], j->value);
        pthread_mutex_unlock(&all);
        
        stock[j->stockId] = stock[j->stockId] + j->value;
        
        pthread_mutex_lock(&all);
        printf("job %.2d AFTER  | stock quantity %i - input val %i\n\n", j->jid, stock[j->stockId], j->value);
        pthread_mutex_unlock(&all);
    }
    else {
        // can't remove, stock goes negative
        if((stock[j->stockId] - j->value) < 0) {
            pthread_mutex_lock(&all);
            printf("job %.2d        | bumped for insufficient stock\n\n", j->jid);
            if (j->bumps == 0) {
                bumpedJobs++;
            }
            j->bumps++;
            JobQueue_push(jQueue, j);
            pthread_mutex_unlock(&all);
            pthread_mutex_unlock(&stockLock[j->stockId]);
            return;
        }
        // remove from stock
        pthread_mutex_lock(&all);
        printf("job %.2d BEFORE | stock quantity %i - output value %i\n", j->jid, stock[j->stockId], j->value);
        pthread_mutex_unlock(&all);
        
        stock[j->stockId] = stock[j->stockId] - j->value;
        
        pthread_mutex_lock(&all);
        printf("job %.2d AFTER  | stock quantity %i - output value %i\n\n", j->jid, stock[j->stockId], j->value);
        pthread_mutex_unlock(&all);
        
    }
    pthread_mutex_unlock(&stockLock[j->stockId]);
}

void *takeJob(void *arg) {
    pthread_mutex_lock(&all);
    JobQueue *jQueue = (JobQueue *)arg;
    JobNode *j;
    int workerId = 0;
    pthread_mutex_unlock(&all);
    
    while(true){
        pthread_mutex_lock(&all);
        j = JobQueue_pop(jQueue);
        
        if (j == NULL){
            pthread_mutex_unlock(&all);
            break;
        }
        if (j->bumps > 2) {
            printf("job %.2d        | cancelled due to lack of progress\n\n", j->jid);
            incompleteJobs++;
            pthread_mutex_unlock(&all);
            continue;
        }
        else {
            j->workerId = pthread_self();
            printf("job %.2d        | taken by worker %i\n", j->jid, j->workerId);
            pthread_mutex_unlock(&all);
        }
        updateStock(jQueue, j);
        usleep(10);
    }
}

int main() {
    pthread_t threadId[3];
    int threadCount = 0;
    
    JobQueue jQueue;
    JobQueue_init(&jQueue);
    int stockId[] = {0, 3, 1, 2, 3, 5, 4, 0, 1, 5, 2, 4, 0, 3, 0};
    int value[] = {5, 3, 10, 3, 8, 10, 4, 3, 8, 5, 6, 5, 12, 5, 8};
    Type type[] = {OUTPUT, OUTPUT, OUTPUT, INPUT, INPUT, OUTPUT, INPUT, INPUT, INPUT, OUTPUT, INPUT, INPUT, OUTPUT, INPUT, INPUT};
    int jid = 0;
    
    for (int i = 0; i < 6; i++) {
        pthread_mutex_init(&stockLock[i], NULL);
    }
    
    for (int i = 0; i < 15; i++) {
        JobNode *j = malloc(sizeof(JobNode));
        JobNode_init(j, jid++, stockId[i], value[i], type[i]);
        JobQueue_push(&jQueue, j);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &Start);
    
    // create thread pool
    for (int i = 0; i < 3; i++) {
        pthread_create(&threadId[i], NULL, takeJob, &jQueue);
    }
    
    // join threads after jobs are finished
    for (int i = 0; i < 3; i++) {
        pthread_join(threadId[i], NULL);
    }
    float endTime = get_ms();
    
    char id = 'A';
    printf("\nFinal Stock Count\n");
    for (int i = 0; i < 6; i++) {
        printf("| %c | %.2d |\n", id++, stock[i]);
    }
    printf("Number of operations bumped for insufficient stock: %i\n", bumpedJobs);
    printf("Number of operations cancelled for lack of stock: %i\n", incompleteJobs);
    printf("Total time elapsed: %.5f ms\n", endTime);
    printf("Average completion time: %.5f ms", (endTime / 6));
    
    
    return 0;
}