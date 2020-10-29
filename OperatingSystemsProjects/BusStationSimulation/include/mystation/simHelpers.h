/***** simHelpers.h *****/
#ifndef _SIMHELPERS_H
#define _SIMHELPER_H

#include "linkedList.h" /* Linked List */

#include <stdio.h> /* FILE pointer */
#include <unistd.h>/* pid_t */

/* Signal handling variable */
extern short flag;

/* Shared segment id string */
extern char *shmidString;


/* Station signal handler */
void sig_h(int signo);

/* License plates connected list functions */

char licensePlateCmp(const void *key, const void *data);
void purgeLicensePlate(void *data);

/* Functions for exec */

char *shmidStringify(int shmid);
void generateLicensePlate(char licensePlate[8]);

/* Exec functions */

pid_t stationManagerExec();
pid_t comptrollerExec();
pid_t busExec(pid_t *busPids, LinkedList *licensePlatesList);

#endif
