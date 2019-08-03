/*
 * SortedListTest.c
 * Copyright (C) 2019 Jim Zenn <zenn@ucla.edu>
 *
 * Distributed under terms of the MIT license.
 */

#include <stdlib.h>
#include "SortedList.h"

int opt_yield = 0;

int main() {
  SortedList_t list;
  SortedList_t *head = &list;
  head->key = NULL;
  head->next = head;
  head->prev = head;

  SortedList_t a, b, c, d, e, f, g, h, i;
  a.key = "a";
  b.key = "a";
  c.key = "a";
  d.key = "a";
  e.key = "e";
  f.key = "f";
  g.key = "g";
  h.key = "j";
  i.key = "j";

  SortedList_insert(head, &i);
  SortedList_insert(head, &b);
  SortedList_insert(head, &c);
  SortedList_insert(head, &d);
  SortedList_insert(head, &a);
  SortedList_insert(head, &f);
  SortedList_insert(head, &g);
  SortedList_insert(head, &h);
  SortedList_insert(head, &e);

  SortedList_length(head);

  return 0;
}

