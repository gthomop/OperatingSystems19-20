/***** voter.c *****/
#include "voter.h"
#include "mystrsep.h"
#include "postalcodes.h"

#include <stdlib.h>
#include <string.h>

Voter *voter_init(char *record, int *post) {
  char *idnum, *name, *surname, *age, *sex, *poststr;
  Voter *ret = malloc(sizeof(Voter));

  idnum = mystrsep(&record, " ");
  ret->id_num = malloc(sizeof(char) * (strlen(idnum) + 1));
  strcpy(ret->id_num, idnum);
  name = mystrsep(&record, " ");
  ret->name = malloc(sizeof(char) * (strlen(name) + 1));
  strcpy(ret->name, name);
  surname = mystrsep(&record, " ");
  ret->surname = malloc(sizeof(char) * (strlen(surname) + 1));
  strcpy(ret->surname, surname);
  age = mystrsep(&record, " ");
  ret->age = (int)strtol(age, NULL, 10);
  sex = mystrsep(&record, " ");
  ret->sex = sex[0];
  poststr = mystrsep(&record, " \n");
  *post = (int)strtol(poststr, NULL, 10);

  ret->hasvoted = NO;

  return ret;
}

void voter_free(Voter *v) {
  free(v->id_num);
  free(v->name);
  free(v->surname);

  free(v);

  return;
}