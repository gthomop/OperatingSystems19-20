/***** busUtilities.c *****/
#include "busUtilities.h"
#include "common.h"

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <time.h>
#include <unistd.h>

void sig_h(int signo) {
  if (signo == SIGUSR1) {
    flag = SIGNAL_RECEIVED;
  }

  return;
}

void arrive(BusStation *station, Bus *thisBus) {
  /* Notify the other buses of this type that this one is filling the request */
  LOG("BUS %s: Checking if arrival request possible.\n",
      thisBus->licensePlate.number);
  if (sem_wait(&station->arrivalReq[thisBus->type].waitingApproval))
    return;
  LOG("BUS %s: Request possible!\n", thisBus->licensePlate.number);
  /* Write the request */
  memcpy(&station->arrivalReq[thisBus->type].bus, thisBus, sizeof(Bus));
  /* Let the station manager that THIS request is new */
  sem_post(&station->arrivalReq[thisBus->type].pending);
  /* Notify the station manager of the new request */
  sem_post(&station->pendingRequests);
  sem_post(&station->cannotLeaveEvent);
  LOG("BUS %s: Arrival request made, now waiting for approval.\n",
      thisBus->licensePlate.number);
  /* Wait until the station manager has approved the request */
  if (sem_wait(&station->arrivalReq[thisBus->type].approval))
    return;

  thisBus->inStation = time(NULL);

  thisBus->parkedAt = station->arrivalReq[thisBus->type].bus.parkedAt;
  thisBus->spotNumber = station->arrivalReq[thisBus->type].bus.spotNumber;
  LOG("BUS %s: Request accepted.\n", thisBus->licensePlate.number);

  /* Notify the other buses that they can make an arrival request */
  sem_post(&station->arrivalReq[thisBus->type].waitingApproval);
  LOG("BUS %s: Other buses can now make an arrival request.\n",
      thisBus->licensePlate.number);

  /* Make the maneuver before parking */
  if (sleep(thisBus->manTime))
    return;

  /* Notify that this bus ended its maneuver */
  sem_post(&station->busCanPark);

  sem_post(&station->cannotParkEvent);

  LOG("BUS %s: Now parked.\n", thisBus->licensePlate.number);

  return;
}

void updateStatsArrival(BusStation *station, Bus *thisBus, time_t difference) {
  if (sem_wait(&station->ledger.status.statusMutex))
    return;
  station->ledger.status.passengersDeparked += thisBus->passengersToDepark;
  sem_post(&station->ledger.status.statusMutex);
  if (sem_wait(&station->ledger.stats.statsMutex))
    return;
  station->ledger.stats.passengersDeparked += thisBus->passengersToDepark;
  station->ledger.stats.summaryWaiting.waitingTime += (unsigned int)difference;
  station->ledger.stats.summaryWaiting.busCount++;
  station->ledger.stats.wStats[thisBus->type].waitingTime +=
      (unsigned int)difference;
  station->ledger.stats.wStats[thisBus->type].busCount++;
  sem_post(&station->ledger.stats.statsMutex);

  return;
}

void stayParked(Bus *thisBus) {
  if (sleep(thisBus->parkPeriod))
    return;

  return;
}

void pickUpPassengers(Bus *thisBus) {
  srand(123456789);
  thisBus->passengersPickedUp = rand() % thisBus->capacity;

  return;
}

void leave(BusStation *station, Bus *thisBus, time_t *timeApproval) {
  LOG("BUS %s: Checking if leave request possible.\n",
      thisBus->licensePlate.number);
  /* Notify that a bus is filling the request */
  if (sem_wait(&station->leaveReq.waitingApproval))
    return;
  LOG("BUS %s: Request now possible.\n", thisBus->licensePlate.number);
  /* Write the request */
  memcpy(&station->leaveReq.bus, thisBus, sizeof(Bus));
  /* Let the station manager know that THIS request is new */
  sem_post(&station->leaveReq.pending);
  /* Notify the station manager of the new request */
  sem_post(&station->pendingRequests);

  sem_post(&station->cannotParkEvent);
  LOG("BUS %s: Leave request made, now waiting approval.\n",
      thisBus->licensePlate.number);
  /* Wait for approval of the request */
  if (sem_wait(&station->leaveReq.approval))
    return;
  *timeApproval = time(NULL);
  /* Let other buses write to the leave request */
  sem_post(&station->leaveReq.waitingApproval);

  /* Make the maneuver needed before leaving the station */
  if (sleep(thisBus->manTime))
    return;
  LOG("BUS %s: Now left the station.\n", thisBus->licensePlate.number);

  /* Notify that this bus ended its maneuver */
  sem_post(&station->busCanLeave);

  sem_post(&station->cannotLeaveEvent);

  return;
}

void updateStatsLeave(BusStation *station, Bus *thisBus, time_t difference) {
  if (sem_wait(&station->ledger.status.statusMutex))
    return;
  station->ledger.status.passengersDeparked -= thisBus->passengersToDepark;
  sem_post(&station->ledger.stats.statsMutex);
  if (sem_wait(&station->ledger.stats.statsMutex))
    return;
  station->ledger.stats.passengersPickedUp += thisBus->passengersPickedUp;
  station->ledger.stats.summaryWaiting.waitingTime += (unsigned int)difference;
  station->ledger.stats.wStats[thisBus->type].waitingTime +=
      (unsigned int)difference;
  sem_post(&station->ledger.status.statusMutex);

  return;
}
