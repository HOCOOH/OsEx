#include "stdio.h"

int main(int argc, char * argv[])
{
    int i, j, k;
    for (i = 0; i < 10000; i++) {
        for (j = 0; j < 2000; j++) {

        }
    }

	for (i = 1; i < argc; i++)
		printf("%s%s", i == 1 ? "" : " ", argv[i]);
	printf("\n");

	return 0;
}