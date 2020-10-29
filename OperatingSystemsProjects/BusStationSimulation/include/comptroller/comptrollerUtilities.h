/***** comptrollerUtilities.h *****/
#ifndef _COMPTROLLERUTILITIES_H
#define _COMPTROLLERUTILITIES_H

#include "busStation.h"

#include <stdio.h>

extern short flag;
extern FILE *logFile;

void sig_h(int signo);
void printStatistics(BusStation *station);
void printStatus(BusStation *station);

#endif