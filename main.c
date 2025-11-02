#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "JobQueue.h"

int stock[] = {11, 20, 5, 1, 8, 12};
pthread_mutex_t stockLock[6] = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t all = PTHREAD_MUTEX_INITIALIZER;
int incompleteJobs = 0;
int bumpedJobs = 0;

void updateStock(JobQueue *jQueue, JobNode *j) {
    if (j == NULL) {
        return;
    }
    
    // add to stock
    if (j->type == INPUT) {
        pthread_mutex_lock(&stockLock[j->stockId]);
        printf("BEFORE | stock quantity %i - input val %i - jobId %i\n", stock[j->stockId], j->value, j->jid);
        stock[j->stockId] = stock[j->stockId] + j->value;
        printf("AFTER  | stock quantity %i - input val %i - jobId %i\n\n", stock[j->stockId], j->value, j->jid);
        pthread_mutex_unlock(&stockLock[j->stockId]);
    }
    else {
        // can't remove, stock goes
        if((stock[j->stockId] - j->value) < 0) {
            pthread_mutex_lock(&stockLock[j->stockId]);
            printf("job %i bumped for insufficient stock\n\n", j->jid);
            pthread_mutex_lock(&all);
            JobQueue_push(jQueue, j);
            if (j->bumps == 0) {
                bumpedJobs++;
            }
            pthread_mutex_unlock(&all);
            j->bumps++;
            pthread_mutex_unlock(&stockLock[j->stockId]);
            return;
        }
        // remove from stock
        pthread_mutex_lock(&stockLock[j->stockId]);
        printf("BEFORE | stock quantity %i - output val %i - jobId %i\n", stock[j->stockId], j->value, j->jid);
        stock[j->stockId] = stock[j->stockId] - j->value;
        printf("AFTER  | stock quantity %i - output val %i - jobId %i\n\n", stock[j->stockId], j->value, j->jid);
        pthread_mutex_unlock(&stockLock[j->stockId]);
        
    }
}

void *takeJob(void *arg) {
    pthread_mutex_lock(&all);
    JobQueue *jQueue = (JobQueue *)arg;
    JobNode *j;
    int workerId = 0;
    pthread_mutex_unlock(&all);
    
    while((j = JobQueue_pop(jQueue)) != NULL){
        pthread_mutex_lock(&all);
        if (j->bumps > 1) {
            printf("job %i cancelled due to lack of progress\n\n", j->jid);
            incompleteJobs++;
            pthread_mutex_unlock(&all);
            continue;
        }
        else {
            j->workerId = pthread_self();
            printf("job %i, taken by worker %i\n", j->jid, j->workerId);
        }
        pthread_mutex_unlock(&all);
        updateStock(jQueue, j);
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
    
    for (int i = 0; i < 15; i++) {
        JobNode *j = malloc(sizeof(JobNode));
        JobNode_init(j, jid++, stockId[i], value[i], type[i]);
        JobQueue_push(&jQueue, j);
    }
    
    // create thread pool
    for (int i = 0; i < 3; i++) {
        pthread_create(&threadId[i], NULL, takeJob, &jQueue);
    }
    
    // join threads after jobs are finished
    for (int i = 0; i < 3; i++) {
        pthread_join(threadId[i], NULL);
    }
    
    char id = 'A';
    printf("\nFinal Stock Count\n");
    for (int i = 0; i < 6; i++) {
        printf("| %c | %.2d |\n", id++, stock[i]);
    }
    printf("Number of operations bumped for insufficient stock: %i\n", bumpedJobs);
    printf("Number of operations cancelled for lack of stock: %i", incompleteJobs);
    
    return 0;
}