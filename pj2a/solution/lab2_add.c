/*
 * lab2_add.c
 * Copyright (C) 2019 Qingwei Zeng <zenn@ucla.edu>
 *
 * Distributed under terms of the MIT license.
 */

#include <errno.h>
#include <getopt.h>
#include <pthread.h>
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

struct addArgs {
  long long *pointer;
  long long value;
};

char const program_name[] = PROGRAM_NAME;

void _c(int ret, char *errmsg);

void usage();

void add(long long *pointer, long long value);

void timer_start();
void timer_end();

/* options descriptor */
static struct option const long_opts[] = {
    {"threads", required_argument, NULL, THREADS_SHORT_OPTION},
    {"iterations", required_argument, NULL, ITERATIONS_SHORT_OPTION},
    {"yield", required_argument, NULL, YIELD_SHORT_OPTION},
    {"sync", required_argument, NULL, SYNC_SHORT_OPTION},
    {0, 0, 0, 0},
};

int threads = 1, iterations = 1;

struct timespec start_time, end_time;

int main(int argc, char *argv[]) {
  long long counter = 0;

  /* option parsing */
  int optc;
  while ((optc = getopt_long(argc, argv, ":", long_opts, NULL)) != -1) {
    switch (optc) {
      case THREADS_SHORT_OPTION:
        break;
      case ITERATIONS_SHORT_OPTION:
        break;
      case YIELD_SHORT_OPTION:
        break;
      case SYNC_SHORT_OPTION:
        break;
      default:
        fputs("Invalid arguments.\n", stderr);
        usage();
    }
  }

  /* pre-threading */
  pthread_t thread_id;
  timer_start();

  /* run the threads */
  for (int i = 0; i < threads; i++)
    _c(pthread_create(&thread_id, NULL, add, NULL), "Failed to create thread");
  _c(pthread_join(thread_id, NULL), "Failed to join threads");

  /* post-threading */

  exit(EXIT_SUCCESS);
}

void add(long long *pointer, long long value) {
  long long sum = *pointer + value;
  *pointer = sum;
}

void timer_start() {
  _c(clock_gettime(CLOCK_REALTIME, &start_time),
     "Failed to retrieve the start time");
}

void timer_end() {
  _c(clock_gettime(CLOCK_REALTIME, &end_time),
     "Failed to retrieve the end time");
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
  exit(EXIT_FAILURE);
}

/* syscall check */
void _c(int ret, char *errmsg) {
  if (ret != -1) return;  // syscall suceeded
  fprintf(stderr, "%s: %s. errno %d\n", errmsg, strerror(errno), errno);
  exit(EXIT_FAILURE);
}

