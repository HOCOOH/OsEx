#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "tty.h"
#include "console.h"
#include "string.h"
#include "fs.h"
#include "mfqs_queue.h"
#include "proc.h"
#include "global.h"
#include "proto.h"

PUBLIC int enqueue(int queue_num, int pid) {
    struct proc* p = proc_table + pid;
    struct proc_queue* q = mfqs_queue + queue_num;
    p->current_queue = queue_num;
    p->time_remain = q->time_slot;
    if (NEXT(q->front) == q->rear) {
        return -1;
    }
    q->proc_pid[q->front] = pid;
    q->front = NEXT(q->front);
    return 0;
}

PUBLIC int dequeue(int queue_num, int* p_pid) {
    struct proc_queue* q = mfqs_queue + queue_num;
    if (q->front == q->rear) {
        return -1;
    }
    *p_pid = q->proc_pid[q->rear];
    q->rear = NEXT(q->rear);
    return 0;
}
