#!/bin/bash
set -xe

autoreconf -i
$SHELL configure
make -j`nproc` $@
