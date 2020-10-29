/***** rbt.c *****/

/** Algorithms of rotations, insertions, restoration, transplant, deletion and
 * searching are adjusted versions of the respective algorithms found in the
 * book "Introduction to algorithms" by Cormen, Leiserson, Rivest and Stein
 */
#include "rbt.h"
#include "connlist.h"

#include <stdlib.h>
#include <string.h>

char bst_insert(RBT *tree, RBT_Node *z);
void rbt_restoration(RBT *tree, RBT_Node *z);
RBT_Node *rbt_find_RET_NODE(RBT *tree, void *key);
void rbt_transplant(RBT *tree, RBT_Node *u, RBT_Node *v);
RBT_Node *bst_min_of_tree(RBT *tree, RBT_Node *x);
void rbt_restore_deletion(RBT *tree, RBT_Node *x);
void rbt_free_node(RBT *tree, RBT_Node *n);
char connlist_del_function(const void *key, const void *data);
void rbt_inorder_traversal(RBT_Node *root, ConnList *list);

RBT *rbt_init(RBTNodesCmpFunction rbtnodescmpfunction,
              RBTKeyCmpFunction rbtkeycmpfunction,
              RBTFreeFunction rbtfreefunction) {
  if (rbtnodescmpfunction == NULL || rbtkeycmpfunction == NULL) {
    return NULL;
  }
  RBT *ret = malloc(sizeof(RBT));
  ret->rbtnodescmpfunction = rbtnodescmpfunction;
  ret->rbtkeycmpfunction = rbtkeycmpfunction;
  ret->rbtfreefunction = rbtfreefunction;

  ret->guard = malloc(sizeof(RBT_Node));
  ret->guard->data = NULL;
  ret->guard->color = BLACK;
  ret->guard->parent = ret->guard;
  ret->guard->left = ret->guard;
  ret->guard->right = ret->guard;

  ret->root = ret->guard;

  return ret;
}

char rbt_insert(RBT *tree, void *data) {
  RBT_Node *z = malloc(sizeof(RBT_Node));
  z->data = data;
  z->left = tree->guard;
  z->right = tree->guard;
  z->color = RED;

  char ret = bst_insert(tree, z);

  if (ret == 1) {
    free(z);
    return ret;
  }

  rbt_restoration(tree, z);

  return ret;
}

void *rbt_find(RBT *tree, void *key) {
  RBT_Node *ret = tree->root;
  char cmp;

  while (ret != tree->guard) {
    cmp = tree->rbtkeycmpfunction(key, ret->data);

    if (cmp < 0) {
      ret = ret->left;
    } else if (cmp > 0) {
      ret = ret->right;
    } else {
      return ret->data;
    }
  }
  /* This line is reached only if the data is not in the RBT */
  return NULL;
}

char rbt_delete(RBT *tree, void *key) {
  if (key == NULL) {
    return 1;
  }

  RBT_Node *to_delete = rbt_find_RET_NODE(tree, key);

  if (to_delete == NULL)
    return 1;

  RBT_Node *y = to_delete;
  RBT_Node *x = tree->guard;
  char original_y_color = y->color;

  if (to_delete->left == tree->guard) {
    x = to_delete->right;
    rbt_transplant(tree, to_delete, to_delete->right);
  } else if (to_delete->right == tree->guard) {
    x = to_delete->left;
    rbt_transplant(tree, to_delete, to_delete->left);
  } else {
    y = bst_min_of_tree(tree, to_delete->right);
    original_y_color = y->color;
    x = y->right;
    if (y->parent == to_delete) {
      x->parent = y;
    } else {
      rbt_transplant(tree, y, y->right);
      y->right = to_delete->right;
      y->right->parent = y;
    }
    rbt_transplant(tree, to_delete, y);
    y->left = to_delete->left;
    y->left->parent = y;
    y->color = to_delete->color;
  }

  if (original_y_color == BLACK) {
    rbt_restore_deletion(tree, x);
  }

  rbt_free_node(tree, to_delete);

  return 0;
}

void rbt_free(RBT *tree) {
  rbt_free_node(tree, tree->root);

  free(tree->guard);
  free(tree);

  return;
}

ConnList *rbt_get_all_nodes(RBT *tree) {
  ConnList *ret = connlist_init(NULL, connlist_del_function, NULL);

  rbt_inorder_traversal(tree->root->left, ret);
  connlist_insert(ret, tree->root->data);
  rbt_inorder_traversal(tree->root->right, ret);

  return ret;
}

/** Not part of the interface **/

/** No need to do anything, the list will be deleted immediately **/
char connlist_del_function(const void *key, const void *data) { return 0; }

void rbt_inorder_traversal(RBT_Node *root, ConnList *list) {
  if (root->data == NULL)
    return;
  rbt_inorder_traversal(root->left, list);
  connlist_insert(list, root->data);
  rbt_inorder_traversal(root->right, list);

  return;
}

void rbt_free_node(RBT *tree, RBT_Node *n) {
  if (n == tree->guard) {
    return;
  }

  rbt_free_node(tree, n->left);
  rbt_free_node(tree, n->right);

  if (tree->rbtfreefunction == NULL) {
    free(n);
    return;
  }

  tree->rbtfreefunction(n->data);
  free(n);

  return;
}

void rbt_left_rotation(RBT *tree, RBT_Node *x) {
  RBT_Node *y = x->right;
  x->right = y->left;

  if (y->left != tree->guard) {
    y->left->parent = x;
  }
  y->parent = x->parent;

  if (x->parent == tree->guard) {
    tree->root = y;
  } else if (x == x->parent->left) {
    x->parent->left = y;
  } else {
    x->parent->right = y;
  }

  y->left = x;
  x->parent = y;

  return;
}

void rbt_right_rotation(RBT *tree, RBT_Node *x) {
  RBT_Node *y = x->left;
  x->left = y->right;

  if (y->right != tree->guard) {
    y->right->parent = x;
  }
  y->parent = x->parent;

  if (x->parent == tree->guard) {
    tree->root = y;
  } else if (x == x->parent->right) {
    x->parent->right = y;
  } else {
    x->parent->left = y;
  }

  y->right = x;
  x->parent = y;

  return;
}

char bst_insert(RBT *tree, RBT_Node *z) {
  RBT_Node *y = tree->guard;
  RBT_Node *x = tree->root;

  char cmp;

  while (x != tree->guard) {
    y = x;
    if ((cmp = tree->rbtnodescmpfunction(z->data, x->data)) < 0)
      x = x->left;
    else if (cmp > 0)
      x = x->right;
    else
      return 1; /** Duplicate keys are not allowed **/
  }

  z->parent = y;
  if (y == tree->guard) { /* or (y == tree.guard) */
    tree->root = z;
  } else if ((cmp = tree->rbtnodescmpfunction(z->data, y->data)) < 0)
    y->left = z;
  else if (cmp > 0)
    y->right = z;
  else
    return 1;

  return 0; /** Successfull **/
}

void rbt_restoration(RBT *tree, RBT_Node *z) {
  RBT_Node *y = tree->guard;

  while (z->parent->color == RED) { /* Red nodes cannot have red children and z
                                       has just been inserted as red */
    if (z->parent ==
        z->parent->parent
            ->left) { /* If z's parent is the left child of its parent */
      y = z->parent->parent->right;

      if (y->color == RED) {
        z->parent->color = BLACK;
        y->color = BLACK;
        z->parent->parent->color = RED;
        z = z->parent->parent;
      } else {
        if (z == z->parent->right) {
          z = z->parent;
          rbt_left_rotation(tree, z);
        }
        z->parent->color = BLACK;
        z->parent->parent->color = RED;
        rbt_right_rotation(tree, z->parent->parent);
      }
    } else { /* If z's parent is the right child of its parent */
      y = z->parent->parent->left;

      if (y->color == RED) {
        z->parent->color = BLACK;
        y->color = BLACK;
        z->parent->parent->color = RED;
        z = z->parent->parent;
      } else {
        if (z == z->parent->left) {
          z = z->parent;
          rbt_right_rotation(tree, z);
        }
        z->parent->color = BLACK;
        z->parent->parent->color = RED;
        rbt_left_rotation(tree, z->parent->parent);
      }
    }
  }

  tree->root->color = BLACK;

  return;
}

void rbt_transplant(RBT *tree, RBT_Node *u, RBT_Node *v) {
  if (u->parent == tree->guard) {
    tree->root = v;
  } else if (u == u->parent->left) {
    u->parent->left = v;
  } else {
    u->parent->right = v;
  }
  v->parent = u->parent;

  return;
}

RBT_Node *bst_min_of_tree(RBT *tree, RBT_Node *x) {
  while (x->left != tree->guard) {
    x = x->left;
  }
  return x;
}

void rbt_restore_deletion(RBT *tree, RBT_Node *x) {
  RBT_Node *w;
  while (x != tree->root && x->color == BLACK) {
    if (x == x->parent->left) {
      w = x->parent->right;
      if (w->color == RED) {
        w->color = BLACK;
        x->parent->color = RED;
        rbt_left_rotation(tree, x->parent);
        w = x->parent->right;
      }

      if (w->left->color == BLACK && w->right->color == BLACK) {
        w->color = RED;
        x = x->parent;
      } else if (w->right->color == BLACK) {
        w->left->color = BLACK;
        w->color = RED;
        rbt_right_rotation(tree, w);
        w = x->parent->right;
      }
      w->color = x->parent->color;
      x->parent->color = BLACK;
      w->right->color = BLACK;
      rbt_left_rotation(tree, x->parent);
      x = tree->root;
    } else {
      w = x->parent->left;
      if (w->color == RED) {
        w->color = BLACK;
        x->parent->color = RED;
        rbt_right_rotation(tree, x->parent);
        w = x->parent->left;
      }

      if (w->right->color == BLACK && w->left->color == BLACK) {
        w->color = RED;
        x = x->parent;
      } else if (w->left->color == BLACK) {
        w->right->color = BLACK;
        w->color = RED;
        rbt_left_rotation(tree, w);
        w = x->parent->left;
      }
      w->color = x->parent->color;
      x->parent->color = BLACK;
      w->left->color = BLACK;
      rbt_right_rotation(tree, x->parent);
      x = tree->root;
    }

    x->color = BLACK;
  }

  return;
}

RBT_Node *rbt_find_RET_NODE(RBT *tree, void *key) {
  RBT_Node *ret = tree->root;
  char cmp;

  while (ret != tree->guard) {
    cmp = tree->rbtkeycmpfunction(key, ret->data);

    if (cmp < 0) {
      ret = ret->left;
    } else if (cmp > 0) {
      ret = ret->right;
    } else {
      return ret;
    }
  }

  /* This line is reached only if the data is not in the RBT */
  return NULL;
}