/***** bus.h *****/
#ifndef _BUS_H
#define _BUS_H

#include "common.h"

#include <time.h>

typedef struct Plate {
  char number[8]; /* ABC-123 */
} LicensePlate;

typedef struct Bus {
  LicensePlate licensePlate;
  TripType type;
  unsigned int passengersToDepark;
  unsigned int passengersPickedUp;
  unsigned int capacity;
  unsigned int parkPeriod;
  unsigned int manTime;
  TripType parkedAt;
  unsigned int spotNumber;
  time_t inStation;
} Bus;

#endif