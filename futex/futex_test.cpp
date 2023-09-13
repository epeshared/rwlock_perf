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

#define NUM_READERS 200
#define NUM_WRITERS 40
#define TEST_COUNT 10000
#define INNER_READ_TEST_COUNT  100000
#define INNER_WRITE_TEST_COUNT 1000000
#define WAKE_NUM 10

// #define CMPCCXADD

// #define WAIT_TIME 1000
uint32_t WAIT_TIME = 1000;

#define VAL_EXCLUSIVE	      ((uint32_t) ((1 << 24)-1))
#define LW_VAL_EXCLUSIVE		((uint32_t) 1 << 24)
// #define VAL_WAIT_MAX ((uint32_t)1 << 31)

// #define DEBUG_WRITE_INNER_TIME

#ifdef CMPCCXADD
static uint32_t lock = 0;
#else
std::atomic<uint32_t> lock(0);
#endif
std::atomic<uint32_t> read_attemps(0);
std::atomic<uint32_t> write_attemps(0);
volatile int __futex = 0;
static int mod = 1 << 31;

extern "C" {
int __attribute__((weak)) futex_hook(uint32_t *uaddr, int futex_op, uint32_t val, const struct timespec *timeout)
{
  return syscall(SYS_futex, uaddr, futex_op, val, timeout);
}
}

static struct timespec make_timespec(int64_t us)
{
  timespec ts;
  ts.tv_sec = us / 1000000;
  ts.tv_nsec = 1000 * (us % 1000000);
  return ts;
}

extern "C" {
extern int futex_hook(uint32_t *uaddr, int futex_op, uint32_t val, const struct timespec *timeout);
}

#define futex(...) futex_hook(__VA_ARGS__)

inline int futex_wait(volatile int *p, int val, const timespec *timeout)
{
  int ret = 0;
  if (0 != futex((uint32_t *)p, FUTEX_WAIT_PRIVATE, val, timeout))
  {
    ret = -1;
  }
  return ret;
}

inline int futex_wake(volatile int *p, int val)
{
  return futex((uint32_t *)p, FUTEX_WAKE_PRIVATE, val, NULL);
}

int read_lock()
{
#ifdef CMPCCXADD
	uint32_t old_val;
	uint32_t threshold = VAL_EXCLUSIVE;

  // printf("read lock %d\n", lock);
  old_val = _cmpccxadd_epi32(&lock, VAL_EXCLUSIVE, 1, _CMPCCX_B);
  // printf("read lock old_val %d lock is %d\n", old_val, lock);
  
  if (old_val < threshold) {
    return true;
  }

  return false;
#else
  uint32_t old_val;
  old_val = lock.load();
  if (old_val >= VAL_EXCLUSIVE) {
    return false;
  }

  if (lock.compare_exchange_strong(old_val, old_val + 1)) {    
    return true;
  }

  return false;
#endif
}

int read_unlock()
{
  uint32_t old_val;
#ifdef CMPCCXADD
  old_val = __atomic_fetch_sub(&lock, 1, __ATOMIC_SEQ_CST);
  // old_val = _cmpccxadd_epi32(&lock, VAL_EXCLUSIVE, -1, _CMPCCX_L);
#else
  old_val = lock.fetch_sub(1);
#endif  
  if (old_val == 0)
  {
    printf("read lock old_val value is %d lock is %d, ERROR!\n", old_val, lock.load());
    _exit(0);
  }

  if (old_val < LW_VAL_EXCLUSIVE)
  {
    futex_wake(&__futex, WAKE_NUM);
  } else {
    printf("read lock old_val %d is exceed VAL_EXCLUSIVE, ERROR!\n", old_val);
    _exit(0);    
  }
  return 0;

}

int write_lock()
{
#ifdef CMPCCXADD
	uint32_t old_val;
	uint32_t threshold = 1;

  // printf("write lock %d\n", lock);
  // printf("1\n");
  old_val = _cmpccxadd_epi32(&lock, 1, LW_VAL_EXCLUSIVE, _CMPCCX_L);
  // printf("2\n");
  // printf("write lock old_val %d\n", old_val);

  // if (old_val != 0) {
  //   printf("write_lock pld value %d is below 0, ERROR!\n", old_val);
  //   _exit(0);     
  // }

  if (old_val < 1) {
    // printf("write lock get lock by old vlaue %d, lock %d\n", old_val, lock);
    return true;
  }

  return false;
#else
  uint32_t old_val = lock.load();  
  if (old_val == 0) {
    if (lock.compare_exchange_strong(old_val, LW_VAL_EXCLUSIVE)) {
      return true;
    } else {
      return false;
    }    
  }
  return false;
#endif
}

int write_unlock()
{
  uint32_t old_val;
#ifdef CMPCCXADD
  old_val = __atomic_fetch_sub(&lock, LW_VAL_EXCLUSIVE, __ATOMIC_SEQ_CST);
#else
  old_val = lock.fetch_sub(LW_VAL_EXCLUSIVE);
#endif
  if (old_val != LW_VAL_EXCLUSIVE) {
    printf("write lock value %d is not %d, ERROR!\n", old_val, VAL_EXCLUSIVE);
    _exit(0);    
  } else {
    // printf("write lock value %d is %d!\n", old_val, VAL_EXCLUSIVE);
    futex_wake(&__futex, WAKE_NUM);
  }
  
  return 0;
}

static inline 
uint64_t rdtsc()
{
	unsigned int lo, hi;
       	__asm__ volatile ("rdtsc" : "=a" (lo), "=d" (hi));
	return ((uint64_t)hi << 32) | lo;
}


// #define TRANS_PER_THREAD	1L << 20
// #define TRANS_PER_THREAD 100000
#define TRANS_PER_THREAD 100000
static long write_count=0;
static long read_count=0;
static int sleep_count = 10;
void* do_trans(void *arg)
{
	// int _mod = *((int*)arg);
  // uint32_t _mod = 1 << 31;
  // printf("mod is %d\n",_mod);
	for(int i=1;i<TRANS_PER_THREAD;i++)
	{
		if(i % mod == 0){
  //       //do write lock      
      while (!write_lock()) {
        // printf("try write lock\n");
        write_attemps++;
        timespec ts = make_timespec(WAIT_TIME);
        futex_wait(&__futex, 0, &ts);
      }
      // printf("got write lock\n");
			write_count++;
			// sleep_count = rdtsc() % 100;
			for(int j=0;j<sleep_count;j++)
				_mm_pause();
      write_unlock();
      // printf("unlock write lock\n");
		} else {
      while (!read_lock()) {
        // printf("try read lock\n");
        read_attemps++;
        timespec ts = make_timespec(WAIT_TIME);
        futex_wait(&__futex, 0, &ts);
      }
      // printf("got read lock\n");
			read_count++;
      // sleep_count = rdtsc() % 100;
			for(int j=0;j<sleep_count;j++)
				_mm_pause();
      read_unlock();
      // printf("unlock read lock\n");
		}
	}
	return NULL;
}

int main(int argc, char* argv[])
{	
	pthread_t* threadlist;
	int err;
	struct timeval start,end;

  lock = 0;

	if(argc < 3){
		printf("usage: %s [thread_num] [read ratio, 50 - 100]\n", argv[0]);
		return -1;
	}
	int thread_num = atoi(argv[1]);
	int read_ratio = atoi(argv[2]);

  // printf("thread num %d\n", thread_num);

	if(read_ratio < 50 || read_ratio >100){
		printf("read ratio should between 50 and 100\n");
	}
	
	if(read_ratio != 100){
		 mod = 100/(100-read_ratio);
	}

  // printf("mod: %ld\n", mod);

  while (sleep_count <=200) {
    while (WAIT_TIME <= 1000) {
      threadlist = (pthread_t*) malloc(sizeof(pthread_t) * thread_num);
      write_count = 0;
      read_count = 0;
      write_attemps = 0;
      read_attemps = 0;
      srand(111);    
      for(int i=0;i<thread_num;i++)
      {
        // printf("creating %d thread\n", i);
        err = pthread_create(&threadlist[i],NULL,do_trans,&mod);
        if(err){
          printf("new thread create failed\n");
          return -1;
        }
      }
      gettimeofday(&start,NULL);
      
      for(int i=0;i<thread_num;i++){
        pthread_join(threadlist[i],NULL);
      }
      gettimeofday(&end,NULL);

      double timeused = ( end.tv_sec - start.tv_sec ) + (end.tv_usec - start.tv_usec)/1000000.0;
      // printf("thread_num: %d: read_ratio: %d: execution time: %lf: num of write: %ld num of read: %ld write_attempt: %ld read_attempt: %ld\n", 
      //     thread_num, read_ratio, timeused, write_count, read_count, write_attemps.load(), read_attemps.load());	
      printf("%d; %d; %d; %ld; %lf; %ld; %ld; %ld; %ld;\n", thread_num, sleep_count, read_ratio, (WAIT_TIME/1000), timeused, write_count, read_count, write_attemps.load(), read_attemps.load());  

      free(threadlist);

      WAIT_TIME = WAIT_TIME * 2;
    }
    WAIT_TIME = 1000;
    sleep_count = sleep_count + 10;  
  }




	return 0;

}
