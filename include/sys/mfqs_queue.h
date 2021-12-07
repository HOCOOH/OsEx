#ifndef	_ORANGES_MFQS_QUEUE_H_
#define	_ORANGES_MFQS_QUEUE_H_

#define NEXT(x) ((x + 1) % NR_PROC_IN_QUEUE)
#define PREVIOUS(x) ((x - 1 + NR_PROC_IN_QUEUE) % NR_PROC_IN_QUEUE)

struct proc_queue {
    int proc_pid[NR_PROC_IN_QUEUE];
    int front;
    int rear;
    int time_slot;
};


#endif