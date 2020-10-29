/***** busArguments.c *****/
#include "busArguments.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum FlagType {
  INCPASSENGERS,
  CAPACITY,
  PARKPERIOD,
  MANTIME,
  SHMID,
  TYPE,
  LICENSE
} FlagType;

#define FLAGSTRING(FLAG)                                                       \
  (FLAG == INCPASSENGERS)                                                      \
      ? ("-n")                                                                 \
      : ((FLAG == CAPACITY)                                                    \
             ? ("-c")                                                          \
             : ((FLAG == PARKPERIOD)                                           \
                    ? ("-p")                                                   \
                    : ((FLAG == MANTIME)                                       \
                           ? ("-m")                                            \
                           : ((FLAG == SHMID)                                  \
                                  ? ("-s")                                     \
                                  : ((FLAG == TYPE) ? ("-t") : (("-l")))))))

int readUnsigned(unsigned int *value, char *token, FlagType flag) {
  long tmp;
  char *end;

  errno = 0;
  tmp = strtol(token, &end, 10);

  if (errno != 0) {
    fprintf(stderr, "ERROR: BUS: Argument after flag %s: strtol(): %s\n",
            FLAGSTRING(flag), strerror(errno));
    return EXIT_FAILURE;
  } else if (end[0] != 0) {
    fprintf(stderr, "ERROR: BUS: Wrong argument after flag %s.\n",
            FLAGSTRING(flag));
    return EXIT_FAILURE;
  } else if (tmp < 0) {
    fprintf(stderr, "ERROR: BUS: Negative value after flag %s.\n",
            FLAGSTRING(flag));
    return EXIT_FAILURE;
  } else if (tmp > UINT_MAX) {
    fprintf(stderr, "ERROR: BUS: Too large value after flag %s.\n",
            FLAGSTRING(flag));
    return EXIT_FAILURE;
  }

  *value = (unsigned int)tmp;

  return EXIT_SUCCESS;
}

void flagTwice(short flag) {
  fprintf(stderr, "ERROR: BUS: Flag %s found twice.\n", FLAGSTRING(flag));
  return;
}

BusArguments *errorUsage(BusArguments *ret) {
  free(ret);

  return NULL;
}

BusArguments *busParse(int argc, char **argv) {
  if (argc != 15) {
    fprintf(stderr, "ERROR: BUS: Wrong number of arguments.\n");
    fprintf(stderr,
            "BUS: Usage: %s -l licensePlate -t type -n incpassengers -c "
            "capacity -p parkperiod -m mantime -s shmid\n",
            argv[0]);
    return NULL;
  }

  int argn;
  char *end, *type;
  BusArguments *ret;
  long tmp;

  ret = malloc(sizeof(BusArguments));

  memset(&ret->b, 0, sizeof(Bus));
  ret->b.type = UNSET;
  ret->shmid = -1;

  for (argn = 1; argn < argc; argn += 2) {
    switch (argv[argn][1]) { /* skip the dash */
    case 't':
      if (ret->b.type != UNSET) {
        flagTwice(TYPE);
        return errorUsage(ret);
      }
      type = argv[argn + 1];
      if (!(strcmp(type, "ASK"))) {
        ret->b.type = ASK;
      } else if (!(strcmp(type, "PEL"))) {
        ret->b.type = PEL;
      } else if (!(strcmp(type, "VOR"))) {
        ret->b.type = VOR;
      } else {
        return errorUsage(ret);
      }
      break;
    case 'n':
      if (ret->b.passengersToDepark != 0) {
        flagTwice(INCPASSENGERS);
        return errorUsage(ret);
      }
      if (readUnsigned(&ret->b.passengersToDepark, argv[argn + 1],
                       INCPASSENGERS) == EXIT_FAILURE) {
        return errorUsage(ret);
      }
      break;
    case 'c':
      if (ret->b.capacity != 0) {
        flagTwice(CAPACITY);
        return errorUsage(ret);
      }
      if (readUnsigned(&ret->b.capacity, argv[argn + 1], CAPACITY) ==
          EXIT_FAILURE) {
        return errorUsage(ret);
      } else if (ret->b.capacity == 0) {
        fprintf(stderr, "ERROR: BUS: Capacity cannot be 0.\n");
        return errorUsage(ret);
      }
      break;
    case 'p':
      if (ret->b.parkPeriod != 0) {
        flagTwice(PARKPERIOD);
        return errorUsage(ret);
      }
      if (readUnsigned(&ret->b.parkPeriod, argv[argn + 1], PARKPERIOD) ==
          EXIT_FAILURE) {
        return errorUsage(ret);
      } else if (ret->b.parkPeriod == 0) {
        fprintf(stderr, "ERROR: BUS: Park Period cannot be 0.\n");
      }
      break;
    case 'm':
      if (ret->b.manTime != 0) {
        flagTwice(MANTIME);
        return errorUsage(ret);
      }
      if (readUnsigned(&ret->b.manTime, argv[argn + 1], MANTIME) ==
          EXIT_FAILURE) {
        return errorUsage(ret);
      } else if (ret->b.manTime == 0) {
        fprintf(stderr, "ERROR: BUS: Maneuver Time cannot be 0.\n");
      }
      break;
    case 's':
      if (ret->shmid != -1) {
        flagTwice(SHMID);
        return errorUsage(ret);
      }
      errno = 0;
      tmp = strtol(argv[argn + 1], &end, 10);
      if (errno != 0) {
        fprintf(stderr, "ERROR: BUS: Argument after flag -s: strtol(): %s\n",
                strerror(errno));
        return errorUsage(ret);
      } else if (end[0] != 0) {
        fprintf(stderr, "ERROR: BUS: Wrong argument after flag -s.\n");
        return errorUsage(ret);
      } else if (tmp > INT_MAX) {
        fprintf(stderr, "ERROR: BUS: Shmid very large.\n");
        return errorUsage(ret);
      } else if (tmp <= 0) {
        fprintf(stderr, "ERROR: BUS: shmid cannot be <= 0.\n");
        return errorUsage(ret);
      }
      if (end[0] != 0 || tmp > INT_MAX) {
        return errorUsage(ret);
      }
      ret->shmid = (int)tmp;
      break;
    case 'l':
      if (ret->b.licensePlate.number[0] != 0) {
        flagTwice(LICENSE);
        return errorUsage(ret);
      }
      if (strlen(argv[argn + 1]) * sizeof(char) >=
          sizeof(ret->b.licensePlate.number)) {
        return errorUsage(ret);
      }
      strcpy(ret->b.licensePlate.number, argv[argn + 1]);
      break;
    default:
      fprintf(stderr, "ERROR: BUS: Unknown flag %s\n", argv[argn]);
      fprintf(stderr,
              "BUS: Usage: %s -l licensePlate -t type -n incpassengers -c "
              "capacity -p parkperiod -m mantime -s shmid\n",
              argv[0]);
      return errorUsage(ret);
    }
  }

  return ret;
}