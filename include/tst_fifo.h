// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2019 Joerg Vehlow <joerg.vehlow@aox-tech.de>
 */

void tst_fifo_init(void);

void tst_fifo_create(const char *name);
void tst_fifo_destroy(const char *name, int ignore_error);

int tst_fifo_send(const char *name, const char *data, int timeout_ms);
int tst_fifo_receive(const char *name, char *data, int maxlen, int timeout_ms);
