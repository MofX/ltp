// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2021 Joerg Vehlow <joerg.vehlow@aox-tech.de>
 */
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <poll.h>
#include <fcntl.h>

#define TST_NO_DEFAULT_MAIN
#include "tst_test.h"
#include "tst_clocks.h"
#include "tst_timer.h"
#include "old_tmpdir.h"
#include "tst_fifo.h"

#ifndef PATH_MAX
#ifdef MAXPATHLEN
#define PATH_MAX MAXPATHLEN
#else
#define PATH_MAX 1024
#endif
#endif

static const char FIFO_ENV_VAR[] = "LTP_FIFO_PATH";
static const char TYPE_REQ[] = "req";
static const char TYPE_ACK[] = "ack";
static const int POLL_SLEEP_TIME = 1000 * 10 /* us */;

static char *FIFO_DIR = NULL;

static void tst_gettime_monotonic(const char *file, int line,
                                  struct timespec *ts)
{
	if (tst_clock_gettime(CLOCK_MONOTONIC, ts))
		tst_brk_(file, line, TBROK | TERRNO, "tst_clock_gettime() failed");
}

#define SAFE_GETTIME_MONOTONIC(ts) \
	tst_gettime_monotonic(__FILE__, __LINE__, ts)

static void tst_fifo_get_path(const char *name, const char *type,
                              int access_mode, char *path, int size)
{
	if (!FIFO_DIR)
		tst_brk(TBROK, "you must call tst_fifo_init first");
	snprintf(path, size, "%s/tst_fifo_%s.%s", FIFO_DIR, name, type);
	if (access_mode && access(path, access_mode))
		tst_brk(TBROK, "cannot access '%s'", path);
}

void tst_fifo_init(void)
{
	if (tst_tmpdir_created()) {
		FIFO_DIR = tst_get_tmpdir();
		setenv(FIFO_ENV_VAR, FIFO_DIR, 1);
	} else {
		FIFO_DIR = getenv(FIFO_ENV_VAR);
	}

	if (!FIFO_DIR)
		tst_brk(TBROK, "no tempdir and %s not set", FIFO_ENV_VAR);
}

void tst_fifo_create(const char *name)
{
	char path_req[PATH_MAX];
	char path_ack[PATH_MAX];

	tst_fifo_get_path(name, TYPE_REQ, 0, path_req, sizeof(path_req));
	tst_fifo_get_path(name, TYPE_ACK, 0, path_ack, sizeof(path_ack));

	if (mkfifo(path_req, S_IRWXU | S_IRWXG | S_IRWXO))
		tst_brk(TBROK, "mkfifo(%s) failed with %s",
		        path_req, tst_strerrno(errno));

	if (mkfifo(path_ack, S_IRWXU | S_IRWXG | S_IRWXO))
		tst_brk(TBROK, "mkfifo(%s) failed with %s",
		        path_ack, tst_strerrno(errno));
}

void tst_fifo_destroy(const char *name, int ignore_error)
{
	char path_req[PATH_MAX];
	char path_ack[PATH_MAX];
	int mode = R_OK | W_OK;

	if (ignore_error)
		mode = 0;

	tst_fifo_get_path(name, TYPE_REQ, mode, path_req, sizeof(path_req));
	tst_fifo_get_path(name, TYPE_ACK, mode, path_ack, sizeof(path_ack));

	if (remove(path_req)) {
		if (!ignore_error)
			tst_brk(TBROK, "unable to remove fifo '%s'", path_req);
	}
	if (remove(path_ack)) {
		if (!ignore_error)
			tst_brk(TBROK, "unable to remove fifo '%s'", path_ack);
	}
}

static int tst_fifo_send_msg(const char *path, const char *data,
                             int timeout_ms)
{
	int fd;
	struct timespec start, ts;

	SAFE_GETTIME_MONOTONIC(&start);
	/* manual polling here, because there is no api, to poll
	 * for writable fif, except maybe inotify.
	 * But inotify is not necessarily avaliable
	 */
	while (1) {
		fd = open(path, O_WRONLY | O_NONBLOCK);
		if (fd != -1) {
			break;
		} else if (errno != ENXIO && errno != EINTR) {
			tst_brk(TBROK | TERRNO, "Unexpected error returned from open");
		} else {
			SAFE_GETTIME_MONOTONIC(&ts);
			if (timeout_ms && tst_timespec_diff_ms(ts, start) > timeout_ms) {
				errno = ETIMEDOUT;
				return -1;
			}
			usleep(POLL_SLEEP_TIME);
		}
	}
	/* disable non-blocking */
	fcntl(fd, F_SETFL, O_WRONLY);
	SAFE_WRITE(1, fd, data, strlen(data));
	SAFE_CLOSE(fd);
	return 0;
}

static int tst_fifo_recv_msg(const char *path, char *data, int maxlen, int timeout_ms)
{
	int fd;
	int ret;
	int pos;
	int nbyte;
	struct pollfd fds = {};

	fd = SAFE_OPEN(path, O_RDONLY | O_NONBLOCK);

	fds.fd = fd;
	fds.events = POLLIN;
	if (timeout_ms == 0)
		timeout_ms = -1;
	do {
		ret = poll(&fds, 1, timeout_ms);
		if (ret == 0) {
			SAFE_CLOSE(fd);
			errno = ETIMEDOUT;
			return -1;
		} else if (ret == 1) {
			if (fds.revents & POLLIN) {
				/* fifo is now open */
				break;
			} else if (fds.revents & POLLHUP) {
				tst_brk(TBROK, "Peer unexpectedly closed the fifo");
			} else {
				tst_brk(TBROK, "Unexpectedly event in poll: %d", fds.revents);
			}
		}
	} while (ret == -1 && errno == EINTR);
	/* disable non-blocking */
	fcntl(fd, F_SETFL, O_RDONLY);

	pos = 0;
	while (1) {
		nbyte = SAFE_READ(0, fd, data + pos, maxlen - pos);
		if (nbyte == 0)
			break;
		pos += nbyte;
		if (pos == maxlen)
			tst_brk(TBROK, "buffer is not big enough");
	}

	SAFE_CLOSE(fd);

	data[pos] = 0;
	return pos;
}

int tst_fifo_send(const char *name, const char *data, int timeout_ms)
{
	int ret;
	char ack[3];
	char path_req[PATH_MAX];
	char path_ack[PATH_MAX];

	tst_fifo_get_path(name, TYPE_REQ, W_OK, path_req, sizeof(path_req));
	tst_fifo_get_path(name, TYPE_ACK, R_OK, path_ack, sizeof(path_ack));

	if ((ret = tst_fifo_send_msg(path_req, data, timeout_ms)))
		return ret;

	ret = tst_fifo_recv_msg(path_ack, ack, sizeof(ack), timeout_ms);
	if (ret != 2) {
		errno = EBADMSG;
		return -1;
	}
	return 0;
}

int tst_fifo_receive(const char *name, char *data, int maxlen, int timeout_ms)
{
	int ret;
	int nbytes;
	char path_req[PATH_MAX];
	char path_ack[PATH_MAX];

	tst_fifo_get_path(name, TYPE_REQ, R_OK, path_req, sizeof(path_req));
	tst_fifo_get_path(name, TYPE_ACK, W_OK, path_ack, sizeof(path_ack));

	nbytes = tst_fifo_recv_msg(path_req, data, maxlen, timeout_ms);
	if (nbytes == -1)
		return nbytes;

	if ((ret = tst_fifo_send_msg(path_ack, "OK", timeout_ms)))
		return ret;
	return nbytes;
}
