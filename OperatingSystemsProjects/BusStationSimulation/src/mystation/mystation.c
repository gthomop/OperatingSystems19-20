/***** mystation.c *****/
#include "common.h"
#include "configParser.h"
#include "simUtilities.h"

#include <errno.h>
#include <signal.h> /* kill() */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  /* strerror() */
#include <sys/shm.h> /* shared segment */
#include <sys/stat.h>
#include <unistd.h> /* getppid() */

FILE *simLog;
ConfigParser parser;

int main(int argc, char **argv) {
  BusStation *station;
  int shmid;
  int stationRet;
  size_t shmsize;

  if ((simLog = fopen(SIMLOG_PATH, "w")) == NULL) {
    perror("ERROR: simLog: fopen()");
    exit(EXIT_FAILURE);
  }

  if (argc != 3 || argv[1][1] != 'l') {
    SIMLOG("ERROR: Usage: %s -l configFile\n", argv[0]);
    fclose(simLog);
    exit(EXIT_FAILURE);
  }

  if (parseConfigurationFile(&parser, argv[2]) == EXIT_FAILURE) {
    fclose(simLog);
    return EXIT_FAILURE;
  }

  shmsize = sizeof(BusStation) +
            sizeof(char) *
                (parser.spots[VOR] + parser.spots[ASK] + parser.spots[PEL]);

  /* Create shared memory segment */
  if ((shmid = shmget(IPC_PRIVATE, shmsize, 0666)) == -1) {
    SIMLOG("ERROR: shmget(): %s\n", strerror(errno));
    fclose(simLog);
    exit(EXIT_FAILURE);
  }

  /* Attach shared memory segment and cast it to BusStation */
  if ((station = (BusStation *)shmat(shmid, NULL, 0)) == (void *)-1) {
    SIMLOG("ERROR: shmat(): %s\n", strerror(errno));
    shmctl(shmid, IPC_RMID, NULL);
    fclose(simLog);
    exit(EXIT_FAILURE);
  }

  /* Configure the station */
  stationConfig(station);

  /**
   * Avoid detaching the shared segment each time a fork() is called.
   * Will be reattached at the end to destroy semaphores.
   */
  shmdt(station);

  /**
   * Start simulation
   */
  SIMLOG("Starting simulation.\n");
  /**
   * -2 is only returned by child-of-mystation processes
   * that failed to exec, so they must not free the
   * shared memory resources, but only send
   * SIGUSR1 to the station process
   */

  if ((stationRet = startStation(shmid)) == EXEC_FAILED) {
    kill(getppid(), SIGUSR1);
    exit(EXIT_FAILURE);
  } else if (stationRet == SIGNAL_RECEIVED) {
    SIMLOG("ERROR: Terminating due to a signal from a child process.\n");
  } else if (stationRet == OK) {
    SIMLOG("Simulation successful.\n");
  } else {
    SIMLOG("ERROR: logFile: fopen(): %s\n", strerror(errno));
  }

  closeStation(station); /* Destroy mutexes */

  /* Detach and destroy shared segment */
  shmdt(station);
  if (shmctl(shmid, IPC_RMID, NULL) == -1) {
    SIMLOG("ERROR: SIM: shmctl(): %s\n", strerror(errno));
  }

  fclose(simLog);

  exit((stationRet == OK) ? (EXIT_SUCCESS) : (EXIT_FAILURE));
}