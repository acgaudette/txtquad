#ifndef SYS_H
#define SYS_H

#include <stdlib.h>
#include <stdio.h>

static void panic()
{
	fflush(stdout);
	fprintf(stderr, "Exit failure\n");
	exit(1);
}

static void panic_msg(const char *msg)
{
	fflush(stdout);
	fprintf(stderr, "panic(): %s\n", msg);
	panic();
}

#endif
