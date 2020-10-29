/***** common.h *****/
#ifndef _COMMON_H
#define _COMMON_H

#include <stdio.h> /* FILE */

typedef enum { VOR, ASK, PEL, UNSET } TripType;

extern FILE *logFile;
extern short flag; /* signal handling */

#define PARKING_BAYS 3

/* Return values-messages */

#define FORK_FAILED -1
#define EXEC_FAILED -2
#define OK 0
#define SIGNAL_RECEIVED 1
#define FOPEN_ERROR 2

#define LOG_FILE_PATH "./log.file"

#define TYPESTRING(type)                                                       \
  ((type == ASK) ? ("ASK") : ((type == VOR) ? ("VOR") : ("PEL")))
#define LOG(...)                                                               \
  {                                                                            \
    if (!sem_wait(&station->logWrite)) {                                       \
      fprintf(logFile, ##__VA_ARGS__);                                         \
      fflush(logFile);                                                         \
      sem_post(&station->logWrite);                                            \
    }                                                                          \
  }

#endif