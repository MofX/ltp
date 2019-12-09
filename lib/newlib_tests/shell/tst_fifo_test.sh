#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2019 Joerg Vehlow <joerg.vehlow@aox-tech.de>

TST_TESTFUNC=do_test
TST_NEEDS_FIFO=1
TST_NEEDS_TMPDIR=1
TST_CNT=2
TST_TIMEOUT=10

. tst_test.sh

S2P="fifo_shell_to_proc"
P2S="fifo_proc_to_shell"

do_test1()
{
    tst_fifo_create $S2P
    tst_fifo_create $P2S

    tst_fifo_proc &
    pid=$!

    tst_fifo_send $S2P "init message" 0
    tst_res TPASS "init message send"

    tst_fifo_send $S2P "second init message" 0
    tst_res TPASS "second init message send"

    data=$(tst_fifo_receive $P2S 0)
    if [ "$data" = "answer 1" ]; then
        tst_res TPASS "Received first expected answer"
    else
        tst_res TFAIL "First expected answer mismatch ('$data')"
    fi

    data=$(tst_fifo_receive $P2S 0)
    if [ "$data" = "answer 2" ]; then
        tst_res TPASS "Received second expected answer"
    else
        tst_res TFAIL "Second expected answer mismatch ('$data')"
    fi

    data=$(tst_fifo_receive $P2S 0)
    if [ "$data" = "answer 3" ]; then
        tst_res TPASS "Received third expected answer"
    else
        tst_res TFAIL "Third expected answer mismatch ('$data')"
    fi

    tst_res TINFO "Waiting for tst_fifo_proc to terminate"
    wait $pid

    tst_fifo_destroy $S2P
    tst_fifo_destroy $P2S

    tst_res TPASS "All tests passed"
}

do_test2()
{
    tst_res TINFO "Testing timeouts of send and receive function"

    tst_fifo_create $S2P
    tst_fifo_send $S2P Test 100
    tst_fifo_receive $S2P 100
    tst_fifo_destroy $S2P

    tst_res TPASS "Timeout tests successfull"
}

tst_run
