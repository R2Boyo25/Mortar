import os
import sys

os.system("$SHELL .avalon/fetchdeps.sh")

if len(sys.argv) > 1:
    if (sys.argv[1] == "--tests"):
        if os.system("g++ -Ofast -omortar *.cpp include/doctest/parts/doctest.cpp -DCOMMITHASH=\"\\\"`git rev-parse HEAD`\\\"\" -DDOCTEST_CONFIG_INCLUDE_TYPE_TRAITS -DDOCTEST_CONFIG_NO_UNPREFIXED_OPTIONS -std=c++17 -lpthread -Iinclude"):
            sys.exit(1)
        sys.exit(0)
if os.system("g++ -Ofast -omortar *.cpp include/doctest/parts/doctest.cpp -DCOMMITHASH=\"\\\"`git rev-parse HEAD`\\\"\" -DDOCTEST_CONFIG_DISABLE -std=c++17 -lpthread -Iinclude"):
    sys.exit(1)
