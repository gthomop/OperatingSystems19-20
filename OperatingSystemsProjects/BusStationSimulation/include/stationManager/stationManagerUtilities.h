/***** stationManagerUtilities.h *****/
#ifndef _STATIONMANAGERUTILITIES_H
#define _STATIONMANAGERUTILITIES_H

#include "busStation.h"
#include "common.h" /* TripType */

#include <stdio.h> /* FILE */

#define REFERENCE_LEDGER_PATH "./referenceLedger.file"

#define YES 1
#define NO 0

extern FILE *refLedger;

void sig_h(int signo);
int arrivalPending(BusStation *station, TripType bay);
int leavePending(BusStation *station);
int spotsAvailable(BusStation *station, TripType bay);
int busCanPark(BusStation *station);
int busCanLeave(BusStation *station);
void busLeaves(BusStation *station);
void busParks(BusStation *station, TripType bay);

#endif
