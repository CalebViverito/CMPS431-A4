#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "JobQueue.h"

void JobNode_init(JobNode *j, int jid, int stockId, int value, Type type) {
    j->jid = jid;
    j->stockId = stockId;
    j->workerId = 0;
    j->burst = 1;
    j->value = value;
    j->bumps = 0;
    j->type = type;
    j->next = NULL;
}

void JobQueue_init(JobQueue *jQueue) {
    jQueue->head = NULL;
    jQueue->tail = NULL;
}

void JobQueue_push(JobQueue *jQueue, JobNode *j) {
    j->next = NULL;
    
    if(jQueue->head == NULL) {
        jQueue->head = j;
        jQueue->tail = j;
    }
    else {
        jQueue->tail->next = j;
        jQueue->tail = j;
    }
}

JobNode *JobQueue_pop(JobQueue *jQueue) {
    if(jQueue->head == NULL) {
        return NULL;
    }
    
    JobNode *j = jQueue->head;
    jQueue->head = j->next;
    
    if(jQueue->head == NULL) {
        jQueue->tail = NULL;
    }
    j->next = NULL;
    
    return j;
}

void JobQueue_destroy(JobQueue *jQueue) {
    JobNode *curr = jQueue->head;
    
    while (curr != NULL) {
        JobNode *next = curr->next;
        free(curr);
        curr = next;
    }
    jQueue->head = NULL;
    jQueue->tail = NULL;
}
