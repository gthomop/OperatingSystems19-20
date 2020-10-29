/***** votersrbt.c *****/
#include "votersrbt.h"

#include <stdlib.h>
#include <string.h>

char votersrbt_nodescmpfunction(const void *data1, const void *data2);
char votersrbt_keycmpfunction(const void *key, const void *data);
void votersrbt_freefunction(void *data);

VotersRBT *votersrbt_init() {
  return (VotersRBT *)rbt_init(votersrbt_nodescmpfunction,
                               votersrbt_keycmpfunction,
                               votersrbt_freefunction);
}

char votersrbt_insert(VotersRBT *tree, Voter *v) { return rbt_insert(tree, v); }

Voter *votersrbt_find(VotersRBT *tree, char *idnum) {
  return (Voter *)rbt_find(tree, idnum);
}

char votersrbt_delete(VotersRBT *tree, char *idnum) {
  return rbt_delete(tree, idnum);
}

void votersrbt_free(VotersRBT *tree) {
  rbt_free(tree);
  return;
}

/************/

char votersrbt_nodescmpfunction(const void *data1, const void *data2) {
  Voter *v1 = (Voter *)data1;
  Voter *v2 = (Voter *)data2;

  int cmp = strcmp(v1->id_num, v2->id_num);

  if (cmp < 0)
    return -1;
  else if (cmp == 0)
    return 0;
  else
    return 1;
}

char votersrbt_keycmpfunction(const void *key, const void *data) {
  char *keystr = (char *)key;
  Voter *v = (Voter *)data;

  int cmp = strcmp(keystr, v->id_num);

  if (cmp < 0)
    return -1;
  else if (cmp == 0)
    return 0;
  else
    return 1;
}

void votersrbt_freefunction(void *data) {

  voter_free((Voter *)data);

  return;
}