/*
 * SortedList.c
 * Copyright (C) 2019 Qingwei Zeng <zenn@ucla.edu>
 *
 * Distributed under terms of the MIT license.
 */

#include "SortedList.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

inline bool SortedList_is_head(SortedList_t *element);

void SortedList_insert(SortedList_t *list, SortedListElement_t *elem) {
  const char ERR_PROMPT[] = "Failed to insert the given element to the list";

  if (list == NULL) {
    fprintf(stderr, "%s: the given list is NULL", ERR_PROMPT);
    exit(EXIT_FAILURE);
  }
  if (elem == NULL) {
    fprintf(stderr, "%s: the given element is NULL", ERR_PROMPT);
    exit(EXIT_FAILURE);
  }

  for (SortedListElement_t *curr = list->next; !SortedList_is_head(curr->next);
       curr = curr->next) {
    if (elem->key < curr->key) {
      elem->prev = curr->prev;
      elem->next = curr;
      elem->prev->next = elem;
      curr->prev = elem;
    }
  }
}

int SortedList_delete(SortedListElement_t *elem) {
  const char ERR_PROMPT[] = "Failed to delete the given element";

  if (elem == NULL) {
    fprintf(stderr, "%s: the given element is NULL", ERR_PROMPT);
    return 1;
  }
  if (elem->prev == NULL || elem->next == NULL) {
    fprintf(stderr,
            "%s: the given element is not part of a well-defined SortedList",
            ERR_PROMPT);
    return 1;
  }

  elem->prev->next = elem->next;
  elem->next->prev = elem->prev;
  return 0;
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {
  const char ERR_PROMPT[] = "Failed to lookup the given key in the list";

  if (list == NULL) {
    fprintf(stderr, "%s: the given list is NULL", ERR_PROMPT);
    exit(EXIT_FAILURE);
  }

  for (SortedListElement_t *curr = list->next; !SortedList_is_head(curr->next);
       curr = curr->next)
    if (curr->key == key) return curr;

  return NULL;
}

int SortedList_length(SortedList_t *list) {
  const char ERR_PROMPT[] = "Failed to find the length of the list";

  if (list == NULL) {
    fprintf(stderr, "%s: the given list is NULL", ERR_PROMPT);
    exit(EXIT_FAILURE);
  }

  size_t count = 0;
  for (SortedListElement_t *curr = list->next; !SortedList_is_head(curr->next);
       curr = curr->next)
    count++;

  return count;
}

/* custom functions */

/**
 * SortedList_is_head ... check whether the given element is a list head
 *
 * @param SortedList_t *element ... element to be checked
 */
inline bool SortedList_is_head(SortedList_t *elem) { return elem->key == NULL; }
