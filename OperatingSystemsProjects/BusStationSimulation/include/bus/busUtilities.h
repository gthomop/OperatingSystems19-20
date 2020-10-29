/***** busUtilities.h *****/
#ifndef _BUSUTILITIES_H
#define _BUSUTILITIES_H

#include "busStation.h"

#include <time.h>

extern short flag;

void sig_h(int signo);
void arrive(BusStation *station, Bus *thisBus);
void updateStatsArrival(BusStation *station, Bus *thisBus, time_t difference);
void stayParked(Bus *thisBus);
void pickUpPassengers(Bus *thisBus);
void leave(BusStation *station, Bus *thisBus, time_t *timeApproval);
void updateStatsLeave(BusStation *station, Bus *thisBus, time_t difference);

#endif