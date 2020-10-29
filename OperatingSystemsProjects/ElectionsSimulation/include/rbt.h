/****** rbt.h ******/
/**
 * Red-Black Tree interface.
 * All keys must be unique.
 * To use this data structure, the user must give a function comparing
 * keys.
 * Optional: Freeing function that dellocates any memory allocated for
 *  a node's data.
 */
#ifndef _RBT_H
#define _RBT_H

#include "connlist.h"

typedef struct RBT_Node RBT_Node;

/* Colors */
#define BLACK 0
#define RED 1

struct RBT_Node {
  void *data; /** key inside **/
  char color;
  RBT_Node *left;
  RBT_Node *right;
  RBT_Node *parent;
};

/** User defined functions that take the RBT's nodes' keys as
 * arguments
 */

/**
 * Compares the keys of two nodes that are passed as arguments.
 * Should return:
 * 1 if data1 is of lower importance than data2, (<)
 * 0 if they are equal, (=)
 * -1 if data1 is of greater importance than data2. (>)
 */
typedef char (*RBTNodesCmpFunction)(const void * /** data1 **/,
                                    const void * /** data2 **/);
/**
 * Compares a key (1st argument) against the key of a data segment (2nd
 * argument). Same return codes as the function above.
 */
typedef char (*RBTKeyCmpFunction)(const void * /** Key **/,
                                  const void * /** Data segment **/);
/**
 * Optional:    (if not used, set to NULL at init)
 * Makes any necessary deallocations of the node's data before freeing the node
 * (either when deleting a node, or when the RBT is getting purged)
 */
typedef void (*RBTFreeFunction)(void *);

typedef struct RBT {
  RBT_Node *root;
  RBT_Node *guard;
  RBTNodesCmpFunction rbtnodescmpfunction;
  RBTKeyCmpFunction rbtkeycmpfunction;
  RBTFreeFunction rbtfreefunction;
} RBT;

/** Returns NULL if the first two arguments are NULL **/
RBT *rbt_init(RBTNodesCmpFunction rbtnodescmpfunction,
              RBTKeyCmpFunction rbtkeycmpfunction,
              RBTFreeFunction rbtfreefunction);
/** Returns 0 on success, else 1. In case of failure, the data is left untouched
 * **/
char rbt_insert(RBT *tree, void *data);
/** Returns the node's data on success, else NULL **/
void *rbt_find(RBT *tree, void *key);
/** Returns 0 on success, else 1 **/
char rbt_delete(RBT *tree, void *key);
/** Frees any memory allocated for the RBT. Uses the user-defined
 * rbtfreefunction, if it exists, to free the memory of the data
 */
void rbt_free(RBT *tree);
/** Returns a connected list containing all of the nodes' data in order **/
ConnList *rbt_get_all_nodes(RBT *tree);

#endif