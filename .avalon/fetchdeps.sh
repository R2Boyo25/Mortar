#!/bin/bash
set -xe

mkdir tmp

if [ ! -d "include/clipp" ]; then
    git clone https://github.com/muellan/clipp tmp/clipp --depth=1
    mv tmp/clipp/include include/clipp
fi

if [ ! -d "include/doctest" ]; then
    git clone https://github.com/doctest/doctest tmp/doctest --depth=1
    rm -rf tmp/doctest/doctest/extensions tmp/doctest/doctest/BUILD.bazel
    mv tmp/doctest/doctest include
fi

rm -rf tmp
