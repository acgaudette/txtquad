#ifndef SYS_H
#define SYS_H

#include <stdlib.h>
#include <stdio.h>

static void panic()
{
	printf("Exit failure\n");
	fflush(stdout);
	abort();
}

static void panic_msg(const char *msg)
{
	fflush(stdout);
	fprintf(stderr, "panic(): %s\n", msg);
	panic();
}

#endif
