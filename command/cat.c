#include "stdio.h"
#include "type.h"


int main(int argc, char * argv[])
{
    // 读取文件名称
	const char *filename = argv[1];
    int fd;
	fd = open(filename, O_RDWR);
	if (fd == -1)
		printf("open file fail!\n");
	
    // 获取文件大小
	struct stat s;
	stat(filename, &s);
	int filesize = s.st_size;
	int n;
	char bufr[filesize + 1];

	
	n = read(fd, bufr, filesize);
	assert(n == filesize);
	bufr[n] = 0;
	printf("file content:\n%s\n", bufr);

	close(fd);
	printf("\n");
	return 0;
}