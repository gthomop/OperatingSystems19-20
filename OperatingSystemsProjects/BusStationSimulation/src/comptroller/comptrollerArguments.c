/***** comptrollerArguments.c *****/
#include "comptrollerArguments.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum FlagType { TIME, STATTIME, SHMID } FlagType;

#define FLAGSTRING(FLAG)                                                       \
  (FLAG == TIME) ? ("-d") : ((FLAG == STATTIME) ? ("-t") : ("-s"))

int readUnsigned(unsigned int *value, char *token, FlagType flag) {
  long tmp;
  char *end;

  errno = 0;
  tmp = strtol(token, &end, 10);

  if (errno != 0) {
    fprintf(stderr, "ERROR: CT: Argument after flag %s: strtol(): %s\n",
            FLAGSTRING(flag), strerror(errno));
    return EXIT_FAILURE;
  } else if (end[0] != 0) {
    fprintf(stderr, "ERROR: CT: Wrong argument after flag %s.\n",
            FLAGSTRING(flag));
    return EXIT_FAILURE;
  } else if (tmp < 0) {
    fprintf(stderr, "ERROR: CT: Negative value after flag %s.\n",
            FLAGSTRING(flag));
    return EXIT_FAILURE;
  } else if (tmp > UINT_MAX) {
    fprintf(stderr, "ERROR: CT: Too large value after flag %s.\n",
            FLAGSTRING(flag));
    return EXIT_FAILURE;
  }

  *value = (unsigned int)tmp;

  return EXIT_SUCCESS;
}

void flagTwice(FlagType flag) {
  fprintf(stderr, "ERROR: CT: Flag %s found twice.\n", FLAGSTRING(flag));
  return;
}

ComptrollerArguments *errorUsage(ComptrollerArguments *ret) {
  free(ret);

  return NULL;
}

ComptrollerArguments *comptrollerParse(int argc, char **argv) {
  if (argc != 7) {
    fprintf(stderr, "ERROR: CT: Wrong number of arguments.\n");
    fprintf(stderr, "CT: Usage: %s -d time -t stattimes -s shmid\n", argv[0]);
    return NULL;
  }

  ComptrollerArguments *ret;
  int argn;
  char *end;
  long tmp;

  ret = malloc(sizeof(ComptrollerArguments));
  memset(ret, 0, sizeof(ComptrollerArguments));

  for (argn = 1; argn < argc; argn += 2) {
    switch (argv[argn][1]) {
    case 'd':
      if (ret->statusPeriod != 0) {
        flagTwice(TIME);
        return errorUsage(ret);
      }
      if (readUnsigned(&ret->statusPeriod, argv[argn + 1], TIME)) {
        return errorUsage(ret);
      }
      if (ret->statusPeriod == 0) {
        fprintf(stderr, "ERROR: CT: Time cannot be 0.\n");
        return errorUsage(ret);
      }
      break;
    case 't':
      if (ret->statisticsPeriod != 0) {
        flagTwice(STATTIME);
        return errorUsage(ret);
      } else if (readUnsigned(&ret->statisticsPeriod, argv[argn + 1],
                              STATTIME)) {
        return errorUsage(ret);
      } else if (ret->statisticsPeriod == 0) {
        fprintf(stderr, "ERROR: CT: Stattimes cannot be 0.\n");
        return errorUsage(ret);
      }
      break;
    case 's':
      if (ret->shmid != 0) {
        flagTwice(SHMID);
        return errorUsage(ret);
      }
      errno = 0;
      tmp = strtol(argv[argn + 1], &end, 10);
      if (errno != 0) {
        fprintf(stderr, "ERROR: CT: Arguments after flag %s: strtol(): %s\n",
                FLAGSTRING(SHMID), strerror(errno));
        return errorUsage(ret);
      } else if (end[0] != 0) {
        fprintf(stderr, "ERROR: CT: Wrong argument after flag %s.\n",
                FLAGSTRING(SHMID));
        return errorUsage(ret);
      } else if (tmp > INT_MAX) {
        fprintf(stderr, "ERROR: CT: Too large value after flag %s.\n",
                FLAGSTRING(SHMID));
        return errorUsage(ret);
      } else if (tmp <= 0) {
        fprintf(stderr, "ERROR: Shmid cannot be <= 0.\n");
        return errorUsage(ret);
      }
      ret->shmid = (int)tmp;
      break;
    default:
      fprintf(stderr, "ERROR: CT: Unknow flag %s.\n", argv[argn]);
      fprintf(stderr, "CT: Usage: %s -d time -t stattimes -s shmid\n", argv[0]);
      return errorUsage(ret);
    }
  }

  return ret;
}