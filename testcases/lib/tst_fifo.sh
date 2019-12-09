#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2021 Joerg Vehlow <joerg.vehlow@aox-tech.de>

[ -n "$TST_FIFO_LOADED" ] && return 0
TST_FIFO_LOADED=1
export LTP_FIFO_PATH="$TST_TMPDIR"

tst_fifo_create()
{
    ROD tst_fifo create "$@"
}

tst_fifo_destroy()
{
    ROD tst_fifo destroy "$@"
}

tst_fifo_recreate()
{
    ROD tst_fifo recreate "$@"
}

tst_fifo_send()
{
    tst_fifo send "$@"
}

tst_fifo_receive()
{
    tst_fifo receive "$@"
}
