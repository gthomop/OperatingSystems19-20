/***** connlist.c *****/

#include "connlist.h"

#include <stdlib.h>

char connlist_insert_with_cmp(ConnList *list, void *data);
char connlist_insert_without_cmp(ConnList *list, void *data);

ConnList *connlist_init(ListCmpFunction listcmp, ListDelCmpFunction listdelcmp,
                        ListFreeFunction listfree) {
  if (listdelcmp == NULL) {
    return NULL;
  }

  ConnList *ret = malloc(sizeof(ConnList));
  ret->start_of_list = NULL;

  (listcmp != NULL) ? (ret->listcmp = listcmp) : (ret->listcmp = NULL);
  ret->listdelcmp = listdelcmp;
  (listfree != NULL) ? (ret->listfree = listfree) : (ret->listfree = NULL);

  return ret;
}

char connlist_insert(ConnList *list, void *data) {
  return ((list->listcmp == NULL) ? (connlist_insert_without_cmp(list, data))
                                  : (connlist_insert_with_cmp(list, data)));
}

char connlist_delete_dealloc(ConnList *list, void *data) {
  if (list->listfree == NULL) {
    return ((connlist_delete(list, data) == NULL) ? (1) : (0));
  }

  if (list->start_of_list == NULL) { /** Empty list **/
    return 1;
  }
  ListNode *curr = list->start_of_list, *previous = curr;

  while (curr != NULL) {
    if (!(list->listdelcmp(data, curr->data))) {
      previous->next = curr->next;
      list->listfree(data);
      free(curr);
      return 0;
    }
    previous = curr;
    curr = curr->next;
  }

  return 0;
}

void *connlist_delete(ConnList *list, void *data) {
  if (list->start_of_list == NULL) { /** Empty list **/
    return NULL;
  }
  ListNode *curr = list->start_of_list, *previous = curr;
  void *ret = NULL;

  while (curr != NULL) {
    if (!(list->listdelcmp(data, curr->data))) {
      previous->next = curr->next;
      ret = curr->data;
      free(curr);
      return ret;
    }
    previous = curr;
    curr = curr->next;
  }

  return ret;
}

void connlist_free_dealloc(ConnList *list) {
  if (list->listfree == NULL) {
    connlist_free(list);
    return;
  } else if (list->start_of_list == NULL) {
    free(list);
    return;
  }

  ListNode *curr = list->start_of_list->next, *previous = list->start_of_list;

  while (curr != NULL) {
    list->listfree(previous->data);
    free(previous);
    previous = curr;
    curr = curr->next;
  }

  list->listfree(previous->data);
  free(previous);

  free(list);

  return;
}

void connlist_free(ConnList *list) {
  if (list->start_of_list == NULL) {
    free(list);
    return;
  }

  ListNode *curr = list->start_of_list->next, *previous = list->start_of_list;

  while (curr != NULL) {
    free(previous);
    previous = curr;
    curr = curr->next;
  }

  free(previous);

  free(list);

  return;
}

/** Not part of the interface **/

char connlist_insert_with_cmp(ConnList *list, void *data) {
  if (list->start_of_list == NULL) {
    list->start_of_list = malloc(sizeof(ListNode));
    list->start_of_list->data = data;
    list->start_of_list->next = NULL;
    return 0;
  }

  ListNode *curr = list->start_of_list, *previous = list->start_of_list;
  char cmp = 0;

  if (list->listcmp(data, previous->data) < 0) {
    list->start_of_list = malloc(sizeof(ListNode));
    list->start_of_list->next = previous;
    list->start_of_list->data = data;

    return 0;
  }

  while (curr->next != NULL) {
    curr = curr->next;
    if ((cmp = list->listcmp(data, curr->data)) < 0) {
      previous->next = malloc(sizeof(ListNode));
      previous = previous->next;
      previous->data = data;
      previous->next = curr;

      return 0;
    } else if (cmp == 0)
      return 1;

    previous = curr;
  }

  if ((cmp = list->listcmp(data, curr->data)) < 0) {
    previous->next = malloc(sizeof(ListNode));
    previous = previous->next;
    previous->data = data;
    previous->next = curr;

    return 0;
  } else if (cmp > 0) {
    curr->next = malloc(sizeof(ListNode));
    curr = curr->next;
    curr->data = data;
    curr->next = NULL;

    return 0;
  }

  /** Execution reaches here only when the key of the to-be-inserted
   * data is equal to the key of the last node
   */
  return 1;
}

char connlist_insert_without_cmp(ConnList *list, void *data) {
  if (list->start_of_list == NULL) {
    list->start_of_list = malloc(sizeof(ListNode));
    list->start_of_list->data = data;
    list->start_of_list->next = NULL;

    return 0;
  }

  ListNode *curr = list->start_of_list;

  while (curr->next != NULL) {
    curr = curr->next;
  }

  curr->next = malloc(sizeof(ListNode));
  curr = curr->next;
  curr->data = data;
  curr->next = NULL;

  return 0;
}