import os
import sys

if not os.path.exists("include/doctest"):
    os.system("mkdir tmp")
    os.system("git clone https://github.com/muellan/clipp tmp/clipp")
    os.system("mv tmp/doctest/include include/clipp")
    os.system("rm -rf tmp")

if not os.path.exists("include/clipp"):
    os.system("mkdir tmp")
    os.system("git clone https://github.com/doctest/doctest tmp/doctest")
    os.system(
        "rm -rf tmp/doctest/doctest/extensions tmp/doctest/doctest/BUILD.bazel")
    os.system("mv tmp/doctest/doctest include")
    os.system("rm -rf tmp")

if os.system("g++ -Ofast -omortar *.cpp include/doctest/parts/doctest.cpp -DDOCTEST_CONFIG_DISABLE -std=c++17 -lpthread -Iinclude"):
    sys.exit(1)
