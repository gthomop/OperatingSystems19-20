/***** postalcodes.c *****/

#include "postalcodes.h"
#include "rbt.h"

#include <stdlib.h>

char postalcodes_nodes_cmp(const void *node1, const void *node2);
char postalcodes_key_cmp(const void *key, const void *node);
void postalcodes_freefunction(void *data);

PostalCodesRBT *postalcodes_init() {
  return (PostalCodesRBT *)rbt_init(postalcodes_nodes_cmp, postalcodes_key_cmp,
                                    postalcodes_freefunction);
}

char postalcodes_insert(PostalCodesRBT *postalCodes, int code, Voter *v) {
  PostalCode *found = (PostalCode *)rbt_find(postalCodes, (void *)&code);

  if (found == NULL) {
    found = malloc(sizeof(PostalCode));
    found->code = code;
    found->havevoted = 0;
    found->voters_number = 0;
    rbt_insert(postalCodes, (void *)found);

    found->voters = voterslist_init();
  }

  v->post = found;
  if (voterslist_insert(found->voters, v)) {
    return 1;
  }

  found->voters_number++;

  return 0;
}

char postalcodes_remove_voter(PostalCodesRBT *postalCodes, Voter *v) {
  char ret = voterslist_delete(v->post->voters, v);

  if (!ret) {
    if (v->hasvoted)
      v->post->havevoted--;
    v->post->voters_number--;

    if (v->post->voters_number == 0) {
      postalcodes_delete(postalCodes, v->post->code);
    }
  }

  return ret;
}

char postalcodes_delete(PostalCodesRBT *postalCodes, int code) {
  return rbt_delete(postalCodes, (void *)&code);
}

int postalcodes_get_votes(PostalCodesRBT *postalCodes, int code) {
  PostalCode *pc = rbt_find(postalCodes, &code);

  if (pc == NULL) {
    return -1;
  }

  return pc->havevoted;
}

void postalcodes_free(PostalCodesRBT *postalCodes) {
  rbt_free(postalCodes);
  return;
}

/************/

char postalcodes_nodes_cmp(const void *node1, const void *node2) {
  PostalCode *pc1 = (PostalCode *)node1;
  PostalCode *pc2 = (PostalCode *)node2;

  if (pc1->code < pc2->code)
    return 1;
  else if (pc1->code == pc2->code)
    return 0;
  else
    return -1;
}

char postalcodes_key_cmp(const void *key, const void *node) {
  PostalCode *pc = (PostalCode *)node;
  int *code = (int *)key;

  if (*code < pc->code)
    return 1;
  else if (*code == pc->code)
    return 0;
  else
    return -1;
}

void postalcodes_freefunction(void *data) {
  PostalCode *pc = (PostalCode *)data;

  connlist_free(pc->voters);

  free(pc);

  return;
}