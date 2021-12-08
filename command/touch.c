#include "stdio.h"
#include "type.h"
#include "string.h"

int main(int argc, char * argv[])
{
    
    char *filename = argv[1];
	int fd;
    fd = open(filename, O_CREAT | O_RDWR);
	if (fd == -1)
		printf("open file fail!\n");
    

    printf("Please write contents in new file:\n");
    char bufr[20];
    int r = read(0, bufr, 10);
	bufr[r] = 0;

	int n;
	/* write */
	n = write(fd, bufr, strlen(bufr));
	if (n != strlen(bufr))
		printf("write file error!\n");
    return 0;
}