/***** comptrollerUtilities.c *****/
#include "comptrollerUtilities.h"
#include "busStation.h"

#include <signal.h>

void sig_h(int signo) {
  if (signo == SIGUSR1) {
    flag = 1;
  }
  return;
}

void printStatistics(BusStation *station) {
  if (sem_wait(&station->ledger.stats.statsMutex))
    return;

  double avgWaiting;
  double avgASKWaiting;
  double avgVORWaiting;
  double avgPELWaiting;

  avgASKWaiting = (station->ledger.stats.wStats[ASK].busCount == 0)
                      ? (0.0)
                      : ((double)station->ledger.stats.wStats[ASK].waitingTime /
                         (double)station->ledger.stats.wStats[ASK].busCount);
  avgVORWaiting = (station->ledger.stats.wStats[VOR].busCount == 0)
                      ? (0.0)
                      : ((double)station->ledger.stats.wStats[VOR].waitingTime /
                         (double)station->ledger.stats.wStats[VOR].busCount);
  avgPELWaiting = (station->ledger.stats.wStats[PEL].busCount == 0)
                      ? (0.0)
                      : ((double)station->ledger.stats.wStats[PEL].waitingTime /
                         (double)station->ledger.stats.wStats[PEL].busCount);
  avgWaiting = (station->ledger.stats.summaryWaiting.busCount == 0)
                   ? (0.0)
                   : ((double)station->ledger.stats.summaryWaiting.waitingTime /
                      (double)station->ledger.stats.summaryWaiting.busCount);

  printf("****** Statistics ******\n");

  printf("Average Waiting Time: %.2lf sec | ASK Buses Average Waiting Time: "
         "%.2lf sec | VOR Buses Average Waiting Time: %.2lf sec | PEL Buses "
         "Average Waiting Time: %.2lf sec\n",
         avgWaiting, avgASKWaiting, avgVORWaiting, avgPELWaiting);
  printf("Passengers deparked: %u | Passengers picked up: %u\n",
         station->ledger.stats.passengersDeparked,
         station->ledger.stats.passengersPickedUp);
  printf("Buses left from the station: %u | Buses arrived at the station: %u\n",
         station->ledger.stats.busesLeft, station->ledger.stats.busesArrived);

  sem_post(&station->ledger.stats.statsMutex);
  fflush(stdout);

  return;
}

void printStatus(BusStation *station) {
  if (sem_wait(&station->ledger.status.statusMutex))
    return;
  if (sem_wait(&station->baysMutex))
    return;

  printf("****** Status ******\n");
  printf("Buses currently in the station: %u\n",
         station->ledger.status.busesInStation);
  printf("Passengers deparked from in-station buses: %u\n",
         station->ledger.status.passengersDeparked);
  printf("ASK Parking Bay available spots: %u\n", station->bay[ASK].available);
  printf("VOR Parking Bay available spots: %u\n", station->bay[VOR].available);
  printf("PEL Parking Bay available spots: %u\n", station->bay[PEL].available);

  sem_post(&station->baysMutex);
  sem_post(&station->ledger.status.statusMutex);

  fflush(stdout);

  return;
}