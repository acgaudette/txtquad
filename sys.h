#ifndef SYS_H
#define SYS_H

#include <stdlib.h>
#include <stdio.h>

__attribute__((noreturn))
static void panic()
{
	printf("Exit failure\n");
	fflush(stdout);
	abort();
}

__attribute__((noreturn))
static void panic_msg(const char *msg)
{
	fflush(stdout);
	fprintf(stderr, "panic(): %s\n", msg);
	panic();
}

#endif
