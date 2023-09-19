CC = /home/gangdeng/llvm-project/build/bin/clang
C++ = /home/gangdeng/llvm-project/build/bin/clang++
# CC = clang
# C++ = clang++
# CFLAGS = -pthread -mcmpccxadd -I/home/gangdeng/llvm-project/build/lib/clang/17/include/
CFLAGS = -pthread -mcmpccxadd

all: test

test: test.cpp
	$(C++) $(CFLAGS) test.cpp -std=c++17 -o test

clean:
	rm -f test

.PHONY: clean
