/***** voterslist.c *****/

#include "voterslist.h"

#include <string.h>

char voterslist_cmp(const void *data1, const void *data2);
char voterslist_del_cmp(const void *key, const void *data);

VotersList *voterslist_init() {
  return (VotersList *)connlist_init(voterslist_cmp, voterslist_del_cmp, NULL);
}

char voterslist_insert(VotersList *voterslist, Voter *v) {
  return connlist_insert(voterslist, v);
}

char voterslist_delete(VotersList *voterslist, Voter *v) {
  return ((connlist_delete(voterslist, v) == NULL) ? 0 : 1);
}

void voterslist_free(VotersList *voterslist) {
  connlist_free(voterslist);
  return;
}
/*************/

char voterslist_cmp(const void *data1, const void *data2) {
  Voter *v1 = (Voter *)data1;
  Voter *v2 = (Voter *)data2;

  int cmp = strcmp(v1->id_num, v2->id_num);

  if (cmp < 0)
    return 1;
  else if (cmp == 0)
    return 0;
  else
    return -1;
}

char voterslist_del_cmp(const void *key, const void *data) {
  char *id = (char *)key;
  Voter *v = (Voter *)data;

  if (!(strcmp(id, v->id_num))) {
    return 0;
  }

  return 1;
}