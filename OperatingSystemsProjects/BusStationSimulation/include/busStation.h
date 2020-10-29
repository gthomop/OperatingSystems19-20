/***** busStation.h *****/
#ifndef _BUSSTATION_H
#define _BUSSTATION_H

#include "bus.h"
#include "common.h"

#include <semaphore.h>

#define SPOT(station, bay, spotNum)                                            \
  ((char *)station + station->bay[bay].spotsOffset)[spotNum]

typedef struct ParkBay {
  unsigned int spotsNumber;
  unsigned int available;
  size_t spotsOffset;
} ParkingBay;

typedef struct Req {
  /**
   * The station manager checks this semaphore to know
   * if this request form has been filled but not approved
   */
  sem_t pending;
  /**
   * A bus locks this mutex before starting
   * writing to the request to prevent other buses from doing so
   */
  sem_t waitingApproval;
  /**
   * A bus waits on this semaphore which is initialized with zero
   * and it is waken up when the station manager
   * has processed its request
   */
  sem_t approval;
  Bus bus;
} Request;

typedef struct WaitStats {
  unsigned int waitingTime;
  unsigned int busCount;
} WaitingStats;

typedef struct Stats {
  sem_t statsMutex;

  /** Waiting statistics:
   * the long variables contain the number of buses that
   * have left the station and, as a results, written their
   * waiting times.
   * Buses that are in the station are not counted in these.
   */
  WaitingStats wStats[PARKING_BAYS];
  WaitingStats summaryWaiting;

  /* Summary statistics */
  unsigned int passengersDeparked;
  unsigned int passengersPickedUp;
  unsigned int busesLeft;
  unsigned int busesArrived;
} Stats;

typedef struct Status {
  sem_t statusMutex;
  unsigned int
      busesInStation; /* Buses currently in station (moving and parked) */
  unsigned int passengersDeparked;
} Status;

typedef struct Ledger {
  Status status;
  Stats stats;
} ReferenceLedger;

typedef struct Station {
  /**
   * This semaphore synchs the processes that want to write
   * to the logFile FILE pointer
   */
  sem_t logWrite;
  /**
   * This semaphore is locked by any process which needs to write or read
   * to the bays' available spots numbers
   */
  sem_t baysMutex;
  /* Available parking spots numbers */
  ParkingBay bay[PARKING_BAYS];
  ReferenceLedger ledger;
  /**
   * Initialized to 1, the maximum number of buses that can
   * make a parking maneuver simultaneously
   */
  sem_t busCanPark;
  sem_t cannotParkEvent;
  /**
   * Initialized to 1, the maximum number of buses that can
   * make a departure maneuver simultaneously
   */
  sem_t busCanLeave;
  sem_t cannotLeaveEvent;
  /**
   * Initialized to 0, the station manager waits on this
   * semaphore while there are no requests pending.
   */
  sem_t pendingRequests;
  /**
   * The request "forms" the buses have to fill
   * when they want to arrive from the
   * corresponding area
   */
  Request arrivalReq[PARKING_BAYS];
  /**
   * The request "form" a bus has to fill
   * when it wants to leave the station
   */
  Request leaveReq;
} BusStation;

#endif