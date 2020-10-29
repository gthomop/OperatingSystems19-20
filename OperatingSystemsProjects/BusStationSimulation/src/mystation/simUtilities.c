/***** simUtilities.c *****/

/**
 * The functions in this file are directly called
 * in mystation.c
 */
#include "simUtilities.h"
#include "bus.h" /* Bus struct */
#include "busStation.h"
#include "configParser.h"
#include "linkedList.h" /* Linked list */
#include "simHelpers.h" /* Variables+functions for the functions below */

#include <errno.h>
#include <fcntl.h> /* open() */
#include <semaphore.h>
#include <signal.h> /* sigaction(), kill() */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h> /* getcwd() */
#include <wait.h>

/** Parses the configuration file and sets up the simulation.
 * It edits values in the shared segment too.
 */
void stationConfig(BusStation *station) {
  /**
   * TODO: Fix offsets
   */
  station->bay[VOR].spotsNumber = parser.spots[VOR];
  station->bay[VOR].available = station->bay[VOR].spotsNumber;
  station->bay[ASK].spotsNumber = parser.spots[ASK];
  station->bay[ASK].available = station->bay[ASK].spotsNumber;
  station->bay[PEL].spotsNumber = parser.spots[PEL];
  station->bay[PEL].available = station->bay[PEL].spotsNumber;

  station->bay[VOR].spotsOffset = sizeof(BusStation);
  station->bay[ASK].spotsOffset =
      station->bay[VOR].spotsOffset + station->bay[VOR].spotsNumber;
  station->bay[PEL].spotsOffset =
      station->bay[ASK].spotsOffset + station->bay[ASK].spotsNumber;

  sem_init(&station->logWrite, 1, 1);
  sem_init(&station->baysMutex, 1, 1);
  sem_init(&station->ledger.stats.statsMutex, 1, 1);
  sem_init(&station->ledger.status.statusMutex, 1, 1);
  sem_init(&station->busCanPark, 1, 1);
  sem_init(&station->cannotParkEvent, 1, 0);
  sem_init(&station->busCanLeave, 1, 1);
  sem_init(&station->cannotLeaveEvent, 1, 0);
  sem_init(&station->pendingRequests, 1, 0);
  for (int i = 0; i < PARKING_BAYS; i++) {
    sem_init(&station->arrivalReq[i].pending, 1, 0);
    sem_init(&station->arrivalReq[i].waitingApproval, 1, 1);
    sem_init(&station->arrivalReq[i].approval, 1, 0);
  }
  sem_init(&station->leaveReq.pending, 1, 0);
  sem_init(&station->leaveReq.waitingApproval, 1, 1);
  sem_init(&station->leaveReq.approval, 1, 0);

  return;
}

/* Destroyes all semaphores in BusStation */
void closeStation(BusStation *station) {
  sem_destroy(&station->logWrite);
  sem_destroy(&station->baysMutex);
  sem_destroy(&station->ledger.stats.statsMutex);
  sem_destroy(&station->ledger.status.statusMutex);
  sem_destroy(&station->busCanPark);
  sem_destroy(&station->cannotParkEvent);
  sem_destroy(&station->busCanLeave);
  sem_destroy(&station->cannotLeaveEvent);
  sem_destroy(&station->pendingRequests);
  for (int i = 0; i < PARKING_BAYS; i++) {
    sem_destroy(&station->arrivalReq[i].pending);
    sem_destroy(&station->arrivalReq[i].approval);
    sem_destroy(&station->arrivalReq[i].waitingApproval);
  }
  sem_destroy(&station->leaveReq.pending);
  sem_destroy(&station->leaveReq.waitingApproval);
  sem_destroy(&station->leaveReq.approval);

  return;
}

/**
 * This function is the simulation.
 * Creates processes and handles errors in child processes
 */
int startStation(int shmid) {
  pid_t stationManagerPid;
  pid_t comptrollerPid;
  pid_t *busPids;
  LinkedList *licensePlatesList;
  struct sigaction sigact;

  /* Initialize sigaction */
  memset(&sigact, 0, sizeof(struct sigaction));
  sigact.sa_handler = sig_h;
  sigact.sa_flags = SA_RESTART;

  sigaction(SIGUSR1, &sigact, NULL);

  /* Make shmid string */
  shmidString = shmidStringify(shmid);

  /* Create/Erase contents of logfile before starting the processes */
  FILE *logFile;

  if ((logFile = fopen(LOG_FILE_PATH, "w")) == NULL) {
    free(shmidString);
    return FOPEN_ERROR;
  }
  fclose(logFile);

  /* Create station manager process */
  if ((stationManagerPid = stationManagerExec()) == FORK_FAILED ||
      stationManagerPid == EXEC_FAILED) {
    free(shmidString);
    return stationManagerPid;
  }
  SIMLOG("Station manager process just created.\n");

  if (flag == SIGNAL_RECEIVED) {
    kill(stationManagerPid, SIGUSR1);
    waitpid(stationManagerPid, NULL, 0);
    free(shmidString);
    return SIGNAL_RECEIVED;
  }

  /* Create comptroller process */
  if ((comptrollerPid = comptrollerExec()) == FORK_FAILED ||
      comptrollerPid == EXEC_FAILED) {
    if (comptrollerPid == FORK_FAILED) {
      kill(stationManagerPid, SIGUSR1);
      waitpid(stationManagerPid, NULL, 0);
    }
    free(shmidString);
    return comptrollerPid;
  }
  SIMLOG("Comptroller process just created.\n");

  /**
   * Initialize the linked list of license plates as ordered so as to
   * avoid duplicate license plates
   */
  licensePlatesList = linkedListInit(licensePlateCmp, purgeLicensePlate, 1);

  /* Seed random function for bus exec */
  srand(time(NULL));

  if (flag == SIGNAL_RECEIVED) {
    kill(stationManagerPid, SIGUSR1);
    kill(comptrollerPid, SIGUSR1);
    waitpid(stationManagerPid, NULL, 0);
    waitpid(comptrollerPid, NULL, 0);
    free(shmidString);
    return SIGNAL_RECEIVED;
  }

  busPids = malloc(sizeof(pid_t) * parser.buses);

  unsigned int i = 0;
  while (i < parser.buses && flag == OK) {
    SIMLOG("Going to create a new bus process.\n");
    if ((busPids[i] = busExec(busPids, licensePlatesList)) == FORK_FAILED) {
      linkedListPurge_f(licensePlatesList);
      kill(comptrollerPid, SIGUSR1);
      waitpid(comptrollerPid, NULL, 0);
      kill(stationManagerPid, SIGUSR1);
      waitpid(stationManagerPid, NULL, 0);
      for (unsigned int j = 0; j < i; j++) {
        kill(busPids[j], SIGUSR1);
        waitpid(busPids[j], NULL, 0);
      }
      free(busPids);
      free(shmidString);
      return FORK_FAILED;
    } else if (busPids[i] == EXEC_FAILED) {
      free(shmidString);
      return EXEC_FAILED;
    }
    i++;
    if (sleep(parser.busesCreationTimeout))
      break;
  }

  /* Now obsolete */
  linkedListPurge_f(licensePlatesList);

  if (flag == SIGNAL_RECEIVED) {
    SIMLOG("Going to force exit because of a SIGUSR1 signal.\n");
    for (unsigned int j = 0; j < i; j++) {
      kill(busPids[i], SIGUSR1);
      waitpid(busPids[i], NULL, 0);
    }
  } else {
    SIMLOG("Now waiting for a character from tty before terminating station "
           "and purging allocated memory.\n");
    getchar();
    for (i = 0; i < parser.buses; i++) {
      kill(busPids[i], SIGUSR1);
      waitpid(busPids[i], NULL, 0);
    }
  }

  free(busPids);
  kill(stationManagerPid, SIGUSR1);
  waitpid(stationManagerPid, NULL, 0);
  kill(comptrollerPid, SIGUSR1);
  waitpid(comptrollerPid, NULL, 0);

  free(shmidString);

  return flag;
}
