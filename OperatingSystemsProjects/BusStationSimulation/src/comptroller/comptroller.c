/***** comptroller.c *****/

#include "busStation.h"
#include "common.h"
#include "comptrollerArguments.h"
#include "comptrollerUtilities.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <unistd.h>

short flag = OK;
FILE *logFile;

void (*fun1)(BusStation *);
void (*fun2)(BusStation *);

int main(int argc, char **argv) {
  ComptrollerArguments *args;
  struct sigaction sigactRestart, sigactNoRestart;
  int shmid;
  unsigned int firstSleep, secondSleep;
  void (*fun1)(BusStation *);
  void (*fun2)(BusStation *);
  BusStation *station;

  /* Initialize sigaction */
  memset(&sigactRestart, 0, sizeof(struct sigaction));
  sigactRestart.sa_handler = sig_h;

  memset(&sigactNoRestart, 0, sizeof(struct sigaction));
  sigactNoRestart.sa_handler = sig_h;
  sigactNoRestart.sa_flags = SA_RESTART;

  sigaction(SIGUSR1, &sigactRestart, NULL);

  /* Open logfile */
  if ((logFile = fopen(LOG_FILE_PATH, "a")) == NULL) {
    perror("ERROR: CT: Log file fopen()");
    kill(getppid(), SIGUSR1);
    exit(EXIT_FAILURE);
  }

  /* Parse arguments */
  if ((args = comptrollerParse(argc, argv)) == NULL) {
    kill(getppid(), SIGUSR1);
    fclose(logFile);
    exit(EXIT_FAILURE);
  }

  if (flag == SIGNAL_RECEIVED) {
    fclose(logFile);
    free(args);
    exit(EXIT_SUCCESS);
  }

  /* Take arguments from struct */
  shmid = args->shmid;
  if (args->statisticsPeriod < args->statusPeriod) {
    firstSleep = args->statisticsPeriod;
    fun1 = printStatistics;
    secondSleep = args->statusPeriod - args->statisticsPeriod;
    fun2 = printStatus;
  } else {
    firstSleep = args->statusPeriod;
    fun1 = printStatus;
    secondSleep = args->statisticsPeriod - args->statusPeriod;
    fun2 = printStatistics;
  }
  free(args); /* No further need */

  if ((station = (BusStation *)shmat(shmid, NULL, 0)) == (void *)-1) {
    perror("ERROR: CT: shmat()");
    kill(getppid(), SIGUSR1);
    fclose(logFile);
    exit(EXIT_SUCCESS);
  }

  /****** Now can write to logFile safely *******/

  LOG("CT: Started.\n");

  /* Need sem_wait() calls to NOT restart when a signal is received */
  sigaction(SIGUSR1, &sigactNoRestart, NULL);

  while (flag == OK) {
    sleep(firstSleep);

    if (flag == SIGNAL_RECEIVED) {
      break;
    }

    LOG("CT: First print.\n");
    fun1(station);

    sleep(secondSleep);

    if (flag == SIGNAL_RECEIVED) {
      break;
    }
    LOG("CT: Second print.\n");
    fun2(station);
  }

  LOG("CT: Terminating. (SIGUSR1)\n");
  shmdt(station);
  fclose(logFile);

  exit(EXIT_SUCCESS);
}
