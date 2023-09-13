CC = /home/gangdeng/llvm-project/build/bin/clang
C++ = /home/gangdeng/llvm-project/build/bin/clang++
# CC = clang
# C++ = clang++
# CFLAGS = -pthread -mcmpccxadd -I/home/gangdeng/llvm-project/build/lib/clang/17/include/
CFLAGS = -pthread -mcmpccxadd

all: futex_test

futex_test: futex_test.cpp
	$(C++) $(CFLAGS) futex_test.cpp -std=c++17 -o futex_test

clean:
	rm -f futex_test

.PHONY: clean
