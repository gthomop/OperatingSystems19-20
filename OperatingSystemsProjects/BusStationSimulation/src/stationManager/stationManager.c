/***** stationManager.c *****/

#include "busStation.h"
#include "common.h"
#include "stationManagerUtilities.h"

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <unistd.h>

short flag = OK;
FILE *logFile;
FILE *refLedger;

int main(int argc, char **argv) {
  struct sigaction sigactRestart, sigactNoRestart;
  int shmid;
  char *end;
  BusStation *station;

  memset(&sigactRestart, 0, sizeof(struct sigaction));
  sigactRestart.sa_handler = sig_h;
  sigactRestart.sa_flags = SA_RESTART;

  memset(&sigactNoRestart, 0, sizeof(struct sigaction));
  sigactNoRestart.sa_handler = sig_h;

  sigaction(SIGUSR1, &sigactRestart, NULL);

  /* Create logFile or erase the old and write in the new one */
  if ((logFile = fopen(LOG_FILE_PATH, "a")) == NULL) {
    perror("ERROR: SM: Log file fopen()");
    kill(getppid(), SIGUSR1);
    exit(EXIT_FAILURE);
  }

  if (argc != 3) {
    fprintf(stderr, "ERROR: SM: Wrong number of arguments.\n");
    fprintf(stderr, "SM: Usage: %s -s shmid\n", argv[0]);
    kill(getppid(), SIGUSR1);
    fclose(logFile);
    exit(EXIT_FAILURE);
  } else if (argv[1][1] != 's') {
    fprintf(stderr, "ERROR: SM: Unknown flag %s.\n", argv[1]);
    fprintf(stderr, "SM: Usage: %s -s shmid\n", argv[0]);
    kill(getppid(), SIGUSR1);
    fclose(logFile);
    exit(EXIT_FAILURE);
  }

  /* Parse shmid */
  long tmp;
  errno = 0;
  tmp = strtol(argv[2], &end, 10);
  if (errno != 0) {
    fprintf(stderr, "ERROR: SM: shmid: strtol(): %s\n", strerror(errno));
    kill(getppid(), SIGUSR1);
    fclose(logFile);
    exit(EXIT_FAILURE);
  } else if (end[0] != 0) {
    fprintf(stderr, "ERROR: SM: Wrong argument after -s flag.\n");
    kill(getppid(), SIGUSR1);
    fclose(logFile);
    exit(EXIT_FAILURE);
  } else if (tmp > INT_MAX) {
    fprintf(stderr, "ERROR: SM: Too large shmid.\n");
    kill(getppid(), SIGUSR1);
    fclose(logFile);
    exit(EXIT_FAILURE);
  } else if (tmp <= 0) {
    fprintf(stderr, "ERROR: SM: shmid cannot be <= 0.\n");
    kill(getppid(), SIGUSR1);
    fclose(logFile);
    exit(EXIT_FAILURE);
  }
  shmid = (int)tmp;

  if ((station = (BusStation *)shmat(shmid, NULL, 0)) == (void *)-1) {
    perror("ERROR: SM: shmat()");
    kill(getppid(), SIGUSR1);
    fclose(logFile);
    exit(EXIT_FAILURE);
  }

  /***** Now can write to logFile safely *****/

  LOG("SM: Started.\n");

  /* Open refLedger */
  if ((refLedger = fopen(REFERENCE_LEDGER_PATH, "w")) == NULL) {
    LOG("ERROR: SM: Reference ledger fopen(): %s\n", strerror(errno));
    kill(getppid(), SIGUSR1);
    shmdt(station);
    fclose(logFile);
    exit(EXIT_FAILURE);
  }

  LOG("SM: Opened reference ledger.\n");

  if (flag == SIGNAL_RECEIVED) {
    LOG("SM: Terminating. (SIGUSR1)\n");
    shmdt(station);
    fclose(logFile);
    fclose(refLedger);
    exit(EXIT_SUCCESS);
  }

  /* Need sem_wait() calls to NOT restart */
  sigaction(SIGUSR1, &sigactNoRestart, NULL);
  char consumed[PARKING_BAYS];
  memset(consumed, 0, sizeof(char) * PARKING_BAYS);
  LOG("SM: Going into working loop.\n");
  /* Continue statements ensure that priority is kept. */
  while (flag == OK) {
    TripType leaveFrom;

    if (sem_wait(&station->pendingRequests))
      break;

    if (leavePending(station) == YES) {
      if (busCanLeave(station) == YES) {
        leaveFrom = station->leaveReq.bus.parkedAt;
        LOG("SM: A bus is going to leave.\n");
        busLeaves(station);

        if (sem_trywait(&station->pendingRequests) != -1) {
          if (busCanPark(station) == YES) {
            if ((leaveFrom == PEL || leaveFrom == VOR) && consumed[VOR] == 1) {
              LOG("SM: A VOR bus is going to park.\n");
              busParks(station, VOR);
              consumed[VOR] = 0;
            } else if ((leaveFrom == PEL || leaveFrom == ASK) &&
                       consumed[ASK] == 1) {
              LOG("SM: A ASK bus is going to park.\n");
              busParks(station, ASK);
              consumed[ASK] = 0;
            } else if (leaveFrom == PEL && consumed[PEL] == 1) {
              LOG("SM: A PEL bus is going to park.\n");
              busParks(station, PEL);
              consumed[PEL] = 0;
            }
          } else {
            sem_post(&station->pendingRequests);
          }
        }

        continue;
      } else {
        sem_post(&station->leaveReq.pending);
        /* No pending requests except for this one */
        if (sem_trywait(&station->pendingRequests) == -1) {
          sem_post(&station->pendingRequests); /* Leave request not approved */
          /* Make the semaphore 0 */
          while (sem_trywait(&station->cannotLeaveEvent) != -1) {
          };
          /* Wait until either the bus can leave or a new request has arrived */
          if (sem_wait(&station->cannotLeaveEvent))
            break;

          continue;
        } else {
          sem_post(&station->pendingRequests); /* Leave request not approved */
        }
      }
    }

    if (flag == SIGNAL_RECEIVED)
      break;

    if (consumed[VOR] == 0 && arrivalPending(station, VOR) == YES) {
      if (busCanPark(station) == YES) {
        if (spotsAvailable(station, VOR) == YES ||
            spotsAvailable(station, PEL) == YES) {
          LOG("SM: A VOR bus is going to park.\n");
          busParks(station, VOR);
          continue;
        } else {
          consumed[VOR] = 1;
        }
      } else {
        sem_post(&station->arrivalReq[VOR].pending);
        /* No pending requests except for this one */
        if (sem_trywait(&station->pendingRequests) == -1) {
          sem_post(
              &station->pendingRequests); /* Arrival request not approved */
          /* Make the semaphore 0 */
          while (sem_trywait(&station->cannotParkEvent) != -1) {
          };
          if (sem_wait(&station->cannotParkEvent))
            break;

          continue;
        } else {
          sem_post(&station->pendingRequests); /* Could not park */
          sem_post(&station->pendingRequests); /* sem_trywait() */
          continue; /* If this bus cannot park, then the others cannot too, so
                       continue */
        }
      }
    }

    if (consumed[ASK] == 0 && arrivalPending(station, ASK) == YES) {
      if (busCanPark(station) == YES) {
        if (spotsAvailable(station, ASK) == YES ||
            spotsAvailable(station, PEL) == YES) {
          LOG("SM: A ASK bus is going to park.\n");
          busParks(station, ASK);
          continue;
        } else {
          consumed[ASK] = 1;
        }
      } else {
        sem_post(&station->arrivalReq[ASK].pending);
        /* No pending requests except for this one */
        if (sem_trywait(&station->pendingRequests) == -1) {
          sem_post(
              &station->pendingRequests); /* Arrival request not approved */
          /* Make the semaphore 0 */
          while (sem_trywait(&station->cannotParkEvent) != -1) {
          };
          if (sem_wait(&station->cannotParkEvent))
            break;

          continue;
        } else {
          sem_post(&station->pendingRequests); /* Could not park */
          sem_post(&station->pendingRequests); /* sem_trywait() */
          continue; /* If this bus cannot park, then the others cannot too, so
                       continue */
        }
      }
    }

    if (consumed[PEL] == 0 && arrivalPending(station, PEL) == YES) {
      if (busCanPark(station) == YES) {
        if (spotsAvailable(station, PEL) == YES) {
          LOG("SM: A PEL bus is going to park.\n");
          busParks(station, PEL);
          continue;
        } else {
          consumed[PEL] = 1;
        }
      } else {
        sem_post(&station->arrivalReq[PEL].pending);
        /* No pending requests except for this one */
        if (sem_trywait(&station->pendingRequests) == -1) {
          sem_post(
              &station->pendingRequests); /* Arrival request not approved */
          /* Make the semaphore 0 */
          while (sem_trywait(&station->cannotParkEvent) != -1) {
          };
          if (sem_wait(&station->cannotParkEvent))
            break;

          continue;
        } else {
          sem_post(&station->pendingRequests); /* Could not park */
          sem_post(&station->pendingRequests); /* sem_trywait() */
          continue;
        }
      }
    }
  }

  LOG("SM: Terminating. (SIGUSR1)\n");
  shmdt(station);

  fclose(logFile);
  fflush(refLedger);
  fclose(refLedger);

  exit(EXIT_SUCCESS);
}
