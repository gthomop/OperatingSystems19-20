/***** stationManagerUtilities.c *****/
#include "stationManagerUtilities.h"
#include "busStation.h"
#include "common.h"

#include <signal.h>
#include <stdio.h>
#include <time.h>

void sig_h(int signo) {
  if (signo == SIGUSR1) {
    flag = SIGNAL_RECEIVED;
  }

  return;
}

int arrivalPending(BusStation *station, TripType bay) {
  if (sem_trywait(&station->arrivalReq[bay].pending) != -1) {
    return YES;
  }

  return NO;
}

int leavePending(BusStation *station) {
  if (sem_trywait(&station->leaveReq.pending) != -1) {
    return YES;
  }

  return NO;
}

int busCanPark(BusStation *station) {
  if (sem_trywait(&station->busCanPark) != -1) {
    return YES;
  }

  return NO;
}

int spotsAvailable(BusStation *station, TripType bay) {
  if (station->bay[bay].available > 0) {
    return YES;
  }

  return NO;
}

int busCanLeave(BusStation *station) {
  if (sem_trywait(&station->busCanLeave) != -1) {
    return YES;
  }

  return NO;
}

unsigned int occupyFirstAvailable(BusStation *station, TripType bay) {
  for (unsigned int spot = 0; spot < station->bay[bay].spotsNumber; spot++) {
    if (SPOT(station, bay, spot) == 0) {
      SPOT(station, bay, spot) = 1;
      station->bay[bay].available--;
      return spot;
    }
  }
  /* Must never return from here */
  return 0;
}

void letParkingSpot(BusStation *station, TripType bay,
                    unsigned int spotNumber) {
  SPOT(station, bay, spotNumber) = 0;
  station->bay[bay].available++;
  return;
}

void busLeaves(BusStation *station) {
  /* Update available spots */
  if (sem_wait(&station->baysMutex))
    return;
  letParkingSpot(station, station->leaveReq.bus.parkedAt,
                 station->leaveReq.bus.spotNumber);

  time_t now = time(NULL);
  struct tm *departureTimeTM = localtime(&now);
  char departureTimeS[9];
  strftime(departureTimeS, 9, "%T", departureTimeTM);
  /* Update reference ledger */
  if (sem_wait(&station->ledger.stats.statsMutex))
    return;
  station->ledger.stats.busesLeft++;
  sem_post(&station->ledger.stats.statsMutex);
  if (sem_wait(&station->ledger.status.statusMutex))
    return;
  station->ledger.status.busesInStation--;
  sem_post(&station->ledger.status.statusMutex);

  fprintf(refLedger,
          "Leave from %s %u spot @ %s: LicensePlate: %s | To: %s | Passengers "
          "picked up: %u | Capacity: %u | Time parked: %u\n",
          TYPESTRING(station->leaveReq.bus.parkedAt),
          station->leaveReq.bus.spotNumber, departureTimeS,
          station->leaveReq.bus.licensePlate.number,
          TYPESTRING(station->leaveReq.bus.type),
          station->leaveReq.bus.passengersPickedUp,
          station->leaveReq.bus.capacity,
          (unsigned int)now - (unsigned int)station->leaveReq.bus.inStation);
  fflush(refLedger);
  sem_post(&station->baysMutex);
  /* Notify the bus that it can begin leaving the station */
  sem_post(&station->leaveReq.approval);
  LOG("SM: The bus has been notified that it can start leaving the station.\n");

  return;
}

void busParks(BusStation *station, TripType bay) {
  /* Decide where the bus will be parked. */
  if (sem_wait(&station->baysMutex))
    return;

  if (station->bay[bay].available > 0) {
    station->arrivalReq[bay].bus.parkedAt = bay;
    station->arrivalReq[bay].bus.spotNumber =
        occupyFirstAvailable(station, bay);
  } else if (bay == VOR || bay == ASK) {
    station->arrivalReq[bay].bus.parkedAt = PEL;
    station->arrivalReq[bay].bus.spotNumber =
        occupyFirstAvailable(station, PEL);
  }

  LOG("SM: The bus will park at a %s %u spot.\n",
      TYPESTRING(station->arrivalReq[bay].bus.parkedAt),
      station->arrivalReq[bay].bus.spotNumber);
  time_t now = time(NULL);
  struct tm *arrivalTimeTM = localtime(&now);
  char arrivalTimeS[9];
  strftime(arrivalTimeS, 9, "%T", arrivalTimeTM);
  if (sem_wait(&station->ledger.stats.statsMutex))
    return;
  station->ledger.stats.busesArrived++;
  sem_post(&station->ledger.stats.statsMutex);

  if (sem_wait(&station->ledger.status.statusMutex))
    return;
  station->ledger.status.busesInStation++;
  sem_post(&station->ledger.status.statusMutex);

  fprintf(refLedger,
          "Arrival @ %s: Parking Spot: %s %u | License Plate: %s | From: %s | "
          "Passengers to depark: %u | Capacity: %u\n",
          arrivalTimeS, TYPESTRING(station->arrivalReq[bay].bus.parkedAt),
          station->arrivalReq[bay].bus.spotNumber,
          station->arrivalReq[bay].bus.licensePlate.number,
          TYPESTRING(station->arrivalReq[bay].bus.type),
          station->arrivalReq[bay].bus.passengersToDepark,
          station->arrivalReq[bay].bus.capacity);
  fflush(refLedger);
  sem_post(&station->baysMutex);
  /* Notify the bus that it can get into the station and start parking */
  sem_post(&station->arrivalReq[bay].approval);
  LOG("SM: The bus has been notified that it can get into the station.\n");

  return;
}