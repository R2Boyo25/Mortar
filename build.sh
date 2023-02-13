#!/bin/bash
set -xe

aclocal
autoconf
automake --add-missing
$SHELL configure
make -j`nproc` $@
