#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <linux/futex.h>
#include <syscall.h>
#include <stdio.h>
#include <atomic>
#include <math.h>
#include <sys/time.h>
#include <x86gprintrin.h>
// #include <immintrin.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdlib.h> 

static const uint32_t WRITE_MASK = 1<<30;
static const uint32_t WAIT_MASK = 1<<31;

int main(int argc, char* argv[]) {
    printf("WRITE_MASK %u\n", WRITE_MASK);
    printf("WAIT_MASK %u\n", WAIT_MASK);

    uint32_t lock = WRITE_MASK;    
    uint32_t old_val = _cmpccxadd_epi32(&lock, WRITE_MASK, -WRITE_MASK, _CMPCCX_Z);
    printf("old value %u, lock %u\n", old_val, lock);
    if ((WAIT_MASK == old_val)) {
        printf("lock equal to write mask\n");
    }
}