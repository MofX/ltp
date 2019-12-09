// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2021 Joerg Vehlow <joerg.vehlow@aox-tech.de>
 */

#include <stdlib.h>

#include "tst_test.h"
#include "tst_fifo.h"
#include "tst_timer.h"

static const char MSG_P2C[] = "AbcD";
static const char MSG_C2P[] =  "Hello World";

static const int TEST_TIMEOUT = 10 /* ms */;
/* tst_fifo uses busy polling for writing with a sleep
 * interval of 10 ms.
 * Use 15 here, to add some slack.
 */
static const int TEST_TIMEOUT_EPSILON = 15 /* ms */;

static void recreate_fifo(const char *name)
{
	tst_fifo_destroy(name, 1);
	tst_fifo_create(name);
}

static void do_setup(void)
{
	tst_fifo_init();
}

static void test_simple(void)
{
	tst_res(TINFO, "Simple test");

	pid_t pid = SAFE_FORK();
	if (pid == 0) {
		char data[sizeof(MSG_P2C)];
		if (tst_fifo_receive("p2c", data, sizeof(data), 0) != strlen(MSG_P2C))
			tst_res(TFAIL, "Child did not receive expected length");
		if (strcmp(data, MSG_P2C) != 0)
			tst_res(TFAIL, 
			        "Child did not receive expected value ('%s' != '%s')",
					MSG_P2C, data);
		else
			tst_res(TPASS, "Child received expected value");
		
		tst_fifo_send("c2p", MSG_C2P, 0);
	} else {
		tst_fifo_send("p2c", MSG_P2C, 0);

		char data[sizeof(MSG_C2P)];
		if (tst_fifo_receive("c2p", data, sizeof(data), 0) != strlen(MSG_C2P))
			tst_res(TFAIL, "Parent did not receive expected length");
		if (strcmp(data, MSG_C2P) != 0)
			tst_res(TFAIL, 
			        "Parent did not receive expected value ('%s' != '%s')",
					MSG_C2P, data);
		else
			tst_res(TPASS, "Parent received expected value");
	}
}

static void test_recv_timeout(void)
{
	char data[sizeof(MSG_P2C)];
	int ret;
	tst_res(TINFO, "Receive timeout test");

	tst_timer_start(CLOCK_MONOTONIC);
	ret = tst_fifo_receive("p2c", data, sizeof(data), TEST_TIMEOUT);
	if (ret != -1 || errno != ETIMEDOUT)
		tst_res(TFAIL | TERRNO,
		        "Unexpected result from tst_fifo_receive: %d", ret);
	tst_timer_stop();
	
	if (labs(tst_timer_elapsed_ms() - TEST_TIMEOUT) < TEST_TIMEOUT_EPSILON) {
		ret = TPASS;
	} else {
		ret = TFAIL;
	}

	tst_res(ret, "tst_fifo_receive with %d ms timeout returned after %lld ms",
	        TEST_TIMEOUT,
	        tst_timer_elapsed_ms());
}

static void test_send_timeout(void)
{
	int ret;
	tst_res(TINFO, "Send timeout test");

	tst_timer_start(CLOCK_MONOTONIC);
	ret = tst_fifo_send("p2c", MSG_P2C, TEST_TIMEOUT);
	if (ret != -1 || errno != ETIMEDOUT)
		tst_res(TFAIL | TERRNO,
				"Unexpected result from tst_fifo_send: %d", ret);
	tst_timer_stop();
	
	if (labs(tst_timer_elapsed_ms() - TEST_TIMEOUT) < TEST_TIMEOUT_EPSILON) {
		ret = TPASS;
	} else {
		ret = TFAIL;
	}
	
	tst_res(ret, "tst_fifo_send with %d ms timeout returned after %lld ms",
	        TEST_TIMEOUT,
	        tst_timer_elapsed_ms());
}

static void do_test(unsigned int i)
{
	recreate_fifo("p2c");
	recreate_fifo("c2p");

	switch (i) {
	case 0:
		test_simple();
	break;
	case 1:
		test_recv_timeout();
	break;
	case 2:
		test_send_timeout();
	break;
	}
}

static struct tst_test test = {
	.needs_tmpdir = 1,
	.forks_child = 1,
	.tcnt = 3,
	.setup = do_setup,
	.test = do_test
};
