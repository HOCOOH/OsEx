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

PUBLIC int enqueue(int queue_num, int pid, int time_remain) {
    struct proc* p = proc_table + pid;
    struct proc_queue* q = mfqs_queue + queue_num;
    p->current_queue = queue_num;
    if (time_remain) {
        p->time_remain = time_remain;
    }
    else {
        p->time_remain = q->time_slot;
    }
    
    if (NEXT(q->rear) == q->front) {
        return -1;
    }
    q->proc_pid[q->rear] = pid;
    q->rear = NEXT(q->rear);
    return 0;
}

PUBLIC int dequeue(int queue_num, int* p_pid) {
    struct proc_queue* q = mfqs_queue + queue_num;
    if (q->front == q->rear) {
        return -1;
    }
    *p_pid = q->proc_pid[q->front];
    q->front = NEXT(q->front);
    return 0;
}

PUBLIC int remove(int pid) {
    int queue_id = proc_table[pid].current_queue;
    struct proc_queue* q = mfqs_queue + queue_id;
    int i;
    for (i = q->front; i != q->rear; i = NEXT(i)) {
        if (q->proc_pid[i] == pid) {
            break;
        }
    }
    if (i != q->rear) {
        assert(pid == q->proc_pid[i]);
        for ( ; i != q->front; i = PREVIOUS(i)) {
            q->proc_pid[i] = q->proc_pid[PREVIOUS(i)];
        }
        q->front = NEXT(q->front);
        return 0;
    }
    return 1;
}