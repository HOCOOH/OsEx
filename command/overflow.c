#include "stdio.h"
#include "type.h"
#include "string.h"
char *p_dst;
int cur;
void target() {
    printf("Overflow success!!!\n");
    asm("jmp 0x10e1");
}
void exec_overflow(char* p_src) {
    
    char buf1[5];
    // asm("xchg %bx, %bx");
    p_dst = buf1;

    for (cur = 0; cur < 13; cur++) 
    {
        *p_dst = *p_src;
        p_dst++;
        p_src++;
    }
    // asm("xchg %bx, %bx");
    
}

int main() {
    char buf3[] = "A0A1A2A3A";
    char buf2[14];
    int i = 0;
    for (i = 0; i < 9; i++ ) {
        buf2[i] = buf3[i];
    }

    buf2[9] = 0x00;
    buf2[10] = 0x10;
    buf2[11] = 0x00;
    buf2[12] = 0x00;
    buf2[13] = 0x00;
    
    // asm("xchg %bx, %bx");
    exec_overflow(buf2);
    // asm("xchg %bx, %bx");
    return 0;
}