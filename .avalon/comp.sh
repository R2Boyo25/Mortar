#!/bin/bash

CXXFLAGS="-Ofast -omortar -std=c++17 -lpthread -Iinclude -DCOMMITHASH=\"`git rev-parse HEAD`\""
SRCFILES="src/*.cpp include/doctest/parts/doctest.cpp"
TESTS=false

if [ -z "$SHELL" ]; then
    SHELL=/bin/bash
fi

set -xe
$SHELL .avalon/fetchdeps.sh

while test $# != 0; do
    case "$1" in
    --tests) TESTS=true ;;
    esac
    shift
done

if [ $TESTS ]; then
    CXXFLAGS="$CXXFLAGS -DDOCTEST_CONFIG_INCLUDE_TYPE_TRAITS -DDOCTEST_CONFIG_NO_UNPREFIXED_OPTIONS"
else
    CXXFLAGS="$CXXFLAGS -DDOCTEST_CONFIG_DISABLE"
fi

g++ $CXXFLAGS $SRCFILES
