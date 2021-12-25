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
    u32 addr;
    //将引发缺页异常的线性地址保存在addr变量里面  
    asm("movl %%cr2,%0":"=r" (addr));   

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

    // cal page index
    u32 proc_page_base = PROCS_BASE + i * PROC_IMAGE_SIZE_DEFAULT;
    u32 proc_pte_base = PROCS_PTE_BASE + i * PROC_PTE_SIZE;
    u32 curr_page_addr = addr & 0xfffff000;
    // cal pte addr
    int curr_page_id = (curr_page_addr - proc_page_base) >> LIMIT_4K_SHIFT;
    u32* curr_pte_addr = (u32*)proc_pte_base + curr_page_id;
    int die_page_id = p->valid_page_id[p->replace];
    u32* die_pte_addr = (u32*)proc_pte_base + die_page_id;
    // kill page
    *die_pte_addr ^= PG_P;
    // add page
    *curr_pte_addr ^= PG_P;
    p->valid_page_id[p->replace] = curr_page_id;
    p->replace = (p->replace + 1) % NR_VALID_PAGE;  // FIFO

    printl("%10s access 0x%x : Page %d -> Page %d\n", p->name, addr, die_page_id, curr_page_id);
}