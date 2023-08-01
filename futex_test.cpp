#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <linux/futex.h>
#include <syscall.h>
#include <stdio.h>
#include <atomic>

#define NUM_READERS 100
#define NUM_WRITERS 20
#define TEST_COUNT 1000

#define VAL_EXCLUSIVE ((uint32_t)1 << 24)

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
  old_val = lock.fetch_add(0);
  while (old_val >= VAL_EXCLUSIVE)
  {
    timespec ts = make_timespec(1000);
    futex_wait(&__futex, 0, &ts);
    old_val = lock.fetch_add(0);
  }

  if (lock.fetch_add(1) >= VAL_EXCLUSIVE)
  {
    lock.fetch_sub(1);
    return false;
  }

  return true;
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
    futex_wake(&__futex, 100);
  }
  return 0;
}

int write_lock()
{
  uint32_t old_val;
  old_val = lock.fetch_add(VAL_EXCLUSIVE);
  while (old_val != 0)
  {
    timespec ts = make_timespec(100);
    futex_wait(&__futex, 0, &ts);
    old_val = lock.fetch_add(VAL_EXCLUSIVE);
  }
  return true;
}

int write_unlock()
{
  lock.fetch_sub(VAL_EXCLUSIVE);
  futex_wake(&__futex, 100);
  return 0;
}

void *reader(void *arg)
{
  for (int i = 0; i < TEST_COUNT; i++)
  {
    while (!read_lock())
    {
      // printf("read don't get lock %d\n", lock.load());
      timespec ts = make_timespec(100);
      futex_wait(&__futex, 0, &ts);
    }
    int j = 0;
    while(j < 10000) {
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
    while (!write_lock())
    {
      timespec ts = make_timespec(1000);
      futex_wait(&__futex, 0, &ts);
    }
    int j = 0;
    while(j < 10000) {
      j++;
    }    
    // printf("write set lock %d\n", lock.load());
    write_unlock();
  }
}

int main()
{
  pthread_t rthreads[NUM_READERS];
  pthread_t wthreads[NUM_WRITERS];

  struct timespec start, end;

  timespec_get(&start, TIME_UTC);
  
  for (int i = 0; i < NUM_READERS; i++)
    pthread_create(&rthreads[i], NULL, reader, NULL);

  for (int i = 0; i < NUM_WRITERS; i++)
    pthread_create(&wthreads[i], NULL, writer, NULL);
  
  for (int i = 0; i < NUM_READERS; i++)
    pthread_join(rthreads[i], NULL);

  for (int i = 0; i < NUM_WRITERS; i++)
    pthread_join(wthreads[i], NULL);

  timespec_get(&end, TIME_UTC);

  double elapsed = (end.tv_sec - start.tv_sec) +
                   (end.tv_nsec - start.tv_nsec) / 1000000000.0;

  printf("Elapsed: %f\n", elapsed);

  return 0;
}
