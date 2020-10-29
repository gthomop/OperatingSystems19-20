/***** run.h *****/
#ifndef _ARGUMENTS_H
#define _ARGUMENTS_H

#include "record.h" /* for macros below */
#define MIN_COLUMN CUSTID
#define MAX_COLUMN AMOUNT

typedef struct Coach {
  char type;
  int columnid;
} Coach;

// auxiliary struct for parsing program arguments
typedef struct Arguments {
  char *filename;
  Coach *coaches;
  int numofcoaches;
} Arguments;

Arguments *Arguments_parser(int argc, char **argv);
// free memory allocated for arguments' strings
void Arguments_free(Arguments *arguments);

#endif