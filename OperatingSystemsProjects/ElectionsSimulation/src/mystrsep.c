/***** mystrsep.c *****/
#include <stdio.h>
#include <string.h>

char *mystrsep(char **record, const char *delim) {
  char *ret = NULL;
  while ((ret = strsep(record, delim))[0] == 0) {
    ;
  }

  return ret;
}