/***** linkedList.h *****/
/**
 * Linked list interface.
 * Optional: Comparing function. It is used in delete functions and, if
 *  ordered is chosen, it is used when inserting a node.
 * Optional: Freeing function that deallocates the data's memory.
 *  Functions with suffix _f use this, if it exists.
 */
#ifndef _LINKEDLIST_H
#define _LINKEDLIST_H

typedef struct ListNode ListNode;

struct ListNode {
  void *data; /** key inside **/
  ListNode *next;
};

/** Optional:   (if not used, set to NULL at init)
 *  Should return:
 *  1 if data1 is of lower importance than data2 (<),
 *  0 if data1 is equal to data2 (=),
 *  -1 if data1 is of higher importance than data2 (>).
 */
typedef char (*LinkedListCmpFunction)(const void * /** data1 **/,
                                      const void * /** data2 **/);
/** Optional:   (if not used, set to NULL at init)
 *  Deallocates any memory inside a node's data.
 */
typedef void (*LinkedListFreeFunction)(void *);

typedef struct LinkedList {
  ListNode *firstNode;
  LinkedListCmpFunction listCmp;
  LinkedListFreeFunction listFree;
  const char ordered;
} LinkedList;

/* Initializes the linked list. If ordered == 1 then the nodes will be ordered.
 */
LinkedList *linkedListInit(LinkedListCmpFunction listCmp,
                           LinkedListFreeFunction listFree, char ordered);
/** Returns 0 on successful insertion, else 1. In case of failure, the data is
 * left untouched. **/
int linkedListInsert(LinkedList *list, void *data);
/** Returns 0 on successful deletion, else 1. Uses listfree function, if there
 * is one **/
int linkedListDelete_f(LinkedList *list, void *data);
/** Returns the data's pointer, or NULL if the deletion was unsuccessful. Does
 * not free the data's allocated memory **/
void *linkedListDelete(LinkedList *list, void *data);
/** Frees nodes' memory. Uses the listfree function, if there is one **/
void linkedListPurge_f(LinkedList *list);
/** Frees nodes' memory, without using listfree function **/
void linkedListPurge(LinkedList *list);

#endif