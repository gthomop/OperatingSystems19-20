/***** configParser.c *****/
#include "configParser.h"
#include "simUtilities.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int iter;

#define SPECIFIER_IS(STR) !(strcmp(specifier, STR))

int readExecutable(char *executablePath, char *token);
int readUnsignedValue(unsigned int *value, char *token);
int readLongValue(long *value, char *token);

int parseConfigurationFile(ConfigParser *parser, char *configFilePath) {
  FILE *configFile;
  char *line = NULL, *token, *specifier;
  size_t len;
  int ret;

  memset(parser, 0, sizeof(ConfigParser));

  if ((configFile = fopen(configFilePath, "r")) == NULL) {
    SIMLOG("ERROR: Configuration parser: fopen(): %s\n", strerror(errno));
    return EXIT_FAILURE;
  }

  for (iter = 1; iter <= 16; iter++) {
    len = 0;
    if (getline(&line, &len, configFile) == -1) {
      SIMLOG("ERROR: Configuration parser: Line %d: getline(): %s\n", iter,
             strerror(errno));
      free(line);
      fclose(configFile);
      return EXIT_FAILURE;
    }

    if (line[strlen(line) - 1] == '\n')
      line[strlen(line) - 1] = 0;

    token = line;
    specifier = strsep(&token, ">");

    if (SPECIFIER_IS("Station Manager Executable -")) {
      ret = readExecutable(parser->stationManagerExecutable, token);
    } else if (SPECIFIER_IS("Comptroller Executable -")) {
      ret = readExecutable(parser->comptrollerExecutable, token);
    } else if (SPECIFIER_IS("Bus Executable -")) {
      ret = readExecutable(parser->busExecutable, token);
    } else if (SPECIFIER_IS("Number of buses -")) {
      ret = readUnsignedValue(&parser->buses, token);
    } else if (SPECIFIER_IS("Buses creation timeout -")) {
      ret = readUnsignedValue(&parser->busesCreationTimeout, token);
    } else if (SPECIFIER_IS("Min Park Time -")) {
      ret = readUnsignedValue(&parser->parkedTime[MIN], token);
    } else if (SPECIFIER_IS("Max Park Time -")) {
      ret = readUnsignedValue(&parser->parkedTime[MAX], token);
    } else if (SPECIFIER_IS("Min Capacity -")) {
      ret = readUnsignedValue(&parser->capacity[MIN], token);
    } else if (SPECIFIER_IS("Max Capacity -")) {
      ret = readUnsignedValue(&parser->capacity[MAX], token);
    } else if (SPECIFIER_IS("Min Maneuver Time -")) {
      ret = readUnsignedValue(&parser->manTime[MIN], token);
    } else if (SPECIFIER_IS("Max Maneuver Time -")) {
      ret = readUnsignedValue(&parser->manTime[MAX], token);
    } else if (SPECIFIER_IS("VOR parking spots -")) {
      ret = readUnsignedValue(&parser->spots[VOR], token);
    } else if (SPECIFIER_IS("ASK parking spots -")) {
      ret = readUnsignedValue(&parser->spots[ASK], token);
    } else if (SPECIFIER_IS("PEL parking spots -")) {
      ret = readUnsignedValue(&parser->spots[PEL], token);
    } else if (SPECIFIER_IS("Statistics Period -")) {
      ret = readUnsignedValue(&parser->statisticsPeriod, token);
    } else if (SPECIFIER_IS("Status Period -")) {
      ret = readUnsignedValue(&parser->statusPeriod, token);
    } else {
      SIMLOG("ERROR: Configuration Parser: Line %d: Unknown specifier.\n",
             iter);
      ret = EXIT_FAILURE;
    }

    free(line);

    if (ret == EXIT_FAILURE) {
      fclose(configFile);
      return EXIT_FAILURE;
    }
  }

  fclose(configFile);

  if (parser->stationManagerExecutable[0] == 0 ||
      parser->comptrollerExecutable[0] == 0 || parser->busExecutable[0] == 0 ||
      parser->buses == 0 /*|| parser->busesCreationTimeout == 0 \*/
      || parser->parkedTime[MIN] == 0 || parser->parkedTime[MAX] == 0 ||
      parser->capacity[MIN] == 0 || parser->capacity[MAX] == 0 ||
      parser->manTime[MIN] == 0 || parser->manTime[MAX] == 0 ||
      parser->spots[VOR] == 0 || parser->spots[ASK] == 0 ||
      parser->spots[PEL] == 0 || parser->statisticsPeriod == 0 ||
      parser->statusPeriod == 0) {
    SIMLOG("ERROR: Configuration Parser: Wrong configuration file.\n");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int readExecutable(char *executablePath, char *token) {
  if (token[0] == ' ') {
    token = &token[1];
  }

  if (realpath(token, executablePath) == NULL) {
    SIMLOG("ERROR: Configuration Parser: Line %d: realpath(): %s\n", iter,
           strerror(errno));
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int readUnsignedValue(unsigned int *value, char *token) {
  if (token[0] == ' ') {
    token = &token[1];
  }

  long tmp;
  char *end;

  errno = 0;
  tmp = strtol(token, &end, 10);

  if (errno != 0) {
    SIMLOG("ERROR: Configuration Parser: Line %d: strtol(): %s\n", iter,
           strerror(errno));
    return EXIT_FAILURE;
  } else if (end[0] != 0) {
    SIMLOG("ERROR: Configuration Parser: Line %d: Wrong token.\n", iter);
    return EXIT_FAILURE;
  } else if (tmp < 0) {
    SIMLOG("ERROR: Configuration Parser: Line %d: Expected token > 0\n", iter);
    return (EXIT_FAILURE);
  } else if (tmp > UINT_MAX) {
    SIMLOG("ERROR: Configuration Parser: Line %d: Token too large.\n", iter);
    return EXIT_FAILURE;
  }

  *value = (unsigned int)tmp;

  return EXIT_SUCCESS;
}