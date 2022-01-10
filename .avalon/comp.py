import os, sys

if os.system("mortar > /dev/null 2>&1"):
    if os.system("g++ -Ofast -omortar *.cpp include/SHA1/sha1.cpp -std=c++17 -lpthread -Iinclude"):
        sys.exit(1)