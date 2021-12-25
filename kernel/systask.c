/*************************************************************************//**
 *****************************************************************************
 * @file   systask.c
 * @brief  
 * @author Forrest Y. Yu
 * @date   2007
 *****************************************************************************
 *****************************************************************************/

#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "mfqs_queue.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

PRIVATE int read_register(char reg_addr);
PRIVATE u32 get_rtc_time(struct time *t);
PRIVATE void do_dump_proc(int pid_begin);
PRIVATE int do_is_finish(int pid_begin);

/*****************************************************************************
 *                                task_sys
 *****************************************************************************/
/**
 * <Ring 1> The main loop of TASK SYS.
 * 
 *****************************************************************************/
PUBLIC void task_sys()
{
	MESSAGE msg;
	struct time t;

	while (1) {
		send_recv(RECEIVE, ANY, &msg);
		int src = msg.source;

		switch (msg.type) {
		case GET_TICKS:
			msg.RETVAL = ticks;
			send_recv(SEND, src, &msg);
			break;
		case GET_PID:
			msg.type = SYSCALL_RET;
			msg.PID = src;
			send_recv(SEND, src, &msg);
			break;
		case GET_RTC_TIME:
			msg.type = SYSCALL_RET;
			get_rtc_time(&t);
			phys_copy(va2la(src, msg.BUF),
				  va2la(TASK_SYS, &t),
				  sizeof(t));
			send_recv(SEND, src, &msg);
			break;
		case PROC_START:
			msg.type = SYSCALL_RET;
			proc_table[src].start_time = ticks;
			send_recv(SEND, src, &msg);
			break;
		case PROC_END:
			msg.type = SYSCALL_RET;
			proc_table[src].end_time = ticks;
			send_recv(SEND, src, &msg);
			break;
		case IS_FINISH:
			msg.RETVAL = do_is_finish(msg.PID);
			msg.type = SYSCALL_RET;
			send_recv(SEND, src, &msg);
			break;
		case DUMP_PROC:
			do_dump_proc(msg.PID);
			msg.type = SYSCALL_RET;
			send_recv(SEND, src, &msg);
			break;
		// case PRINT_FILE:
		// 	do_print_file();
		// 	msg.type = SYSCALL_RET;
		// 	send_recv(SEND, src, &msg);
		// 	break;
		default:
			panic("unknown msg type");
			break;
		}
	}
}

// PUBLIC int do_print_file() {
// 	// open
// 	MESSAGE msg;
// 	char pathname[20] = "files";

// 	msg.type	= OPEN;
// 	msg.PATHNAME	= (void*)pathname;
// 	msg.FLAGS	= O_CREAT | O_RDWR;
// 	msg.NAME_LEN	= strlen(pathname);

// 	send_recv(BOTH, TASK_FS, &msg);
	
// 	assert(msg.type == SYSCALL_RET);
// 	assert(msg.FD != -1);

// 	return 0;
// }


/*****************************************************************************
 *                                get_rtc_time
 *****************************************************************************/
/**
 * Get RTC time from the CMOS
 * 
 * @return Zero.
 *****************************************************************************/
PRIVATE u32 get_rtc_time(struct time *t)
{
	t->year = read_register(YEAR);
	t->month = read_register(MONTH);
	t->day = read_register(DAY);
	t->hour = read_register(HOUR);
	t->minute = read_register(MINUTE);
	t->second = read_register(SECOND);

	if ((read_register(CLK_STATUS) & 0x04) == 0) {
		/* Convert BCD to binary (default RTC mode) */
		t->year = BCD_TO_DEC(t->year);
		t->month = BCD_TO_DEC(t->month);
		t->day = BCD_TO_DEC(t->day);
		t->hour = BCD_TO_DEC(t->hour);
		t->minute = BCD_TO_DEC(t->minute);
		t->second = BCD_TO_DEC(t->second);
	}

	t->year += 2000;

	return 0;
}

/*****************************************************************************
 *                                read_register
 *****************************************************************************/
/**
 * Read register from CMOS.
 * 
 * @param reg_addr 
 * 
 * @return 
 *****************************************************************************/
PRIVATE int read_register(char reg_addr)
{
	out_byte(CLK_ELE, reg_addr);
	return in_byte(CLK_IO);
}

PRIVATE int do_is_finish(int pid_begin) {
	int i; 
	for (i = pid_begin; i < pid_begin + NR_PROC_TEST; i++) {
		if (!(proc_table[i].p_flags & HANGING)) {
			return 0;
		}
	}

	return 1;
}


PRIVATE void do_dump_proc(int pid_begin) {
	printl("|----------------------------------------------------------------------|\n");
	printl("|  proc_name  | eval | arrive | start |  end  | cycle | weighted_cycle |\n");

	int i;
	struct proc* p;
	int sum = 0;
	int weighted_sum = 0;
	int delay_sum[NR_PROC_TEST] = {23, 9, 57, 33, 26};
	for (i = 0; i < NR_PROC_TEST; i++) {
		delay_sum[i] *= 5;
		p = proc_table + pid_begin + i;
		assert(p->p_flags & HANGING);
		sum += p->end_time - p->arrive_time;
		weighted_sum += (p->end_time - p->arrive_time) / delay_sum[i];
		printl("|----------------------------------------------------------------------|\n");
		printl("| %10s  | %4d | %5d  | %5d | %5d | %5d | %9d      |\n", p->name, delay_sum[i], p->arrive_time, \
			p->start_time, p->end_time, p->end_time - p->arrive_time, (p->end_time - p->arrive_time) / delay_sum[i]);
	}
	printl("|----------------------------------------------------------------------|\n");
	printl("| average cycle time: %5d                                          |\n", sum / NR_PROC_TEST);
	printl("|----------------------------------------------------------------------|\n");
	printl("| weighted average cycle time: %5d                                 |\n", weighted_sum / NR_PROC_TEST);
	printl("|----------------------------------------------------------------------|\n");
}