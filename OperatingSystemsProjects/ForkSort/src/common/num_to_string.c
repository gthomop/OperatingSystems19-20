/***** num_to_string.c *****/
#include "num_to_string.h"

#include <stdio.h>  /** snprintf **/
#include <stdlib.h> /** malloc **/

int digits_of_natural(void *p, char type) {
  if (type == LONG)
    return snprintf(NULL, 0, "%ld", *(long *)p);
  else if (type == SIZE_T)
    return snprintf(NULL, 0, "%lu", *(size_t *)p);
  else
    return snprintf(NULL, 0, "%d", *(int *)p);
}

int digits_of_real(void *p, char type) {
  return snprintf(NULL, 0, (type == DOUBLE) ? ("%.2lf") : ("%.2f"),
                  (type == DOUBLE) ? (*(double *)p) : (*(float *)p));
}

char *natural_to_string(void *p, char type) {
  if (type != LONG && type != INT && type != SIZE_T)
    return NULL;
  char *ret;
  int digits;

  digits = digits_of_natural(p, type);

  ret = malloc(sizeof(char) * (digits + 1));

  if (type == LONG)
    snprintf(ret, digits + 1 /* term byte */, "%ld", *(long *)p);
  else if (type == SIZE_T)
    snprintf(ret, digits + 1, "%lu", *(size_t *)p);
  else
    snprintf(ret, digits + 1, "%d", *(int *)p);

  return ret;
}

char *real_to_string(void *p, char type) {
  if (type != DOUBLE && type != FLOAT)
    return NULL;
  char *ret;
  int digits;

  digits = digits_of_real(p, type);

  ret = malloc(sizeof(char) * (digits + 1));

  snprintf(ret, digits + 1 /* term byte */,
           (type == DOUBLE) ? ("%.2lf") : ("%.2f"),
           (type == DOUBLE) ? (*(double *)p) : (*(float *)p));

  return ret;
}