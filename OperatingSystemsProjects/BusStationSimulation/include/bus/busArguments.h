/***** busArguments.h *****/
#ifndef _BUSARGUMENTS_H
#define _BUSARGUMENTS_H

#include "bus.h"

typedef struct {
  Bus b;
  int shmid;
} BusArguments;

BusArguments *busParse(int argc, char **argv);

#endif