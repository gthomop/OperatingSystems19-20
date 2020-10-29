/***** bus.c *****/
#include "busArguments.h"
#include "busStation.h"
#include "busUtilities.h"
#include "common.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <time.h>
#include <unistd.h>

#define CHECK_FLAG(flag)                                                       \
  if (flag == SIGNAL_RECEIVED) {                                               \
    LOG("BUS %s: Terminating. (SIGUSR1)\n", thisBus.licensePlate.number);      \
    kill(getppid(), SIGUSR1);                                                  \
    fclose(logFile);                                                           \
    shmdt(station);                                                            \
    exit(EXIT_SUCCESS);                                                        \
  }

short flag = OK;
FILE *logFile;

int main(int argc, char **argv) {
  BusArguments *args;
  Bus thisBus;
  int shmid;
  BusStation *station;
  struct sigaction sigact;
  time_t timeArrived, timeApproval;
  time_t timePickedUp;

  memset(&sigact, 0, sizeof(struct sigaction));
  sigact.sa_handler = sig_h;
  sigaction(SIGUSR1, &sigact, NULL);

  if ((args = busParse(argc, argv)) == NULL) {
    exit(EXIT_FAILURE);
  }

  memcpy(&thisBus, &args->b, sizeof(Bus));
  shmid = args->shmid;
  free(args);

  if ((logFile = fopen(LOG_FILE_PATH, "a")) == NULL) {
    fprintf(stderr, "ERROR: BUS %s: fopen(): %s\n", thisBus.licensePlate.number,
            strerror(errno));
    kill(getppid(), SIGUSR1);
    exit(EXIT_FAILURE);
  }

  if ((station = (BusStation *)shmat(shmid, NULL, 0)) == (void *)-1) {
    LOG("ERROR: BUS %s: shmat(): %s\n", thisBus.licensePlate.number,
        strerror(errno));
    kill(getppid(), SIGUSR1);
    fclose(logFile);
    exit(EXIT_FAILURE);
  }

  LOG("BUS %s: Started.\n", thisBus.licensePlate.number);
  timeArrived = time(NULL);

  CHECK_FLAG(flag);
  arrive(station, &thisBus);
  CHECK_FLAG(flag);

  updateStatsArrival(station, &thisBus, thisBus.inStation - timeArrived);

  /* Stay parked for parkPeriod seconds */
  stayParked(&thisBus);
  CHECK_FLAG(flag);

  /**
   * TODO: Change seed
   */
  pickUpPassengers(&thisBus);

  timePickedUp = time(NULL);

  leave(station, &thisBus, &timeApproval);
  CHECK_FLAG(flag);

  updateStatsLeave(station, &thisBus, timeApproval - timePickedUp);

  LOG("BUS %s: Terminating.\n", thisBus.licensePlate.number);
  shmdt(station);
  fclose(logFile);

  exit(EXIT_SUCCESS);
}
