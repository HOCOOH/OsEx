#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"

PUBLIC void inform_start() {
    MESSAGE msg;
    msg.type = PROC_START;
    send_recv(BOTH, TASK_SYS, &msg);
    assert(msg.type == SYSCALL_RET);
}

PUBLIC void inform_end() {
    MESSAGE msg;
    msg.type = PROC_END;
    send_recv(BOTH, TASK_SYS, &msg);
    assert(msg.type == SYSCALL_RET);
}

PUBLIC void dump_proc_display(int pid_begin) {
    MESSAGE msg;
    msg.type = DUMP_PROC;
	msg.PID = pid_begin;
	send_recv(BOTH, TASK_SYS, &msg);
	assert(msg.type == SYSCALL_RET);
}