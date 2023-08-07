CC = gcc
C++ = g++
CFLAGS = -pthread

all: futex_test

futex_test: futex_test.cpp
	$(C++) $(CFLAGS) futex_test.cpp -o futex_test

clean:
	rm -f futex_test

.PHONY: clean
