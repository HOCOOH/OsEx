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

PUBLIC void page_fault_handler(int irq) {
    // bb;
    u32 addr;
    //将引发缺页异常的线性地址保存在address变量里面  
    asm("movl %%cr2,%0":"=r" (addr));   
    // printl("0x%x\n", addr);
    // bb;

    // find the proc id
    int i, j;
    for (i = 0; i < NR_PROCS - NR_TASKS - NR_NATIVE_PROCS; i++) {
        if (PROCS_BASE + i * PROC_IMAGE_SIZE_DEFAULT <= addr && \
            addr < PROCS_BASE + (i + 1) * PROC_IMAGE_SIZE_DEFAULT) {
                break;
        }
    }
    int pid = i + NR_TASKS + NR_NATIVE_PROCS;
    struct proc* p = proc_table + pid;
    // assert(p == p_proc_ready);
    // printl("pid = %d\n", pid);

    // cal page index
    u32 proc_page_addr = PROCS_BASE + i * PROC_IMAGE_SIZE_DEFAULT;
    u32 curr_page_addr = addr & 0xfffff000;
    int page_id = (curr_page_addr - proc_page_addr) >> LIMIT_4K_SHIFT;
    // bb;
    u32 proc_pte_addr = PROCS_PTE_BASE + i * PROC_PTE_SIZE;
    u32 curr_pte_addr = proc_pte_addr + 4 * page_id;
    // if (addr == 0xc03000) {
    //     printl("0x%x 0x%x %d 0x%x 0x%x\n", proc_page_addr, curr_page_addr, page_id, proc_pte_addr, curr_pte_addr);
    // }
    // printl("0x%x 0x%x %d\n", proc_page_addr, curr_page_addr, page_id);
    // printl("0x%x\n", curr_pte_addr);
    // bb;
    u32* pm;
    // u32* pm = (u32*)curr_pte_addr;
    // *pm |= 1;


    // 调页
    int die_page_id = p->valid_page_id[p->replace];
    u32 die_pte_addr = proc_pte_addr + 4 * die_page_id;
    // kill page
    pm = (u32*)die_pte_addr;
    *pm ^= PG_P;

    // add page
    pm = (u32*)curr_pte_addr;
    *pm |= PG_P;
    p->valid_page_id[p->replace] = page_id;
    p->replace = (p->replace + 1) % NR_VALID_PAGE;



    // bb;
}