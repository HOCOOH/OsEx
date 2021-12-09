/*************************************************************************//**
 *****************************************************************************
 * @file   mm/exec.c
 * @brief  
 * @author Forrest Y. Yu
 * @date   Tue May  6 14:14:02 2008
 *****************************************************************************
 *****************************************************************************/

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
#include "keyboard.h"
#include "proto.h"
#include "elf.h"


/*****************************************************************************
 *                                do_exec
 *****************************************************************************/
/**
 * Perform the exec() system call.
 * 
 * @return  Zero if successful, otherwise -1.
 *****************************************************************************/
PUBLIC int do_exec()
{
	/* get parameters from the message */
	int name_len = mm_msg.NAME_LEN;	/* length of filename */
	int src = mm_msg.source;	/* caller proc nr. */
	assert(name_len < MAX_PATH);

	char pathname[MAX_PATH];
	phys_copy((void*)va2la(TASK_MM, pathname),
		  (void*)va2la(src, mm_msg.PATHNAME),
		  name_len);
	pathname[name_len] = 0;	/* terminate the string */
	//对elf文件的校验放在代码块里
	{
		//在 do_exec()最前面校验 可执行的elf文件
		//pathname 是文件名
		char temp[MAX_PATH] = {0};
		strcpy(temp, pathname);
		int Flag = CalCheckVal(temp);//计算当前的校验值
		{//在 temp 后面加入 ".chk"
			char * p1 = temp;
			for (; *p1; p1++) {}
			const char * p2 = ".chk";
			for (; *p2; p1++,p2++) {
				*p1 = *p2;
			}
			*p1 = 0;
		}
		int fd_chk = open(temp, O_RDWR);
		// 获取文件大小
		struct stat s;
		stat(temp, &s);
		int filesize = s.st_size;
		int N;
		char bufr[filesize + 1];
		N = read(fd_chk, bufr, filesize);
		close(fd_chk);
		assert(N == filesize);
		bufr[N] = 0;
		if (Flag == 0 && bufr[0] == '0')
		{
			printl("This elf has passed the check! \n");
		}else if (Flag == 1 && bufr[0] == '1')
		{
			printl("This elf has passed the check! \n");
		}else
		{
			printl("Oh The elf file may be changed!\n");
			return -1;
		}
	
	}
	
	/* get the file size */
	struct stat s;
	int ret = stat(pathname, &s);
	if (ret != 0) {
		printl("{MM} MM::do_exec()::stat() returns error. %s", pathname);
		return -1;
	}

	/* read the file */
	int fd = open(pathname, O_RDWR);
	if (fd == -1)
		return -1;
	assert(s.st_size < MMBUF_SIZE);
	read(fd, mmbuf, s.st_size);
	close(fd);

	/* overwrite the current proc image with the new one */
	/* note: multitask shell need  */
	Elf32_Ehdr* elf_hdr = (Elf32_Ehdr*)(mmbuf);
	int i;
	for (i = 0; i < elf_hdr->e_phnum; i++) {
		Elf32_Phdr* prog_hdr = (Elf32_Phdr*)(mmbuf + elf_hdr->e_phoff +
			 			(i * elf_hdr->e_phentsize));
		if (prog_hdr->p_type == PT_LOAD) {
			assert(prog_hdr->p_vaddr + prog_hdr->p_memsz <
				PROC_IMAGE_SIZE_DEFAULT);
			phys_copy((void*)va2la(src, (void*)prog_hdr->p_vaddr),
				  (void*)va2la(TASK_MM,
						 mmbuf + prog_hdr->p_offset),
				  prog_hdr->p_filesz);
		}
	}

	/* setup the arg stack */
	int orig_stack_len = mm_msg.BUF_LEN;
	char stackcopy[PROC_ORIGIN_STACK];
	phys_copy((void*)va2la(TASK_MM, stackcopy),
		  (void*)va2la(src, mm_msg.BUF),
		  orig_stack_len);
	/* note:  */
	u8 * orig_stack = (u8*)(PROC_IMAGE_SIZE_DEFAULT - PROC_ORIGIN_STACK);

	int delta = (int)orig_stack - (int)mm_msg.BUF;

	int argc = 0;
	if (orig_stack_len) {	/* has args */
		char **q = (char**)stackcopy;
		for (; *q != 0; q++,argc++)
			*q += delta;
	}

	phys_copy((void*)va2la(src, orig_stack),
		  (void*)va2la(TASK_MM, stackcopy),
		  orig_stack_len);

	proc_table[src].regs.ecx = argc; /* argc */
	proc_table[src].regs.eax = (u32)orig_stack; /* argv */

	/* setup eip & esp */
	/* note:  */
	proc_table[src].regs.eip = elf_hdr->e_entry; /* @see _start.asm */
	proc_table[src].regs.esp = PROC_IMAGE_SIZE_DEFAULT - PROC_ORIGIN_STACK;

	strcpy(proc_table[src].name, pathname);

	return 0;
}

//对一个 u8 奇偶校验
PUBLIC int parityCheck (u8 v){
	// 待检测的u8 v
    int parity = 0;  //初始判断标记
    //通过while循环，每执行一次，v中1的数目就会减少1，
    //如果v中1的数目为奇数，则parity = 1，否则parity = 0。
    //偶校验
    while (v)
    {
        parity = !parity;
        v = v & (v - 1);
    }
    return parity;
}

//写指定文件的校验值flag至chk文件
PUBLIC void writeChkFile (char* file_name, int flag){
	char chkName[MAX_FILENAME_LEN + 5] = {0};
	strcpy(chkName, file_name);
	//const char* check = ".chk";		
	{//在 file_name 后面加入 ".chk"
		char * p1 = chkName;
		for (; *p1; p1++) {}
		const char * p2 = ".chk";
		for (; *p2; p1++,p2++) {
			*p1 = *p2;
		}
		*p1 = 0;
	}
	int fd = open(chkName, O_CREAT);//创建文件
	if (fd == -1)	//文件已经存在,没有成功打开
	{
		fd = open(chkName, O_RDWR);//打开文件
		if (fd == -1){
			printf("open file fail!\n");
		}
	}else{//文件不存在，创建成功，但是上面仅仅是创建，关闭，以读写打开
	
		//printf("creat file success!\n");
		close(fd);
		fd = open(chkName, O_RDWR);//打开文件
	}
	char buf0[] = "0";
	char buf1[] = "1";
	printf("Parity check for %s : %d \n", file_name, flag);
	int n;
	if (flag == 0){					//写校验文件
		n = write(fd, buf0, strlen(buf0));
		if (n != strlen(buf0))
			printf("flag = 0 write file error! n = %d\n",n);
	}else{
		n = write(fd, buf1, strlen(buf1));
		if (n != strlen(buf1))
			printf("flag = 1 write file error! n = %d\n",n);
	}
	close(fd);


}

//对指定文件奇偶校验，返回校验值
PUBLIC int CalCheckVal(char* Filename) {

    int fd;
	char filename[MAX_FILENAME_LEN + 5] = {0} ;
	strcpy(filename, Filename);
	fd = open(filename, O_RDWR);//打开文件,文件需要存在
	if (fd == -1){
		printf("open file fail!\n");
	}
	
	struct stat s;
	stat(filename, &s);
	int filesize = s.st_size;
	int n;
	u8 bufr[filesize + 1];
	n = read(fd, bufr, filesize);//读取文件内容（u8）到bufr
	assert(n == filesize);
	close(fd);
	bufr[n] = 0;
	int flag = 0;//flag是奇偶校验的结果(int)
	u8* work = bufr;
	int i = 0;
	int cnt = 0;
	for(i = 0; i < n; i++)
	{
	    flag ^= parityCheck(*work);//奇偶校验
		work++;
		cnt++;
	}
	//printf("filesize = %d  cycles: %d\n", n, cnt);
	return flag;
	
}