/***** configParser.h *****/
#ifndef _CONFIGPARSER_H
#define _CONFIGPARSER_H

#include "common.h" /* PARKING BAYS */

#include <limits.h> /* PATH_MAX */

#define EXECUTABLES 3
#define MIN 0
#define MAX 1

/**
 * Values that are going to be used in sleep calls,
 * must be unsigned int and capacity cannot be negative
 */

typedef struct ConfigParser {
  char stationManagerExecutable[PATH_MAX];
  char comptrollerExecutable[PATH_MAX];
  char busExecutable[PATH_MAX];
  unsigned int buses;
  unsigned int busesCreationTimeout;
  unsigned int parkedTime[2];
  unsigned int capacity[2];
  unsigned int manTime[2];
  unsigned int spots[PARKING_BAYS];
  unsigned int statisticsPeriod;
  unsigned int statusPeriod;
} ConfigParser;

int parseConfigurationFile(ConfigParser *parser, char *configFilePath);

#endif