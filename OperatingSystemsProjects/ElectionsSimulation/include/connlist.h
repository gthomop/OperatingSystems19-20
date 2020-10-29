/***** connlist.h *****/
/**
 * Connected list interface.
 * Optional: Comparing function for ordered connected list.
 * Compulsory: Delete comparison function comparing two pieces of data and
 * decide if they are the same. The data inside the nodes must be unique in
 * order for the deletion to work as intended. Optional: Freeing function that
 * deallocates any memory inside a node's data. Can be used to free both the
 * nodes' data and the nodes simultaneously
 */
#ifndef _CONNLIST_H
#define _CONNLIST_H

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
typedef char (*ListCmpFunction)(const void * /** data1 **/,
                                const void * /** data2 **/);
/** Compulsory:
 *  Should return 0 if the key (1st argument) is identical (==) to the
 *  key in the data of the nodes (argument 2)
 */
typedef char (*ListDelCmpFunction)(const void * /** Key **/,
                                   const void * /** Data **/);
/** Optional:   (if not used, set to NULL at init)
 *  Deallocates any memory inside a node's data.
 */
typedef void (*ListFreeFunction)(void *);

typedef struct ConnList {
  ListNode *start_of_list;
  ListCmpFunction listcmp;
  ListDelCmpFunction listdelcmp;
  ListFreeFunction listfree;
} ConnList;

/** Returns NULL if the user has not set a delete function **/
ConnList *connlist_init(ListCmpFunction listcmp,
                        /* Compulsory */ ListDelCmpFunction listdelete,
                        ListFreeFunction listfree);
/** Returns 0 on successful insertion, else 1. In case of failure, the data is
 * left untouched. **/
char connlist_insert(ConnList *list, void *data);
/** Returns 0 on successful deletion, else 1. Uses listfree function, if there
 * is one **/
char connlist_delete_dealloc(ConnList *list, void *data);
/** Returns the data's pointer, or NULL if the deletion was unsuccessful. Does
 * not free the data's allocated memory **/
void *connlist_delete(ConnList *list, void *data);
/** Frees nodes' memory. Uses the listfree function, if there is one **/
void connlist_free_dealloc(ConnList *list);
/** Frees nodes' memory, without using listfree function **/
void connlist_free(ConnList *list);

#endif