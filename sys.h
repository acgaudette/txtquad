#ifndef SYS_H
#define SYS_H

#include <stdlib.h>
#include <stdio.h>

static void panic()
{
	printf("Exit failure\n");
	exit(1);
}

static void panic_msg(const char *msg)
{
	fprintf(stderr, "Error: %s\n", msg);
	panic();
}

#endif
