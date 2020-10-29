/***** comptrollerArguments.h *****/
#ifndef _COMPTROLLERARGUMENTS_H
#define _COMPTROLLERARGUMENTS_H

/**
 * Parse and represent comptroller time arguments
 * as unsigned int because of sleep
 */
typedef struct {
  unsigned int statusPeriod;
  unsigned int statisticsPeriod;
  int shmid;
} ComptrollerArguments;

ComptrollerArguments *comptrollerParse(int argc, char **argv);

#endif
