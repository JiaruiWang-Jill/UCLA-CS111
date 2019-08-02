/*
 * SortedList.c
 * Copyright (C) 2019 Qingwei Zeng <zenn@ucla.edu>
 *
 * Distributed under terms of the MIT license.
 */

#include "SortedList.h"
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void SortedList_insert(SortedList_t *list, SortedListElement_t *elem) {
  const char ERR_PROMPT[] = "Failed to insert the given element to the list";

  if (list == NULL) {
    fprintf(stderr, "%s: the given list is NULL\n", ERR_PROMPT);
    exit(EXIT_FAILURE);
  }
  if (elem == NULL) {
    fprintf(stderr, "%s: the given element is NULL\n", ERR_PROMPT);
    exit(EXIT_FAILURE);
  }
  if (list->next == NULL) {
    fprintf(stderr, "%s: the given list's next is NULL\n", ERR_PROMPT);
    exit(EXIT_FAILURE);
  }

  for (SortedListElement_t *curr = list->next; curr->next != list;
       curr = curr->next) {
    if (elem->key < curr->key) {
      elem->prev = curr->prev;
      elem->next = curr;
      elem->prev->next = elem;
      curr->prev = elem;
      return;
    }
  }

  elem->prev = list->prev;
  elem->next = list;
  elem->prev->next = elem;
  list->prev = elem;
  printf("inserted\n");
}

int SortedList_delete(SortedListElement_t *elem) {
  printf("delete called!\n");
  const char ERR_PROMPT[] = "Failed to delete the given element";

  if (elem == NULL) {
    fprintf(stderr, "%s: the given element is NULL\n", ERR_PROMPT);
    return 1;
  }
  if (elem->prev == NULL || elem->next == NULL) {
    fprintf(stderr,
            "%s: the given element is not part of a well-defined SortedList",
            ERR_PROMPT);
    return 1;
  }

  if (opt_yield & DELETE_YIELD) sched_yield();

  elem->prev->next = elem->next;
  elem->next->prev = elem->prev;
  return 0;
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {
  printf("lookup called!\n");
  const char ERR_PROMPT[] = "Failed to lookup the given key in the list";

  if (list == NULL) {
    fprintf(stderr, "%s: the given list is NULL\n", ERR_PROMPT);
    exit(EXIT_FAILURE);
  }

  for (SortedListElement_t *curr = list->next; curr->next != list;
       curr = curr->next) {
    if (opt_yield & LOOKUP_YIELD) sched_yield();
    if (strcmp(curr->key, key) == 0) return curr;
  }

  return NULL;
}

int SortedList_length(SortedList_t *list) {
  printf("length called!\n");
  const char ERR_PROMPT[] = "Failed to find the length of the list";

  if (list == NULL) {
    fprintf(stderr, "%s: the given list is NULL\n", ERR_PROMPT);
    exit(EXIT_FAILURE);
  }

  size_t count = 0;
  for (SortedListElement_t *curr = list->next; curr->next != list;
       curr = curr->next) {
    if (opt_yield & LOOKUP_YIELD) sched_yield();
    count++;
  }

  return count;
}

