/***** simHelpers.c *****/
/**
 * Functions and variables in this file
 * are used in simUtilities.c
 */
#include "simHelpers.h"
#include "bus.h" /* LicensePlate */
#include "busStation.h"
#include "common.h" /* return values-messages */
#include "configParser.h"
#include "simUtilities.h" /* Logging */

#include <errno.h>
#include <limits.h>
#include <signal.h> /* SIGUSR1 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <time.h>   /* time_t */
#include <unistd.h> /* execv() */
#include <wait.h>   /* waipitd() */

/* Signal handling variable */
short flag = OK;

/* Shared segment id string */
char *shmidString;

/* Station signal handler */
void sig_h(int signo) {
  if (signo == SIGUSR1) {
    flag = SIGNAL_RECEIVED;
  }
  return;
}

/* License plates linked list functions */

char licensePlateCmp(const void *key, const void *data) {
  LicensePlate *plate1 = (LicensePlate *)data;
  LicensePlate *plate2 = (LicensePlate *)key;

  int cmp = strcmp(plate1->number, plate2->number);

  if (cmp == 0)
    return cmp;
  else if (cmp < 0)
    return -1;
  else
    return 1;
}

void purgeLicensePlate(void *data) {
  free(data);
  return;
}
/* Functions for exec */

/* Snprintf with NULL buffer and 0 size returns the number of bytes it would
 * have written (minus the null byte) */
int digitsOfNum(unsigned int num) { return snprintf(NULL, 0, "%u", num); }

/* Converts the shmid (and every int) to a string */
char *shmidStringify(int shmid) {
  char *ret;
  int digits;

  digits = digitsOfNum((unsigned int)shmid); /* Surely not negative */

  ret = malloc(sizeof(char) * (digits + 1));

  snprintf(ret, digits + 1, "%d", shmid);

  return ret;
}

void generateLicensePlate(char licensePlate[8]) {
  for (int ch = 0; ch < 3; ch++) {
    licensePlate[ch] = rand() % ('Z' - 'A') + 'A';
  }

  licensePlate[3] = '-';

  for (int ch = 4; ch < 7; ch++) {
    licensePlate[ch] = rand() % ('9' - '0') + '0';
  }
  licensePlate[7] = 0;

  return;
}

/* Exec functions */

/* Creates a new station manager process passing the shmid string as an argument
 */
pid_t stationManagerExec() {
  pid_t ret = fork();

  switch (ret) {
  case -1:
    SIMLOG("ERROR: stationManager: fork(): %s\n", strerror(errno));
    return FORK_FAILED;
  case 0: /* New process */
    fclose(simLog);

    char **execVector = malloc(sizeof(char *) * 4);
    execVector[0] = malloc(sizeof(char) * (strlen("stationManager") + 1));
    execVector[1] = malloc(sizeof(char) * (strlen("-s") + 1));
    execVector[2] = shmidString;
    execVector[3] = NULL;

    strcpy(execVector[0], "stationManager");
    strcpy(execVector[1], "-s");
    execv(parser.stationManagerExecutable, execVector);

    /* Must write to log file that the exec call was unsuccessful */
    int shmid;
    BusStation *station;
    FILE *logFile;

    shmid = (int)strtol(shmidString, NULL, 10);
    if ((station = shmat(shmid, NULL, 0)) != (void *)-1) {
      if ((logFile = fopen(LOG_FILE_PATH, "a")) != NULL) {
        LOG("ERROR: stationManager: execv(): %s\n", strerror(errno));
        fclose(logFile);
      }
      shmdt(station);
    }

    for (int i = 0; i < 2; i++) {
      free(execVector[i]);
    }

    free(execVector);

    return EXEC_FAILED; /* execv() failed */
  default:
    return ret; /* child's pid */
  }
}

/* Creates a new comptroller process passing the time arguments as defined in
 * the configuration file */
pid_t comptrollerExec() {
  pid_t ret = fork();

  switch (ret) {
  case -1:
    fprintf(simLog, "ERROR: fork(): %s\n", strerror(errno));
    return FORK_FAILED;
  case 0: /* New process */
    fclose(simLog);

    char **execVector = malloc(sizeof(char *) * 8);
    execVector[0] = malloc(sizeof(char) * (strlen("comptroller") + 1));
    execVector[1] = malloc(sizeof(char) * (strlen("-t") + 1));
    execVector[2] =
        malloc(sizeof(char) * (digitsOfNum(parser.statisticsPeriod) + 1));
    execVector[3] = malloc(sizeof(char) * (strlen("-d") + 1));
    execVector[4] =
        malloc(sizeof(char) * (digitsOfNum(parser.statusPeriod) + 1));
    execVector[5] = malloc(sizeof(char) * (strlen("-s") + 1));
    execVector[6] = shmidString;
    execVector[7] = NULL;

    strcpy(execVector[0], "comptroller");
    strcpy(execVector[1], "-t");
    snprintf(execVector[2], digitsOfNum(parser.statisticsPeriod) + 1, "%u",
             parser.statisticsPeriod);
    strcpy(execVector[3], "-d");
    snprintf(execVector[4], digitsOfNum(parser.statusPeriod) + 1, "%u",
             parser.statusPeriod);
    strcpy(execVector[5], "-s");

    execv(parser.comptrollerExecutable, execVector);

    /* Must write to log file that the exec call was unsuccessful */
    int shmid;
    BusStation *station;
    FILE *logFile;

    shmid = (int)strtol(shmidString, NULL, 10);
    if ((station = shmat(shmid, NULL, 0)) != (void *)-1) {
      if ((logFile = fopen(LOG_FILE_PATH, "a")) != NULL) {
        LOG("ERROR: comptroller: execv(): %s\n", strerror(errno));
        fclose(logFile);
      }
      shmdt(station);
    }

    for (int i = 0; i < 6; i++) {
      free(execVector[i]);
    }

    free(execVector);

    return EXEC_FAILED; /* execv() failed */
  default:
    return ret; /* Child's pid */
  }
}

/** Creates a new bus process passing arguments either parsed from the
 * configuration file\ or randomly generated (inside limits defined in the
 * confuration file)
 */
pid_t busExec(pid_t *busPids, LinkedList *licensePlatesList) {
  LicensePlate *licensePlate;
  pid_t ret;

  licensePlate = malloc(sizeof(LicensePlate));
  generateLicensePlate(licensePlate->number);
  while (linkedListInsert(licensePlatesList, (void *)licensePlate) !=
         EXIT_SUCCESS) {
    free(licensePlate);
    licensePlate = malloc(sizeof(LicensePlate));
    generateLicensePlate(licensePlate->number);
  }

  ret = fork();

  switch (ret) {
  case -1:
    fprintf(simLog, "ERROR: fork(): %s\n", strerror(errno));
    return FORK_FAILED;
  case 0: /* New process */
    fclose(simLog);

    free(busPids);
    TripType typeN;
    unsigned int incpassengers, capacity;
    unsigned int parkperiod, mantime;

    typeN = rand() % 3;

    capacity = rand() % (parser.capacity[MAX] - parser.capacity[MIN]) +
               parser.capacity[MIN];
    incpassengers = rand() % capacity;

    parkperiod = rand() % (parser.parkedTime[MAX] - parser.parkedTime[MIN]) +
                 parser.parkedTime[MIN];
    mantime = rand() % (parser.manTime[MAX] - parser.manTime[MIN]) +
              parser.manTime[MIN];

    char **execVector = malloc(sizeof(char *) * 16);
    execVector[0] = malloc(sizeof(char) * (strlen("bus") + 1));
    execVector[1] = malloc(sizeof(char) * (strlen("-t") + 1));
    execVector[2] = malloc(sizeof(char) * (strlen(TYPESTRING(typeN)) + 1));
    execVector[3] = malloc(sizeof(char) * (strlen("-n") + 1));
    execVector[4] = malloc(sizeof(char) * (digitsOfNum(incpassengers) + 1));
    execVector[5] = malloc(sizeof(char) * (strlen("-c") + 1));
    execVector[6] = malloc(sizeof(char) * (digitsOfNum(capacity) + 1));
    execVector[7] = malloc(sizeof(char) * (strlen("-p") + 1));
    execVector[8] = malloc(sizeof(char) * (digitsOfNum(parkperiod) + 1));
    execVector[9] = malloc(sizeof(char) * (strlen("-m") + 1));
    execVector[10] = malloc(sizeof(char) * (digitsOfNum(mantime) + 1));
    execVector[11] = malloc(sizeof(char) * (strlen("-l") + 1));
    execVector[12] = malloc(sizeof(char) * (strlen(licensePlate->number) + 1));
    execVector[13] = malloc(sizeof(char) * (strlen("-s") + 1));
    execVector[14] = shmidString;
    execVector[15] = NULL;

    strcpy(execVector[0], "bus");
    strcpy(execVector[1], "-t");
    strcpy(execVector[2], TYPESTRING(typeN));
    strcpy(execVector[3], "-n");
    snprintf(execVector[4], digitsOfNum(incpassengers) + 1, "%u",
             incpassengers);
    strcpy(execVector[5], "-c");
    snprintf(execVector[6], digitsOfNum(capacity) + 1, "%u", capacity);
    strcpy(execVector[7], "-p");
    snprintf(execVector[8], digitsOfNum(parkperiod) + 1, "%u", parkperiod);
    strcpy(execVector[9], "-m");
    snprintf(execVector[10], digitsOfNum(mantime) + 1, "%u", mantime);
    strcpy(execVector[11], "-l");
    strcpy(execVector[12], licensePlate->number);
    strcpy(execVector[13], "-s");

    linkedListPurge_f(licensePlatesList);

    execv(parser.busExecutable, execVector);

    /* Must write to log file that the exec call was unsuccessful */
    int shmid;
    BusStation *station;
    FILE *logFile;

    shmid = (int)strtol(shmidString, NULL, 10);
    if ((station = shmat(shmid, NULL, 0)) != (void *)-1) {
      if ((logFile = fopen(LOG_FILE_PATH, "a")) != NULL) {
        LOG("ERROR: bus: execv(): %s\n", strerror(errno));
        fclose(logFile);
      }
      shmdt(station);
    }

    for (int i = 0; i < 14; i++) {
      free(execVector[i]);
    }

    free(execVector);

    return EXEC_FAILED; /* execv() failed */
  default:
    return ret; /* Child's pid */
  }
}
