import os, sys

print("Checking for mortar")
if os.system("mortar"):
    print("Well, I didn't find it so I'll compile it with g++")
    if os.system("g++ -Ofast -omortar *.cpp -std=c++17 -lpthread -Iinclude"):
        sys.exit(1)