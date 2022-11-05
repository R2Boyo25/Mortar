import os
import sys

if os.system("g++ -Ofast -omortar *.cpp -std=c++17 -lpthread -Iinclude"):
    sys.exit(1)
