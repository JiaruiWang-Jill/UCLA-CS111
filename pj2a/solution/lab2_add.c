/*
 * lab2_add.c
 * Copyright (C) 2019 Qingwei Zeng <zenn@ucla.edu>
 *
 * Distributed under terms of the MIT license.
 */

#include <errno.h>
#include <getopt.h>
#include <memory.h>
#include <pthread.h>
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PROGRAM_NAME "lab2a-add"
#define AUTHORS proper_name("Qingwei Zeng")

#define THREADS_SHORT_OPTION 256
#define ITERATIONS_SHORT_OPTION 257
#define YIELD_SHORT_OPTION 258
#define SYNC_SHORT_OPTION 259

#define EXIT_WRONG_RESULT 2

int thread_n = 1,  // number of parallel threads, defaults to 1
    iter_n = 1;    // number of iterations, defaults to 1
bool opt_yield_set = false;
char opt_sync;

int spin_lock = 0;
pthread_mutex_t mutex;
char test_name[256];
char sync_partial_name[5] = "none";

long long counter = 0;

struct timespec start_time, end_time;

char const program_name[] = PROGRAM_NAME;

void _c(int ret, char *errmsg);

void usage();

void add(long long *pointer, long long value);
void add_mutex(long long *pointer, long long value);
void add_test_and_set(long long *pointer, long long value);
void add_compare_and_swap(long long *pointer, long long value);

void thread_op();

/* options descriptor */
static struct option const long_opts[] = {
    {"threads", required_argument, NULL, THREADS_SHORT_OPTION},
    {"iterations", required_argument, NULL, ITERATIONS_SHORT_OPTION},
    {"yield", no_argument, NULL, YIELD_SHORT_OPTION},
    {"sync", required_argument, NULL, SYNC_SHORT_OPTION},
    {0, 0, 0, 0},
};

void check_opt_sync(char *optarg);

int main(int argc, char *argv[]) {
  /* option parsing */
  int optc;
  while ((optc = getopt_long(argc, argv, ":", long_opts, NULL)) != -1) {
    switch (optc) {
      case THREADS_SHORT_OPTION:
        thread_n = atoi(optarg);
        break;
      case ITERATIONS_SHORT_OPTION:
        iter_n = atoi(optarg);
        break;
      case YIELD_SHORT_OPTION:
        opt_yield_set = true;
        break;
      case SYNC_SHORT_OPTION:
        check_opt_sync(optarg);
        opt_sync = *optarg;
        switch (opt_sync) {
          case 'm':
            strcpy(sync_partial_name, "m");
            _c(pthread_mutex_init(&mutex, NULL), "Failed to initialize mutex.");
            break;
          case 's':
            strcpy(sync_partial_name, "s");
            break;
          case 'c':
            strcpy(sync_partial_name, "c");
            break;
        }
        // TODO: set sync mode
        break;
      default:
        fputs("Invalid arguments.\n", stderr);
        usage();
    }
  }

  int op_n = thread_n * iter_n * 2;

  pthread_t threads[thread_n];

  _c(clock_gettime(CLOCK_MONOTONIC, &start_time),
     "Failed to record the start time");

  for (int i = 0; i < thread_n; i++)
    _c(pthread_create(&threads[i], NULL, (void *)&thread_op, NULL),
       "Failed to create thread");

  for (int i = 0; i < thread_n; i++)
    _c(pthread_join(threads[i], NULL), "Failed to join threads");

  _c(clock_gettime(CLOCK_MONOTONIC, &end_time),
     "Failed to record the end time");

  // TODO: check the correct usage
  long long runtime = end_time.tv_nsec - start_time.tv_nsec;

  // create test name
  sprintf(test_name, "add-%s%s", opt_yield_set ? "yield-" : "",
          sync_partial_name);
  printf("%s,%d,%d,%d,%lld,%lld,%lld\n", test_name, thread_n, iter_n, op_n,
         runtime, runtime / op_n, counter);

  exit(counter != 0 ? EXIT_WRONG_RESULT : EXIT_SUCCESS);
}

void add(long long *pointer, long long value) {
  long long sum = *pointer + value;
  if (opt_yield_set) sched_yield();
  *pointer = sum;
}

void add_mutex(long long *pointer, long long value) {
  pthread_mutex_lock(&mutex);
  long long sum = *pointer + value;
  if (opt_yield_set) sched_yield();
  *pointer = sum;
  pthread_mutex_unlock(&mutex);
}

void add_test_and_set(long long *pointer, long long value) {
  while (__sync_lock_test_and_set(&spin_lock, 1))
    ;
  long long sum = *pointer + value;
  if (opt_yield_set) sched_yield();
  *pointer = sum;
  __sync_lock_release(&spin_lock);
}

void add_compare_and_swap(long long *pointer, long long value) {
  long long old;
  do {
    old = counter;
    if (opt_yield_set) sched_yield();
  } while (__sync_val_compare_and_swap(pointer, old, old + value) != old);
}

void thread_op() {
  void (*add_fn)(long long *, long long);
  switch (opt_sync) {
    case 'm':
      add_fn = add_mutex;
      break;
    case 's':
      add_fn = add_test_and_set;
      break;
    case 'c':
      add_fn = add_compare_and_swap;
      break;
    default:
      add_fn = add;
  }

  // add 1 to the counter the specified number of times
  for (long i = 0; i < iter_n; i++) add_fn(&counter, 1);
  // add -1 to the counter the specified number of times
  for (long i = 0; i < iter_n; i++) add_fn(&counter, -1);
}

void check_opt_sync(char *optarg) {
  if (strcmp(optarg, "c") == 0 && strcmp(optarg, "s") == 0 &&
      strcmp(optarg, "m") == 0) {
    fprintf(stderr, "Invalid sync option argument\n");
    usage();
  }
}

void usage() {
  fprintf(stderr,
          "\n\
Usage: %s\n\
       %s --threads=THREADNUM\n\
       %s --=iterations=ITERNUM\n\
\n\
",
          program_name, program_name, program_name);
  fputs(
      "\
--threads=THREADNUM   number of threads to use, defaults to 1\n\
--iterations=ITERNUM  number of iterations to run, defaults to 1\n\
",
      stderr);
  // TODO: yield options, sync options
  exit(EXIT_FAILURE);
}

/* syscall check */
void _c(int ret, char *errmsg) {
  if (ret != -1) return;  // syscall suceeded
  fprintf(stderr, "%s: %s. errno %d\n", errmsg, strerror(errno), errno);
  exit(EXIT_FAILURE);
}

