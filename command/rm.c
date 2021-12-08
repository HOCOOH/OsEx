#include "stdio.h"
#include "type.h"

int main(int argc, char * argv[])
{
    
    const char *filename = argv[1];
    int ret = unlink(filename);
    if (!ret)
        printf("file has been deleted.\n");
    else
        printf("delete file error!\n");
    printf("\n");
    return 0;
}