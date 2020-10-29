/***** simUtilities.h *****/
#ifndef _SIMUTILITIES_H
#define _SIMUTILITIES_H

#include "busStation.h"
#include "configParser.h"

#include <stdio.h> /* FILE */

extern FILE *simLog;
extern ConfigParser parser;
#define SIMLOG(...)                                                            \
  {                                                                            \
    fprintf(simLog, ##__VA_ARGS__);                                            \
    fflush(simLog);                                                            \
  }

#define SIMLOG_PATH "./simLog.file"

void stationConfig(BusStation *station);
void closeStation(BusStation *station);
int startStation(int shmid);

#endif