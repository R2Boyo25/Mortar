AUTOMAKE_OPTIONS = foreign
bin_PROGRAMS = mortar

mortar_SOURCES = src/mortar.cpp src/configutils.cpp src/changed.cpp src/util.cpp include/doctest/parts/doctest.cpp
mortar_LDFLAGS = -lpthread

AM_CXXFLAGS = -DCOMMITHASH="\"`git rev-parse HEAD`\"" -DDOCTEST_CONFIG_DISABLE -std=c++17 -Iinclude -Wall -Wextra
