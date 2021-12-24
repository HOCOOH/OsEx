
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

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
#include "proto.h"


/*****************************************************************************
 *                               kernel_main
 *****************************************************************************/
/**
 * jmp from kernel.asm::_start. 
 * 
 *****************************************************************************/
PUBLIC int kernel_main()
{
	disp_str("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
		 "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

	int i, j, eflags, prio;
        u8  rpl;
        u8  priv; /* privilege */

	struct task * t;
	struct proc * p = proc_table;

	char * stk = task_stack + STACK_SIZE_TOTAL;

	/* proc schudule */
#ifdef PROC_DISPLAY
	mfqs_queue[0].time_slot = 20;
	mfqs_queue[1].time_slot = 20;
	mfqs_queue[2].time_slot = 30;
	mfqs_queue[3].time_slot = 50;
	mfqs_queue[4].time_slot = 60;
#else
	mfqs_queue[0].time_slot = 20;
	mfqs_queue[1].time_slot = 20;
	mfqs_queue[2].time_slot = 2;
	mfqs_queue[3].time_slot = 4;
	mfqs_queue[4].time_slot = 8;
#endif

	int k = 0;
	for (; k < NR_PROC_QUEUE; k++) {
		mfqs_queue[k].front = 0;
		mfqs_queue[k].rear = 0;
	}

	for (i = 0; i < NR_TASKS + NR_PROCS; i++,p++,t++) {
		if (i >= NR_TASKS + NR_NATIVE_PROCS) {
			p->p_flags = FREE_SLOT;
			continue;
		}

		if (i < NR_TASKS) {     /* TASK */
			t	= task_table + i;
			priv	= PRIVILEGE_TASK;
			rpl     = RPL_TASK;
			eflags  = 0x1202;/* IF=1, IOPL=1, bit 2 is always 1 */
			prio    = 15;
		} 
		else {                  /* USER PROC */
			t	= user_proc_table + (i - NR_TASKS);
			priv	= PRIVILEGE_USER;
			rpl     = RPL_USER;
			eflags  = 0x202;	/* IF=1, bit 2 is always 1 */
			prio    = 5;
		}

		strcpy(p->name, t->name);	/* name of the process */
		p->p_parent = NO_TASK;

		enqueue(pid2qid(i), i, TICKS_DEFAULT);
		/* only for display */
		p->arrive_time = 0;
		p->start_time = 0;
		p->end_time = 0;

		if (strcmp(t->name, "INIT") != 0 && strcmp(t->name, "TestA") != 0) {
		// if (i != 5) {
			p->ldts[INDEX_LDT_C]  = gdt[SELECTOR_KERNEL_CS >> 3];
			p->ldts[INDEX_LDT_RW] = gdt[SELECTOR_KERNEL_DS >> 3];

			/* change the DPLs */
			p->ldts[INDEX_LDT_C].attr1  = DA_C   | priv << 5;
			p->ldts[INDEX_LDT_RW].attr1 = DA_DRW | priv << 5;
		}
		else {		/* INIT process */
			unsigned int k_base;
			unsigned int k_limit;
			int ret = get_kernel_map(&k_base, &k_limit);
			assert(ret == 0);
			init_desc(&p->ldts[INDEX_LDT_C],
				  0, /* bytes before the entry point
				      * are useless (wasted) for the
				      * INIT process, doesn't matter
				      */
				  (k_base + k_limit) >> LIMIT_4K_SHIFT,
				  DA_32 | DA_LIMIT_4K | DA_C | priv << 5);

			init_desc(&p->ldts[INDEX_LDT_RW],
				  0, /* bytes before the entry point
				      * are useless (wasted) for the
				      * INIT process, doesn't matter
				      */
				  (k_base + k_limit) >> LIMIT_4K_SHIFT,
				  DA_32 | DA_LIMIT_4K | DA_DRW | priv << 5);
		}

		p->regs.cs = INDEX_LDT_C << 3 |	SA_TIL | rpl;
		p->regs.ds =
			p->regs.es =
			p->regs.fs =
			p->regs.ss = INDEX_LDT_RW << 3 | SA_TIL | rpl;
		p->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;
		p->regs.eip	= (u32)t->initial_eip;
		p->regs.esp	= (u32)stk;
		p->regs.eflags	= eflags;
		// paging
		// p->regs.cr3 = 0x100000;
		int m;
		for (m = 0; m < NR_VALID_PAGE; m++) {
			p->valid_page_id[m] = m * 2;
		}
		p->replace = 0;

		p->ticks = p->priority = prio;

		p->p_flags = 0;
		p->p_msg = 0;
		p->p_recvfrom = NO_TASK;
		p->p_sendto = NO_TASK;
		p->has_int_msg = 0;
		p->q_sending = 0;
		p->next_sending = 0;

		for (j = 0; j < NR_FILES; j++)
			p->filp[j] = 0;

		stk -= t->stacksize;
	}

	k_reenter = 0;
	ticks = 0;

	p_proc_ready = proc_table;

	init_clock();
        init_keyboard();

	restart();

	while(1){}
}


/*****************************************************************************
 *                                get_ticks
 *****************************************************************************/
PUBLIC int get_ticks()
{
	MESSAGE msg;
	reset_msg(&msg);
	msg.type = GET_TICKS;
	send_recv(BOTH, TASK_SYS, &msg);
	return msg.RETVAL;
}


/**
 * @struct posix_tar_header
 * Borrowed from GNU `tar'
 */
struct posix_tar_header
{				/* byte offset */
	char name[100];		/*   0 */
	char mode[8];		/* 100 */
	char uid[8];		/* 108 */
	char gid[8];		/* 116 */
	char size[12];		/* 124 */
	char mtime[12];		/* 136 */
	char chksum[8];		/* 148 */
	char typeflag;		/* 156 */
	char linkname[100];	/* 157 */
	char magic[6];		/* 257 */
	char version[2];	/* 263 */
	char uname[32];		/* 265 */
	char gname[32];		/* 297 */
	char devmajor[8];	/* 329 */
	char devminor[8];	/* 337 */
	char prefix[155];	/* 345 */
	/* 500 */
};

/*****************************************************************************
 *                                untar
 *****************************************************************************/
/**
 * Extract the tar file and store them.
 * 
 * @param filename The tar file.
 *****************************************************************************/
void untar(const char * filename)
{
	printf("[extract `%s'\n", filename);
	int fd = open(filename, O_RDWR);
	assert(fd != -1);

	char buf[SECTOR_SIZE * 16];
	int chunk = sizeof(buf);

	int cnt = 0;

	while (1) {
		read(fd, buf, SECTOR_SIZE);
		if (buf[0] == 0)
			break;

		struct posix_tar_header * phdr = (struct posix_tar_header *)buf;

		/* calculate the file size */
		char * p = phdr->size;
		int f_len = 0;
		while (*p)
			f_len = (f_len * 8) + (*p++ - '0'); /* octal */

		int bytes_left = f_len;
		int fdout = open(phdr->name, O_CREAT | O_RDWR);
		if (fdout == -1) {
			printf("    failed to extract file: %s\n", phdr->name);
			printf(" aborted]");
			return;
		}
		printf("    %s (%d bytes)\n", phdr->name, f_len);
#ifdef CMD_CHECK
		char temp[MAX_FILENAME_LEN] = {0};
		strcpy(temp, phdr->name);//文件名存起来了
#endif
		while (bytes_left) {
			// printf("%d\n", bytes_left);
			int iobytes = min(chunk, bytes_left);
			read(fd, buf,
			     ((iobytes - 1) / SECTOR_SIZE + 1) * SECTOR_SIZE);
			write(fdout, buf, iobytes);
			bytes_left -= iobytes;
		}
		close(fdout);
#ifdef CMD_CHECK
		if (cnt!=0){//第一个 kernel.bin 不校验
			writeChkFile(temp, CalCheckVal(temp));
			//CalCheckVal(phdr->name);
		}
		cnt++;
#endif
	}

	close(fd);

	printf(" done]\n");
}

/*****************************************************************************
 *                                shabby_shell
 *****************************************************************************/
/**
 * A very very simple shell.
 * 
 * @param tty_name  TTY file name.
 *****************************************************************************/
void shabby_shell(const char * tty_name)
{
	int fd_stdin  = open(tty_name, O_RDWR);
	assert(fd_stdin  == 0);
	int fd_stdout = open(tty_name, O_RDWR);
	assert(fd_stdout == 1);

	char rdbuf[128];

	while (1) {
		write(1, "$ ", 2);
		int r = read(0, rdbuf, 70);
		rdbuf[r] = 0;

		int argc = 0;
		char * argv[PROC_ORIGIN_STACK];
		char * p = rdbuf;
		char * s;
		int word = 0;
		char ch;
		do {
			ch = *p;
			if (*p != ' ' && *p != 0 && !word) {
				s = p;
				word = 1;
			}
			if ((*p == ' ' || *p == 0) && word) {
				word = 0;
				argv[argc++] = s;
				*p = 0;
			}
			p++;
		} while(ch);
		argv[argc] = 0;

		int concurrency_flag = 0;
		if (argv[argc - 1][0] == '&') {
			argv[--argc] = 0;
			concurrency_flag = 1;
		}

		int fd = open(argv[0], O_RDWR);
		if (fd == -1) {
			if (rdbuf[0]) {
				write(1, "{", 1);
				write(1, rdbuf, r);
				write(1, "}\n", 2);
			}
		}
		else {
			close(fd);
			int pid = fork();
			if (pid != 0) { /* parent */
				// printf("[parent is running, child pid:%d]\n", pid);
				if (!concurrency_flag) {
					int s;
					wait(&s, pid);
				}
			}
			else {	/* child */
				execv(argv[0], argv);
			}
		}
	}

	close(1);
	close(0);
}

/*****************************************************************************
 *                                Init
 *****************************************************************************/
/**
 * The hen.
 * 
 *****************************************************************************/
void Init()
{
	int fd_stdin  = open("/dev_tty0", O_RDWR);
	assert(fd_stdin  == 0);
	int fd_stdout = open("/dev_tty0", O_RDWR);
	assert(fd_stdout == 1);

	printf("Init() is running ...\n");

	/* extract `cmd.tar' */
	untar("/cmd.tar");
			

	// // char * tty_list[] = {"/dev_tty0"};
	char * tty_list[] = {"/dev_tty1", "/dev_tty2"};

	int i;
	for (i = 0; i < sizeof(tty_list) / sizeof(tty_list[0]); i++) {
		int pid = fork();
		if (pid != 0) { /* parent process */
			// printf("[parent is running, child pid:%d]\n", pid);
		}
		else {	/* child process */
			// printf("[child is running, pid:%d]\n", getpid());
			close(fd_stdin);
			close(fd_stdout);
			
			shabby_shell(tty_list[i]);
			assert(0);
		}
	}
	
	printl("orange's os initialization completed, ticks: %d\n", get_ticks());

	// MESSAGE msg;

	// msg.type	= PRINT_FILE;
	// send_recv(BOTH, TASK_SYS, &msg);

	// proc test
	int pid = fork();
	if (pid == 0) {
		proc_eval();
		exit(0);
	}


	while (1) {
		int s;
		int child = wait(&s, -1);
		printf("child (%d) exited with status: %d.\n", child, s);
	}

	assert(0);
}


PUBLIC void proc_eval() {
	printl("\nproc test start\n");
	int i;
	int pids[NR_PROC_TEST];
	MESSAGE msg;

	int delay1[NR_PROC_TEST] = {12, 3, 19, 7, 15};
	int delay2[NR_PROC_TEST] = {9, 2, 17, 16, 5};
	int delay3[NR_PROC_TEST] = {2, 4, 21, 10, 6};
	int delay_sum[NR_PROC_TEST] = {23, 9, 57, 33, 26};

	for (i = 0; i < NR_PROC_TEST; i ++) {
		pids[i] = fork();
		if (pids[i] != 0) {	// parent proc
			// sec_delay(10);
		}
		else {			// chlid proc
			inform_start();
			printl("[child is running, pid:%d]\n", getpid());

			test_delay(delay1[i]);

			msg.CNT = delay2[i];
			send_recv(BOTH, TESTB, &msg);

			test_delay(delay3[i]);

			inform_end();

			exit(0);
		}
	}

	while (!is_finish(pids[0])) {
		test_delay(20);
	}

	dump_proc_display(pids[0]);

	for (i = 0; i < NR_PROC_TEST; i ++) {
		int s;
		wait(&s, pids[i]);
	}
}

/*======================================================================*
                               TestA
 *======================================================================*/
void TestA()
{
	MESSAGE msg;
	while(1) {
		send_recv(RECEIVE, ANY, &msg);

		int src = msg.source;
		test_delay(msg.CNT);
		send_recv(SEND, src, &msg);
	}

	for(;;);
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestB()
{
	MESSAGE msg;
	while(1) {
		send_recv(RECEIVE, ANY, &msg);

		int src = msg.source;
		test_delay(msg.CNT);
		send_recv(SEND, src, &msg);
	}

	for(;;);
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestC()
{
	for(;;);
}

/* delay for about num ticks */
PUBLIC void test_delay(int num) {
	int i, j;
	for (i = 0; i < num; i++) {
		for (j = 0; j < 20000; j++) {
			// empty
		}
	}
}

/*****************************************************************************
 *                                panic
 *****************************************************************************/
PUBLIC void panic(const char *fmt, ...)
{
	int i;
	char buf[256];

	/* 4 is the size of fmt in the stack */
	va_list arg = (va_list)((char*)&fmt + 4);

	i = vsprintf(buf, fmt, arg);

	printl("%c !!panic!! %s", MAG_CH_PANIC, buf);

	/* should never arrive here */
	__asm__ __volatile__("ud2");
}

