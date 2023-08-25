CC = gcc
C++ = g++
CFLAGS = -pthread

all: futex_test

futex_test: futex_test.cpp
	$(C++) $(CFLAGS) futex_test.cpp -std=c++17 -O3 -o futex_test

clean:
	rm -f futex_test

.PHONY: clean
