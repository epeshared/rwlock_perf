#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <linux/futex.h>
#include <syscall.h>
#include <stdio.h>
#include <atomic>
#include <math.h>
#include <emmintrin.h> 
#include <sys/time.h>

#define NUM_READERS 200
#define NUM_WRITERS 40
#define TEST_COUNT 10000
#define INNER_READ_TEST_COUNT  100000
#define INNER_WRITE_TEST_COUNT 1000000
#define WAKE_NUM 10
#define WAIT_TIME 100

#define VAL_EXCLUSIVE ((uint32_t)1 << 24)

// #define DEBUG_WRITE_INNER_TIME

std::atomic<uint32_t> lock(0);
volatile int __futex = 0;

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
  uint32_t old_val;
  old_val = lock.load();
  if (old_val >= VAL_EXCLUSIVE) {
    return false;
  }

  if (lock.compare_exchange_strong(old_val, old_val + 1)) {    
    return true;
  }

  return false;
}

int read_unlock()
{
  uint32_t old_val = lock.fetch_sub(1);
  if (old_val == 0)
  {
    printf("read lock value %d is 0, ERROR!\n", old_val);
    _exit(0);
  }

  if (old_val < VAL_EXCLUSIVE)
  {
    futex_wake(&__futex, WAKE_NUM);
  } else {
    printf("read lock value %d is exceed VAL_EXCLUSIVE, ERROR!\n", old_val);
    _exit(0);    
  }
  return 0;
}

int write_lock()
{
  uint32_t old_val = lock.load();  
  if (old_val == 0) {
    if (lock.compare_exchange_strong(old_val, VAL_EXCLUSIVE)) {
      return true;
    } else {
      return false;
    }    
  }
  return false;
}

int write_unlock()
{
  uint32_t old_val = lock.fetch_sub(VAL_EXCLUSIVE);
  if (old_val != VAL_EXCLUSIVE) {
    printf("write lock value %d is not 0, ERROR!\n", old_val);
    _exit(0);    
  } else {
    futex_wake(&__futex, WAKE_NUM);
  }  
  return 0;
}

void *reader(void *arg)
{
  for (int i = 0; i < TEST_COUNT; i++)
  {
    while (!read_lock()) {
      // printf("read don't get lock %d\n", lock.load());
      timespec ts = make_timespec(WAIT_TIME);
      futex_wait(&__futex, 0, &ts);
      // futex_wait(&__futex, 0, NULL);
    }
    int j = 0;
    while(j < INNER_READ_TEST_COUNT) {
      j++;
    }
    // printf("read set lock %d\n", lock.load());
    read_unlock();
  }
}

void *writer(void *arg)
{
  for (int i = 0; i < TEST_COUNT; i++)
  {
    while (!write_lock()) {
      timespec ts = make_timespec(WAIT_TIME);
      futex_wait(&__futex, 0, &ts);
      // futex_wait(&__futex, 0, NULL);
    }
    // printf("write get lock\n");
    int j = 0;
#ifdef DEBUG_WRITE_INNER_TIME
    struct timespec start, end;
    timespec_get(&start, TIME_UTC);
#endif
    while(j < INNER_WRITE_TEST_COUNT) {
      j++;
    }
#ifdef DEBUG_WRITE_INNER_TIME
    timespec_get(&end, TIME_UTC);
    double elapsed = (end.tv_sec - start.tv_sec) +
                   (end.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("Inner Elapsed: %f\n", elapsed);     
#endif   
    // printf("write set lock %d\n", lock.load());
    write_unlock();
  }
}

static inline 
uint64_t rdtsc()
{
	unsigned int lo, hi;
       	__asm__ volatile ("rdtsc" : "=a" (lo), "=d" (hi));
	return ((uint64_t)hi << 32) | lo;
}


#define TRANS_PER_THREAD	1L << 20
static long write_count=0;
void* do_trans(void *arg)
{
	int _mod = *((int*)arg);

	int sleep_count;
	//get a random value
	for(long i=0;i<TRANS_PER_THREAD;i++)
	{
//		int _rnd = rand();

		if(i % _mod == 0){
        //do write lock
      while (!write_lock()) {
        timespec ts = make_timespec(WAIT_TIME);
        futex_wait(&__futex, 0, &ts);
        // futex_wait(&__futex, 0, NULL);
      }
			write_count++;
			sleep_count = rdtsc() % 100;		
			for(int j=0;j<sleep_count;j++)
				_mm_pause();
      write_unlock();
		} else {
      while (!read_lock()) {
        // printf("read don't get lock %d\n", lock.load());
        timespec ts = make_timespec(WAIT_TIME);
        futex_wait(&__futex, 0, &ts);
        // futex_wait(&__futex, 0, NULL);
      }
			
			// if(write_count % 100000000 == 10000000) {
			// 	printf("current write count %ld\n", write_count);
      // }
      sleep_count = rdtsc() % 100;
			for(int j=0;j<sleep_count;j++)
				_mm_pause();

      read_unlock();
		}
	}
	return NULL;
}

// int main()
// {
//   pthread_t rthreads[NUM_READERS];
//   pthread_t wthreads[NUM_WRITERS];

//   struct timespec start, end;

//   timespec_get(&start, TIME_UTC);
  
//   for (int i = 0; i < NUM_READERS; i++)
//     pthread_create(&rthreads[i], NULL, reader, NULL);

//   for (int i = 0; i < NUM_WRITERS; i++)
//     pthread_create(&wthreads[i], NULL, writer, NULL);
  
//   for (int i = 0; i < NUM_READERS; i++)
//     pthread_join(rthreads[i], NULL);

//   for (int i = 0; i < NUM_WRITERS; i++)
//     pthread_join(wthreads[i], NULL);

//   timespec_get(&end, TIME_UTC);

//   double elapsed = (end.tv_sec - start.tv_sec) +
//                    (end.tv_nsec - start.tv_nsec) / 1000000000.0;

//   printf("Elapsed: %f\n", elapsed);

//   return 0;
// }


int main(int argc, char* argv[])
{	
	pthread_t* threadlist;
	int err;
	struct timeval start,end;

	if(argc < 3){
		printf("usage: %s [thread_num] [read ratio, 50 - 100]\n", argv[0]);
		return -1;
	}
	int thread_num = atoi(argv[1]);
	int read_ratio = atoi(argv[2]);			

	if(read_ratio < 50 || read_ratio >100){
		printf("read ratio should between 50 and 100\n");
	}
	int mod = 1 << 31;
	if(read_ratio != 100){
		 mod = 100/(100-read_ratio);
	}

	threadlist = (pthread_t*) malloc(sizeof(pthread_t) * thread_num);

	srand(111);

	for(int i=0;i<thread_num;i++)
	{
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
	printf("thread_num: %d: read_ratio: %d: execution time: %lf: num of write: %ld\n", thread_num, read_ratio, timeused, write_count);

	free(threadlist);

	return 0;

}
