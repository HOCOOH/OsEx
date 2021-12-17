#ifndef	_ORANGES_MFQS_QUEUE_H_
#define	_ORANGES_MFQS_QUEUE_H_

#define NEXT(x) (((x) + 1) % NR_PROC_IN_QUEUE)
#define PREVIOUS(x) (((x) - 1 + NR_PROC_IN_QUEUE) % NR_PROC_IN_QUEUE)

#define TASK_QUEUE 0
#define INIT_QUEUE 1
#define PROC_QUEUE 2
#define pid2qid(x) (((x) < NR_TASKS) ? TASK_QUEUE : \
    (((x) == INIT) ? INIT_QUEUE : PROC_QUEUE))

#define NEXT_QUEUE(x) ((((x) > INIT_QUEUE) && ((x) < NR_PROC_QUEUE - 1)) ? (x + 1) : (x))

#define TICKS_DEFAULT 0
// #define TICKS_BLOCK(p) (((p)->current_queue ? (p)->ticks : TICKS_DEFAULT))

struct proc_queue {
    int proc_pid[NR_PROC_IN_QUEUE]; // 进程id
    int front;      // 队列头
    int rear;       // 队列尾                
    int time_slot;  // 队列默认时间片
};


#endif