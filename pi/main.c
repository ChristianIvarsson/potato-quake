#include <sys/resource.h>
#include <stdio.h>
#include <string.h>
#include "../core/quakedef.h"

extern int quake_main (int argc, char **argv, size_t memsize);

// Deepest thus far has been 88496 (86.4k)
#define STACK_SIZE         (512 * 1024)
#define HEAP_SIZE          (4 * 1024 * 1024)

static struct rlimit rl;

int main(int argc, char **argv)
{
    if (getrlimit(RLIMIT_STACK, &rl) == 0) {
        rl.rlim_cur = STACK_SIZE;
        if (setrlimit(RLIMIT_STACK, &rl) != 0) {
            printf("Could not resize stack\n");
            return -1;
        }
    }
    else {
    	printf("Could not get current stack stats\n");
    	return -1;
    }

	quake_main(argc, argv, HEAP_SIZE);
	return 0;
}