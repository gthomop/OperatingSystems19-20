/***** linkedList.c *****/
#include "linkedList.h"

#include <stdlib.h>
#include <string.h>

int insertOrdered(LinkedList *list, void *data);
int insertUnordered(LinkedList *list, void *data);

LinkedList *linkedListInit(LinkedListCmpFunction listCmp,
                           LinkedListFreeFunction listFree, char ordered) {
  /* Create a local LinkedList variable due to const char ordered */
  LinkedList tmp = {NULL, listCmp, listFree, ordered};
  LinkedList *ret = malloc(sizeof(LinkedList));

  memcpy(ret, &tmp, sizeof(LinkedList));

  return ret;
}

int linkedListInsert(LinkedList *list, void *data) {
  return ((list->ordered == 1) ? (insertOrdered(list, data))
                               : (insertUnordered(list, data)));
}

int linkedListDelete_f(LinkedList *list, void *data) {
  /* No delete function specified or empty list */
  if (list->listCmp == NULL || list->firstNode == NULL) {
    return EXIT_FAILURE;
  }
  /* No deallocation function specified */
  else if (list->listFree == NULL) {
    return ((linkedListDelete(list, data) == NULL) ? (EXIT_FAILURE)
                                                   : (EXIT_SUCCESS));
  }

  ListNode *curr = list->firstNode, *previous = curr;

  while (curr != NULL) {
    if (!(list->listCmp(data, curr->data))) {
      previous->next = curr->next;
      list->listFree(data);
      free(curr);
      return EXIT_SUCCESS;
    }
    previous = curr;
    curr = curr->next;
  }

  return EXIT_FAILURE;
}

void *linkedListDelete(LinkedList *list, void *data) {
  /* No delete function specified or the list is empty */
  if (list->listCmp == NULL || list->firstNode == NULL) {
    return NULL;
  }
  ListNode *curr = list->firstNode, *previous = curr;
  void *ret = NULL;

  while (curr != NULL) {
    if (!(list->listCmp(data, curr->data))) {
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

void linkedListPurge_f(LinkedList *list) {
  if (list->listFree == NULL) {
    linkedListPurge(list);
    return;
  } else if (list->firstNode == NULL) {
    free(list);
    return;
  }

  ListNode *curr = list->firstNode->next, *previous = list->firstNode;

  while (curr != NULL) {
    list->listFree(previous->data);
    free(previous);
    previous = curr;
    curr = curr->next;
  }

  list->listFree(previous->data);
  free(previous);

  free(list);

  return;
}

void linkedListPurge(LinkedList *list) {
  if (list->firstNode == NULL) {
    free(list);
    return;
  }

  ListNode *curr = list->firstNode->next, *previous = list->firstNode;

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

int insertOrdered(LinkedList *list, void *data) {
  if (list->firstNode == NULL) {
    list->firstNode = malloc(sizeof(ListNode));
    list->firstNode->data = data;
    list->firstNode->next = NULL;
    return EXIT_SUCCESS;
  }

  ListNode *curr = list->firstNode, *previous = list->firstNode;
  char cmp = 0;

  if (list->listCmp(data, previous->data) < 0) {
    list->firstNode = malloc(sizeof(ListNode));
    list->firstNode->next = previous;
    list->firstNode->data = data;

    return EXIT_SUCCESS;
  }

  while (curr->next != NULL) {
    curr = curr->next;
    if ((cmp = list->listCmp(data, curr->data)) < 0) {
      previous->next = malloc(sizeof(ListNode));
      previous = previous->next;
      previous->data = data;
      previous->next = curr;

      return EXIT_SUCCESS;
    } else if (cmp == 0)
      return EXIT_FAILURE;

    previous = curr;
  }

  if ((cmp = list->listCmp(data, curr->data)) < 0) {
    previous->next = malloc(sizeof(ListNode));
    previous = previous->next;
    previous->data = data;
    previous->next = curr;

    return EXIT_SUCCESS;
  } else if (cmp > 0) {
    curr->next = malloc(sizeof(ListNode));
    curr = curr->next;
    curr->data = data;
    curr->next = NULL;

    return EXIT_SUCCESS;
  }

  /** Execution reaches here only when the key of the to-be-inserted
   * data is equal to the key of the last node
   */
  return EXIT_FAILURE;
}

int insertUnordered(LinkedList *list, void *data) {
  if (list->firstNode == NULL) {
    list->firstNode = malloc(sizeof(ListNode));
    list->firstNode->data = data;
    list->firstNode->next = NULL;

    return EXIT_SUCCESS;
  }

  ListNode *curr = list->firstNode;

  while (curr->next != NULL) {
    curr = curr->next;
  }

  curr->next = malloc(sizeof(ListNode));
  curr = curr->next;
  curr->data = data;
  curr->next = NULL;

  return EXIT_SUCCESS;
}