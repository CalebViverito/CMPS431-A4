#ifndef JOBQUEUE_H
#define JOBQUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef enum {OUTPUT, INPUT} Type;

typedef struct JobNode{
    int jid;
    int stockId;
    int workerId;
    int burst;
    int value;
    int bumps;
    Type type;
    struct JobNode *next;
} JobNode;

void JobNode_init(JobNode *j, int jid, int stockId, int value, Type type);

typedef struct JobQueue {
    JobNode *head;
    JobNode *tail;
} JobQueue;

void JobQueue_init(JobQueue *jQueue);
void JobQueue_push(JobQueue *jQueue, JobNode *j);
JobNode *JobQueue_pop(JobQueue *jQueue);
void JobQueue_destroy(JobQueue *jQueue);

#endif