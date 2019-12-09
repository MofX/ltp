// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2021 Joerg Vehlow <joerg.vehlow@aox-tech.de>
 */


#define TST_NO_DEFAULT_MAIN
#include "tst_test.h"
#include "tst_fifo.h"

#define S2P "fifo_shell_to_proc"
#define P2S "fifo_proc_to_shell"

int main(void)
{
    char data[128];

    tst_fifo_init();

    tst_fifo_receive(S2P, data, sizeof(data), 0);
    tst_res(strcmp(data, "init message") == 0 ? TPASS : TFAIL,
            "CHILD: Received expected init message (%s)", data);

    // Wait a bit for asynchronous access to pipe
    sleep(1);

    tst_fifo_receive(S2P, data, sizeof(data), 0);
    tst_res(strcmp(data, "second init message") == 0 ? TPASS : TFAIL,
            "Received expected second init message");

    tst_fifo_send(P2S, "answer 1", 0);
    sleep(1);
    tst_fifo_send(P2S, "answer 2", 0);
    sleep(1);
    tst_fifo_send(P2S, "answer 3", 0);

    tst_res(TPASS, "CHILD: All tests passed");

    return 0;
}
