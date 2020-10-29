/***** sorter.c *****/
#define _GNU_SOURCE /* F_GETPIPE_SZ */
#include "heapsort.h"
#include "quicksort.h"
#include "record.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/types.h>
#include <unistd.h>

void force_exit(Record *array, int fd, int pipe_to_parrent, int err);
void before_exit(Record *array, int fd, int pipe_to_parent);
char cmpfun(const void *record1, const void *record2);

#define FORCE_EXIT(ERR) force_exit(array, fd, pipe_to_parent, ERR);

#define MYREAD(PIPE, BUFF, BUFF_SIZE)                                          \
  if (read(PIPE, BUFF, BUFF_SIZE) == -1)                                       \
    FORCE_EXIT(errno);

#define MYWRITE(PIPE, BUFF, BUFF_SIZE)                                         \
  if (write(PIPE, BUFF, BUFF_SIZE) == -1)                                      \
    FORCE_EXIT(errno);

#define CHECK_FLAG                                                             \
  if (flag)                                                                    \
    FORCE_EXIT(EXIT_SUCCESS);

#define COMP_RUNTIME(T1, T2, TICSPERSEC) (T2 - T1) / TICSPERSEC

int columnid;
char flag = 0;

void sig_h(int sig) {
  /* the coaches send such signals to sorters in order to
   * end them
   */
  if (sig == SIGUSR1) {
    flag = 1;
  }
}

/* Argv:
 *  0 = sorter program name
 *  1 = filename
 *  2 = first record to sort
 *  3 = last record to sort
 *  4 = columnd id to sort by
 *  5 = sorting algorithm
 *  6 = pipe to communicate with coach
 */

int main(int argc, char **argv) {
  argc = argc; /* bypass compiler warning for unused variable */
  /* start receiving signal **/
  struct sigaction sigact;
  memset(&sigact, 0, sizeof(struct sigaction));
  sigact.sa_handler = sig_h;
  sigact.sa_flags = SA_RESTART;

  sigaction(SIGUSR1, &sigact, NULL);

  /* TIME VARIABLES **/
  double t1, t2, runtime;
  struct tms tb1, tb2;
  double ticspersec;
  /* ARGUMENTS VARIABLES **/
  char *filename, algorithm;
  size_t record1, record2;
  int pipe_to_parent;
  /* FILE DESCRIPTOR **/
  int fd;
  /* RECORDS SPECIFIC VARIABLES **/
  Record *array;
  size_t array_size, recordindex;
  size_t remaining;
  size_t to_write;
  /* PIPE VARIABLE */
  int pipe_sz;

  /* TIME INITS **/
  ticspersec = (double)sysconf(_SC_CLK_TCK);
  t1 = (double)times(&tb1);

  /* ARGUMENTS INITS **/
  filename = argv[1];
  record1 = strtol(argv[2], NULL, 10);
  record2 = strtol(argv[3], NULL, 10);
  columnid = strtol(argv[4], NULL, 10); /* global for cmpfun **/
  algorithm = argv[5][0];
  pipe_to_parent = (int)strtol(argv[6], NULL, 10);

  /* FILE DESCRIPTOR INIT **/
  fd = -1;

  /* RECORDS SPECIFIC INITS **/
  array = NULL;
  array_size = 0;

  /* PIPE VAR INIT */
  pipe_sz = fcntl(pipe_to_parent, F_GETPIPE_SZ);

  CHECK_FLAG;

  /* open file **/
  if ((fd = open(filename, O_RDONLY)) == -1) {
    FORCE_EXIT(errno);
  }

  CHECK_FLAG;

  array_size = record2 - record1;
  array = malloc(sizeof(Record) * (array_size));

  /* go to the first record **/
  lseek(fd, record1 * sizeof(Record), SEEK_SET);

  CHECK_FLAG;

  for (recordindex = 0; recordindex < array_size; recordindex++) {
    MYREAD(fd, &array[recordindex], sizeof(Record));
  }

  close(fd);
  fd = -1;

  CHECK_FLAG;

  /* SORT **/
  if (algorithm == 'h') {
    heapsort(array, array_size, sizeof(Record), cmpfun);
  } else {
    quicksort(array, array_size, sizeof(Record), cmpfun);
  }

  /* compute runtime **/
  t2 = (double)times(&tb2);

  runtime = COMP_RUNTIME(t1, t2, ticspersec);

  /* write sorted records **/
  remaining = array_size;
  recordindex = 0;

  while (remaining > 0) {
    to_write = (pipe_sz - sizeof(size_t)) / sizeof(Record);
    if (to_write > remaining) {
      to_write = remaining;
    }

    MYWRITE(pipe_to_parent, &to_write, sizeof(size_t));
    MYWRITE(pipe_to_parent, &array[recordindex], to_write * sizeof(Record));

    remaining -= to_write;
    recordindex += to_write;
  }

  /* write runtime **/
  MYWRITE(pipe_to_parent, &runtime, sizeof(double));

  before_exit(array, fd, pipe_to_parent);

  kill(getppid(), SIGUSR2);

  exit(EXIT_SUCCESS);
}

void force_exit(Record *array, int fd, int pipe_to_parent, int err) {
  char *err_str;
  size_t buff_size;

  if (err > EXIT_SUCCESS) {
    err_str = malloc(sizeof(char) * (strlen(strerror(err)) + 1));
    strcpy(err_str, strerror(err));
    buff_size = strlen(err_str);
    MYWRITE(pipe_to_parent, &buff_size, sizeof(size_t));
    MYWRITE(pipe_to_parent, err_str, buff_size);
    free(err_str);
  }

  before_exit(array, fd, pipe_to_parent);

  exit(EXIT_FAILURE);
  return;
}

void before_exit(Record *array, int fd, int pipe_to_parent) {
  if (fd != -1) {
    close(fd);
  }

  if (pipe_to_parent != -1) {
    close(pipe_to_parent);
  }

  if (array != NULL) {
    free(array);
  }

  return;
}

char cmpfun(const void *record1, const void *record2) {
  Record *rec1 = (Record *)record1;
  Record *rec2 = (Record *)record2;

  switch (columnid) {
  case CUSTID:
    return rec1->custid > rec2->custid;
  case FIRSTNAME:
    return ((strcmp(rec1->FirstName, rec2->FirstName) > 0) ? (1) : (0));
  case LASTNAME:
    return ((strcmp(rec1->LastName, rec2->LastName) > 0) ? (1) : (0));
  case STREET:
    return ((strcmp(rec1->Street, rec2->Street) > 0) ? (1) : (0));
  case HOUSEID:
    return rec1->HouseID > rec2->HouseID;
  case CITY:
    return ((strcmp(rec1->City, rec2->City) > 0) ? (1) : (0));
  case POSTCODE:
    return ((strcmp(rec1->postcode, rec2->postcode) > 0) ? (1) : (0));
  case AMOUNT:
    return rec1->amount > rec2->amount;
  default:
    return 0;
  }
}