// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2021 Joerg Vehlow <joerg.vehlow@aox-tech.de>
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define TST_NO_DEFAULT_MAIN
#include "tst_test.h"
#include "tst_fifo.h"

static void print_usage(void)
{
	printf("Usage: tst_fifo create NAME\n");
	printf("   or: tst_fifo destroy NAME\n");
	printf("   or: tst_fifo recreate NAME\n");
	printf("   or: tst_fifo send NAME MSG TIMEOUT\n");
	printf("   or: tst_fifo receive NAME TIMEOUT\n");
	printf("       NAME - name of the fifo\n");
	printf("       MSG - message to send\n");
	printf("       MAXSIZE - maximum number of bytes to receive\n");
	printf("       TIMEOUT - timeout in ms\n");
}

static int parse_int(const char *name, const char *arg)
{
	int val;
	char *e ;
	val = strtol(arg, &e, 10);
	if (e != arg + strlen(arg) || val < 0) {
		fprintf(stderr, "Invalid value for %s '%s'\n", name, arg);
		return -1;
	}
	return val;
}

int main(int argc, char *argv[])
{
	if (argc < 3)
		goto usage;

	tst_fifo_init();
	if (strcmp(argv[1], "create") == 0) {
		if (argc != 3)
			goto usage;
		tst_fifo_create(argv[2]);
	} else if (strcmp(argv[1], "destroy") == 0) {
		if (argc != 3)
			goto usage;
		tst_fifo_destroy(argv[2], 1);
	} else if (strcmp(argv[1], "recreate") == 0) {
		if (argc != 3)
			goto usage;
		tst_fifo_destroy(argv[2], 1);
		tst_fifo_create(argv[2]);
	} else if (strcmp(argv[1], "send") == 0) {
		int timeout = 0;
		
		if (argc != 5)
			goto usage;
		timeout = parse_int("timeout", argv[4]);
		if (timeout == -1)
			goto usage;

		return tst_fifo_send(argv[2], argv[3], timeout);
	} else if (strcmp(argv[1], "receive") == 0) {
		int timeout = 0;
		int ret;
		char buffer[4096];
		if (argc != 4)
			goto usage;
		timeout = parse_int("timeout", argv[3]);
		if (timeout == -1)
			goto usage;

		ret = tst_fifo_receive(argv[2], buffer, sizeof(buffer), timeout);
		if (ret == -1)
			return 1;
		printf("%s", buffer);
	} else {
		fprintf(stderr, "ERROR: Invalid COMMAND '%s'\n", argv[1]);
		goto usage;
	}

	return 0;
usage:
	print_usage();
	return 1;
}
