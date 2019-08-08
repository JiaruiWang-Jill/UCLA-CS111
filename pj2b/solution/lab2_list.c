/*
 * lab2_list.c
 * Copyright (C) 2019 Qingwei Zeng <zenn@ucla.edu>
 *
 * Distributed under terms of the MIT license.
 */

#include "SortedList.h"

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

#define PROGRAM_NAME "lab2-list"
#define AUTHORS proper_name("Qingwei Zeng")

#define THREADS_SHORT_OPTION 256
#define ITERATIONS_SHORT_OPTION 257
#define YIELD_SHORT_OPTION 258
#define SYNC_SHORT_OPTION 259
#define LISTS_SHORT_OPTION 260

#define MAX_SUM 100000000ul

#define EXIT_WRONG_RESULT 2

#define KEY_LENGTH 512

const int BILLION = 1000000000;

size_t thread_num = 1;  // number of parallel threads, defaults to 1
size_t iter_num = 1;    // number of iterations, defaults to 1
size_t list_num = 1;    // number of sublists
long list_len = 0;

char opt_sync;
int opt_yield = 0;

int *spin_locks;
pthread_mutex_t *mutexes;
char test_name[256];
char yield_option_str[5] = "none";
char sync_option_str[5] = "none";

SortedList_t *lists;
SortedListElement_t *elems;

long long *lock_time;
struct timespec start_time, finish_time;

char const program_name[] = PROGRAM_NAME;

/* system call check */
void _c(int ret, char *errmsg);

/* memory alloc check */
void _m(void *ret, char *name);

void usage();

long long diff_time(struct timespec *start, struct timespec *end);

void initialize_locks();
void lock_list(size_t list_i, size_t tid);
void unlock_list(size_t list_i);

void generate_random_key(char *key);
void thread_op(void *tid_);

/* options descriptor */
static struct option const long_opts[] = {
    {"threads", required_argument, NULL, THREADS_SHORT_OPTION},
    {"iterations", required_argument, NULL, ITERATIONS_SHORT_OPTION},
    {"lists", required_argument, NULL, LISTS_SHORT_OPTION},
    {"yield", required_argument, NULL, YIELD_SHORT_OPTION},
    {"sync", required_argument, NULL, SYNC_SHORT_OPTION},
    {0, 0, 0, 0},
};

void check_opt_sync(char *optarg);

int main(int argc, char *argv[]) {
  /* seed random */
  srand(time(0));
  /* option parsing */
  int optc;
  while ((optc = getopt_long(argc, argv, ":", long_opts, NULL)) != -1) {
    switch (optc) {
      case THREADS_SHORT_OPTION:
        thread_num = atoi(optarg);
        break;
      case ITERATIONS_SHORT_OPTION:
        iter_num = atoi(optarg);
        break;
      case LISTS_SHORT_OPTION:
        list_num = atoi(optarg);
        break;
      case YIELD_SHORT_OPTION:
        for (size_t i = 0; i < strlen(optarg); i++) {
          switch (optarg[i]) {
            case 'i':
              opt_yield |= INSERT_YIELD;
              strcpy(yield_option_str, optarg);
              break;
            case 'd':
              opt_yield |= DELETE_YIELD;
              strcpy(yield_option_str, optarg);
              break;
            case 'l':
              opt_yield |= LOOKUP_YIELD;
              strcpy(yield_option_str, optarg);
              break;
            default:
              fprintf(stderr, "Invalid yield option argument\n");
          }
        }
        break;
      case SYNC_SHORT_OPTION:
        check_opt_sync(optarg);
        opt_sync = *optarg;
        sync_option_str[0] = opt_sync;
        sync_option_str[1] = '\0';
        break;
      default:
        fputs("Invalid arguments.\n", stderr);
        usage();
    }
  }

  size_t elem_num = thread_num * iter_num;

  /* start allocating memory */
  initialize_locks();
  // allocate memory for SortedList head elements
  _m(lists = malloc(list_num * sizeof(SortedListElement_t)), "list heads");
  // initialize the head elements of new empty SortedLists to make sure:
  for (size_t i = 0; i < list_num; i++) {
    SortedList_t *head = &lists[i];
    // 1. head's next is head itself
    head->next = head;
    // 2. head's prev is head itself
    head->prev = head;
    // 3. head's key is NULL
    head->key = NULL;
  }
  // initialize the lock time arrays
  _m(lock_time = calloc(thread_num, sizeof(long long)), "lock time recorder");
  // initialize the list elements
  _m(elems = malloc(sizeof(SortedListElement_t) * elem_num), "list elements");

  char *key;
  for (size_t i = 0; i < elem_num; i++) {
    _m(key = malloc(sizeof(char) * KEY_LENGTH), "key");
    generate_random_key(key);
    elems[i].key = key;
  }

  // initialize threads
  pthread_t *threads;
  _m(threads = malloc(sizeof(pthread_t) * thread_num), "thread ids");

  // initialize thread indexes
  int *ti;
  _m(ti = malloc(sizeof(int) * thread_num), "thread indices");
  for (size_t i = 0; i < thread_num; i++) ti[i] = i;
  /* finish allocating memory */

  /* start threads */
  _c(clock_gettime(CLOCK_MONOTONIC, &start_time),
     "Failed to record the start time");

  for (size_t t = 0; t < thread_num; t++)
    _c(pthread_create(&threads[t], NULL, (void *)&thread_op, (void *)&ti[t]),
       "Failed to create thread");
  for (size_t t = 0; t < thread_num; t++)
    _c(pthread_join(threads[t], NULL), "Failed to join threads");

  _c(clock_gettime(CLOCK_MONOTONIC, &finish_time),
     "Failed to record the finish time");
  /* finish threads */

  long list_len = 0;
  for (size_t i = 0; i < list_num; i++)
    list_len += SortedList_length(&lists[i]);
  if (list_len != 0)
    fprintf(stderr, "The length of the list is %ld instead of zero\n",
            list_len);

  long long runtime = diff_time(&start_time, &finish_time);
  if (runtime < 0) {
    fprintf(stderr, "The runtime cannot be negative\n");
    exit(EXIT_FAILURE);
  }

  long long total_lock_time = 0;
  for (size_t i = 0; i < thread_num; i++) {
    if (lock_time[i] < 0) {
      fprintf(stderr, "The lock time cannot be negative\n");
      exit(EXIT_FAILURE);
    }
    total_lock_time += lock_time[i];
  }
  long op_num = thread_num * iter_num * 3;

  // create test name
  sprintf(test_name, "list-%s-%s", yield_option_str, sync_option_str);
  printf("%s,%lu,%lu,%lu,%ld,%lld,%lld,%lld\n", test_name, thread_num, iter_num,
         list_num, op_num, runtime, runtime / op_num, total_lock_time);

  /* free memory */
  free(threads);
  free(ti);
  free(mutexes);
  free(spin_locks);
  free(lock_time);
  for (size_t i = 0; i < elem_num; i++) free((void *)elems[i].key);
  free(elems);
  free(lists);

  exit(EXIT_SUCCESS);
}

void check_opt_sync(char *optarg) {
  if (strcmp(optarg, "s") == 0 && strcmp(optarg, "m") == 0) {
    fprintf(stderr, "Invalid sync option argument\n");
    usage();
  }
}

void generate_random_key(char *key) {
  for (unsigned long i = 0; i < KEY_LENGTH - 1; i++)
    key[i] = rand() % 94 + 33;  // printable character range
  key[KEY_LENGTH - 1] = '\0';
}

void thread_op(void *tid_) {
  int tid = *((int *)tid_);

  SortedListElement_t *curr, *rslt;
  char hash;
  int list_i;
  for (size_t i = 0; i < iter_num; i++) {
    curr = &(elems[tid * iter_num + i]);
    hash = *(curr->key);
    list_i = hash % list_num;
    lock_list(list_i, tid);
    SortedList_insert(&(lists[list_i]), curr);
    unlock_list(list_i);
  }

  // Note that the calculated list_len is inaccurate, since while we are
  // measuring the length of the list, the list is still being changed
  // (since we only lock one list at a time)
  // However, the correctness seem to be not the focus in this problem.
  // According to Piazza @169, we are just going to proceed with this.
  long list_len = 0;
  for (size_t i = 0; i < list_num; i++) {
    lock_list(i, tid);
    list_len += SortedList_length(&lists[i]);
    unlock_list(i);
  }

  for (size_t i = 0; i < iter_num; i++) {
    curr = &(elems[tid * iter_num + i]);
    hash = *(curr->key);
    list_i = hash % list_num;

    // lookup the element with the key in the list
    lock_list(list_i, tid);
    if ((rslt = SortedList_lookup(&lists[list_i], curr->key)) == NULL) {
      fprintf(stderr, "The expected list element is not found in the list\n");
      exit(EXIT_WRONG_RESULT);
    }
    unlock_list(list_i);

    // delete the element from the list
    lock_list(list_i, tid);
    if (SortedList_delete(rslt) != 0) {
      fprintf(stderr, "Fail to delete element\n");
      exit(EXIT_WRONG_RESULT);
    }
    unlock_list(list_i);
  }
}

void initialize_locks() {
  // initialize mutex locks
  if (opt_sync == 'm') {
    _m(mutexes = malloc(list_num * sizeof(pthread_mutex_t)), "mutex locks");
    for (size_t i = 0; i < list_num; i++)
      _c(pthread_mutex_init(&(mutexes[i]), NULL),
         "Failed to initialize mutex.\n");
  }
  // initialize spin locks with zeroes
  if (opt_sync == 's')
    _m((spin_locks = calloc(list_num, sizeof(int))), "spin locks");
}

void lock_list(size_t list_i, size_t tid) {
  struct timespec lock_start_time, lock_finish_time;

  _c(clock_gettime(CLOCK_MONOTONIC, &lock_start_time),
     "Failed to record the start time");
  if (opt_sync == 'm') pthread_mutex_lock(&(mutexes[list_i]));
  if (opt_sync == 's')
    while (__sync_lock_test_and_set(&spin_locks[list_i], 1))
      ;
  _c(clock_gettime(CLOCK_MONOTONIC, &lock_finish_time),
     "Failed to record the finish time");
  lock_time[tid] += diff_time(&lock_start_time, &lock_finish_time);
}

void unlock_list(size_t list_i) {
  if (opt_sync == 'm') pthread_mutex_unlock(&(mutexes[list_i]));
  if (opt_sync == 's') __sync_lock_release(&spin_locks[list_i]);
}

void usage() {
  fprintf(stderr,
          "\n\
Usage: %s\n\
       %s --threads=THREADNUM\n\
       %s --iterations=ITERNUM\n\
       %s --lists=LISTNUM\n\
       %s --yield=[i|d|l|id|il|dl|idl]\n\
       %s --sync=[m|s]\n\
\n\
",
          program_name, program_name, program_name, program_name, program_name,
          program_name);
  fputs(
      "\
--threads=THREADNUM            number of threads to use, defaults to 1\n\
--iterations=ITERNUM           number of iterations to run, defaults to 1\n\
--lists=LISTNUM                number of lists to use, defaults to 1\n\
--yield=[i|d|l|id|il|dl|idl]   yield mode, i: insert, d: delete, l: lookup\n\
--sync=[m|s]                   sync mode, m: mutex, s: spin\n\
",
      stderr);
  exit(EXIT_FAILURE);
}

long long diff_time(struct timespec *start, struct timespec *end) {
  return BILLION * (end->tv_sec - start->tv_sec) +
         (end->tv_nsec - start->tv_nsec);
}

void _c(int ret, char *errmsg) {
  if (ret != -1) return;  // syscall suceeded
  fprintf(stderr, "%s: %s. errno %d\n", errmsg, strerror(errno), errno);
  exit(EXIT_FAILURE);
}

void _m(void *ret, char *name) {
  if (ret != NULL) return;  // memory allocation suceeded
  fprintf(stderr, "Failed to allocate memory for %s.\n", name);
  exit(EXIT_FAILURE);
}

