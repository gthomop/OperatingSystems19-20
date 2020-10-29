/***** coach.c *****/
#include "num_to_string.h"
#include "pipe_t.h"
#include "record.h"

#include <errno.h>
#include <fcntl.h>
#include <float.h> /* double max-min */
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int sigusr2_received = 0;
/* If flag == 1 the program has to stop */
char flag = 0;

void comp_records(int coachnum, int sorterindex, int numofsorters,
                  size_t numofrecords, size_t *records_start,
                  size_t *records_end);
char *get_sorter_path(char *prog_name);
void compute_times(double *min_time, double *max_time, double *avg_time,
                   double *runtimes, int numofsorters);
void sorter_finished(int sorterindex, int status, int pipe_to_parent,
                     pipe_t *pipes, pid_t *sorterspids, double *runtimes,
                     Record **results, size_t *resultsmaxindexes,
                     int numofsorters, char *sorter_path, struct pollfd *fds,
                     size_t *have_read);
void before_exit(int pipe_to_parent, pipe_t *pipes, pid_t *sorterspids,
                 double *runtimes, Record **results, size_t *resultsmaxindexes,
                 int numofsorters, char *sorter_path, struct pollfd *fds,
                 size_t *have_read);
void force_exit(char *err_str, int pipe_to_parent, pipe_t *pipes,
                pid_t *sorterspids, double *runtimes, Record **results,
                size_t *resultsmaxindexes, int numofsorters, int exitcode,
                char *sorter_path, struct pollfd *fds, size_t *have_read);
void exec_sorter(char *sorter_path, char *filename, size_t record1,
                 size_t record2, int columnid, char algorithm,
                 int pipe_to_child);

#define BEFORE_EXIT                                                            \
  before_exit(pipe_to_parent, pipes, sorterspids, runtimes, results,           \
              resultsmaxindexes, numofsorters, sorter_path, fds, have_read);

#define FORCE_EXIT(ERR_STR, EXITCODE)                                          \
  force_exit(ERR_STR, pipe_to_parent, pipes, sorterspids, runtimes, results,   \
             resultsmaxindexes, numofsorters, EXITCODE, sorter_path, fds,      \
             have_read);

#define CHECK_FLAG                                                             \
  if (flag)                                                                    \
    FORCE_EXIT(NULL, EXIT_SUCCESS);

#define MYWRITE(PIPE, BUFF, BUFF_SIZE)                                         \
  if (write(PIPE, BUFF, BUFF_SIZE) == -1)                                      \
    FORCE_EXIT(NULL, errno);

#define MYREAD(PIPED, BUFF, BUFF_SIZE)                                         \
  if (read(PIPED, BUFF, BUFF_SIZE) == -1)                                      \
    FORCE_EXIT(NULL, errno);

#define COMP_RUNTIME(T1, T2, TICSPERSEC) (T2 - T1) / TICSPERSEC

void sig_h(int sig) {
  if (sig == SIGUSR2) { /* a sorter process sends a SIGUSR2 when it finishes */
    sigusr2_received++;
  } else if (sig == SIGUSR1) { /* the coordinator sends a SIGUSR1 if the program
                                  has to stop */
    flag = 1;
  }
}

/* Argv:
 *  0 = coach program name
 *  1 = filename
 *  2 = number of records in file
 *  3 = column id to sort by
 *  4 = sorting algorithm
 *  5 = this coach's number/id
 *  6 = pipe to communicate with coordinator
 */

int main(int argc, char **argv) {
  argc = argc; /* bypass compiler warning for unused variable */
  /* start receiving signals */
  struct sigaction sigact;
  memset(&sigact, 0, sizeof(struct sigaction));
  sigact.sa_handler = sig_h;
  /* SA_NODEFER: need to catch all SIGUSR2 signals, SA_RESTART: needed to allow
   * poll and waitpid to complete even if the signal handler was invoked */
  sigact.sa_flags = SA_NODEFER | SA_RESTART;

  sigaction(SIGUSR1, &sigact, NULL);
  sigaction(SIGUSR2, &sigact, NULL);

  /* TIME VARIABLES */
  double t1, t2, runtime;
  struct tms tb1, tb2;
  double ticspersec;
  /* ARGUMENTS VARIABLES */
  char *filename, algorithm;
  size_t numofrecords;
  int columnid, coachnum, pipe_to_parent;
  /* SORTERS HANDLING VARIABLES */
  int numofsorters, sorterindex, fork_ret, status;
  pipe_t *pipes;
  pid_t *sorterspids;
  double *runtimes;
  char *sorter_path;
  struct pollfd *fds;
  int poll_ret;
  char *sorterindexstring;
  /* RECORDS SPECIFIC VARIABLES */
  Record **results;
  size_t *resultsmaxindexes;
  size_t records_start, records_end;
  size_t remaining, to_read;
  size_t *have_read;
  /* ERROR HANDLING */
  char *err_str;

  /* TIME INITS */
  t1 = (double)times(&tb1);
  ticspersec = (double)sysconf(_SC_CLK_TCK);

  /* ARGUMENTS INITS */
  filename = argv[1];
  numofrecords = strtol(argv[2], NULL, 10);
  columnid = (int)strtol(argv[3], NULL, 10);
  algorithm = argv[4][0];
  coachnum = (int)strtol(argv[5], NULL, 10);
  pipe_to_parent = (int)strtol(argv[6], NULL, 10);

  /* compute sorters' number */
  numofsorters = 1 << coachnum;

  /* RECORDS INIT */
  results = malloc(sizeof(Record *) * numofsorters);
  resultsmaxindexes = malloc(sizeof(size_t) * numofsorters);
  /* SORTERS HANDLING INIT */
  sorterspids = malloc(sizeof(pid_t) * numofsorters);
  runtimes = malloc(sizeof(double) * numofsorters);
  pipes = malloc(sizeof(pipe_t) * numofsorters);
  sorter_path = get_sorter_path(argv[0]);
  fds = malloc(sizeof(struct pollfd) * numofsorters);
  have_read = NULL;

  CHECK_FLAG;

  for (sorterindex = 0; sorterindex < numofsorters; sorterindex++) {
    results[sorterindex] = NULL;
    pipes[sorterindex].p[0] = -1;
    pipes[sorterindex].p[1] = -1;

    sorterspids[sorterindex] = -1;

    fds[sorterindex].events = POLLIN;
    fds[sorterindex].revents = 0;
  }

  CHECK_FLAG;

  /*
   * The records_start variable is the offset at which the sorter has to find
   * the first record to sort, divided by sizeof(Record). The records_end
   * variable is the offset (non-inclusive) that the sorter has to reach before
   * stopping the sort, divided by sizeof(Record).
   *
   * However, only the first one needs to be multiplied by sizeof(Record), for
   * the use of lseek(). The records_end variable is only used so as to know how
   * many Records to sort.
   */
  records_start = 0;
  records_end = 0;

  /* start sorters */
  for (sorterindex = 0; sorterindex < numofsorters; sorterindex++) {
    /* create pipes */
    if (pipe(pipes[sorterindex].p) == -1) {
      FORCE_EXIT(NULL, errno);
    }

    fds[sorterindex].fd = pipes[sorterindex].p[0];

    comp_records(coachnum, sorterindex, numofsorters, numofrecords,
                 &records_start, &records_end);

    resultsmaxindexes[sorterindex] = records_end - records_start;
    results[sorterindex] =
        malloc(sizeof(Record) * resultsmaxindexes[sorterindex]);

    fork_ret = fork();

    if (fork_ret == -1) {
      FORCE_EXIT(NULL, errno);
    } else if (fork_ret == 0) {
      int tmp_pipe;

      close(pipes[sorterindex].p[0]); /* close the read end of the child */
      pipes[sorterindex].p[0] = -1;
      tmp_pipe = pipes[sorterindex].p[1];
      /* exec_sorter() closes all pipes that are not equal to -1,
       * so the command below prevents it from doing so for
       * the pipe to be passed to the new exec-ed process
       */
      pipes[sorterindex].p[1] = -1;

      before_exit(pipe_to_parent, pipes, sorterspids, runtimes, results,
                  resultsmaxindexes, numofsorters, NULL, fds, NULL);
      exec_sorter(sorter_path, filename, records_start, records_end, columnid,
                  algorithm, tmp_pipe);

      /* Exec failed */
      size_t buff_size = strlen(strerror(errno));
      err_str = malloc(sizeof(char) * (buff_size + 1));
      strcpy(err_str, strerror(errno));

      write(tmp_pipe, &buff_size, sizeof(size_t));
      write(tmp_pipe, err_str, buff_size);

      free(err_str);

      close(tmp_pipe);
      exit(EXIT_FAILURE);
    } else {
      close(pipes[sorterindex].p[1]); /* close the write end */
      pipes[sorterindex].p[1] = -1;
      sorterspids[sorterindex] = fork_ret;
    }
  }

  free(sorter_path);
  sorter_path = NULL;

  CHECK_FLAG;

  /* Start receiving records from sorters */

  remaining = numofrecords;
  have_read = malloc(sizeof(size_t) * numofsorters);
  memset(have_read, 0, sizeof(size_t) * numofsorters);

  while (remaining > 0) {
    /* wait indefinitely until either a pipe has data to be read
     * or a signal is received
     */
    poll_ret = poll(fds, numofsorters, -1);
    CHECK_FLAG;
    if (poll_ret == -1) {
      FORCE_EXIT(NULL, errno);
    } else if (poll_ret > 0) {
      for (sorterindex = 0; sorterindex < numofsorters; sorterindex++) {
        if (fds[sorterindex].revents &
            POLLIN) { /* there is data for the coach to read in the pipe */
          fds[sorterindex].revents = 0;
          MYREAD(fds[sorterindex].fd, &to_read, sizeof(size_t));

          MYREAD(fds[sorterindex].fd,
                 &results[sorterindex][have_read[sorterindex]],
                 to_read * sizeof(Record));
          remaining -= to_read;
          have_read[sorterindex] += to_read;
          if (have_read[sorterindex] ==
              resultsmaxindexes[sorterindex]) { /* if the coach just received
                                                   the last record */
            MYREAD(fds[sorterindex].fd, &runtimes[sorterindex],
                   sizeof(double)); /* get runtime of sorter */
            waitpid(sorterspids[sorterindex], &status,
                    0); /* wait until the sorter finishes and then call
                           sorter_finished */
            sorter_finished(sorterindex, status, pipe_to_parent, pipes,
                            sorterspids, runtimes, results, resultsmaxindexes,
                            numofsorters, sorter_path, fds, have_read);
            fds[sorterindex].fd =
                -1; /* prevent poll from checking for this pipe again */
          } else if (have_read[sorterindex] > resultsmaxindexes[sorterindex]) {
            sorterindex++;
            sorterindexstring = natural_to_string(&sorterindex, INT);
            sorterindex--;
            err_str = malloc(
                sizeof(char) *
                (strlen("An error occurred when getting records from sorter ") +
                 strlen(sorterindexstring) + strlen(".") + 1));
            strcpy(err_str,
                   "An error occurred when getting records from sorter ");
            strcat(err_str, sorterindexstring);
            free(sorterindexstring);
            strcat(err_str, ".");
            FORCE_EXIT(err_str, EXIT_FAILURE);
          }
        } else if (fds[sorterindex].revents &
                   POLLHUP) { /* the sorter closed its write end of the pipe */
          /* something wrong has happened if execution reaches this block
           * because, as soon as the coach realises that a sorter is not
           * supposed to send more data, it makes poll ignore this sorter's
           * pipe. So, if POLLHUP is received, a sorter
           * that has to send more records has closed its pipe
           * because of an error, so terminate the coach
           */
          waitpid(sorterspids[sorterindex], &status, 0);
          sorter_finished(sorterindex, status, pipe_to_parent, pipes,
                          sorterspids, runtimes, results, resultsmaxindexes,
                          numofsorters, sorter_path, fds, have_read);
          /* if sorter_finished returns, this means that an undefined error
           * occurred, so force_exit */
          sorterindex++;
          sorterindexstring = natural_to_string(&sorterindex, INT);
          sorterindex--;
          err_str = malloc(
              sizeof(char) *
              (strlen("An error occurred when communicating with sorter ") +
               strlen(sorterindexstring) + strlen(".") + 1));
          strcpy(err_str, "An error occurred when communicating with sorter ");
          strcat(err_str, sorterindexstring);
          free(sorterindexstring);
          strcat(err_str, ".");
          FORCE_EXIT(err_str, EXIT_FAILURE);
        }
      }
    }
    /* else if (poll_ret == 0) means that no file descriptor was ready and a
     * SIGUSR2 was received */
  }

  free(fds);
  fds = NULL;

  free(have_read);
  have_read = NULL;

  /**********************************************************************/

  /*** Gather results and write them ***/
  char *newfilename, *columnidstring;
  int results_file;
  size_t recordindex;
  double min_time, max_time, avg_time;

  /* Write merged sorters' results to file filename.columnid */
  columnidstring = natural_to_string(&columnid, INT);
  newfilename = malloc(sizeof(char) * (strlen(filename) + strlen(".") +
                                       strlen(columnidstring) + 1));
  strcpy(newfilename, filename);
  strcat(newfilename, ".");
  strcat(newfilename, columnidstring);
  free(columnidstring);

  results_file = open(newfilename, O_WRONLY | O_CREAT | O_TRUNC,
                      S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  free(newfilename);

  if (results_file == -1) {
    FORCE_EXIT(NULL, errno);
  }

  char *to_print, *custid_string, *houseid_string, *amount_string;
  size_t length;
  Record *tmp;

  for (sorterindex = 0; sorterindex < numofsorters; sorterindex++) {
    for (recordindex = 0; recordindex < resultsmaxindexes[sorterindex];
         recordindex++) {
      tmp = &results[sorterindex][recordindex];
      custid_string = natural_to_string(&tmp->custid, LONG);
      houseid_string = natural_to_string(&tmp->HouseID, INT);
      amount_string = real_to_string(&tmp->amount, FLOAT);
      length = strlen(custid_string) + strlen(tmp->FirstName) +
               strlen(tmp->LastName) + strlen(tmp->Street) +
               strlen(houseid_string) + strlen(tmp->City) +
               strlen(tmp->postcode) + strlen(amount_string) +
               8 /* 7 spaces between fields and newline */;
      to_print = malloc(sizeof(char) * (length + 1));
      sprintf(to_print, "%s %s %s %s %s %s %s %s\n", custid_string,
              tmp->FirstName, tmp->LastName, tmp->Street, houseid_string,
              tmp->City, tmp->postcode, amount_string);
      MYWRITE(results_file, to_print, length);
      free(custid_string);
      free(houseid_string);
      free(amount_string);
      free(to_print);
    }
  }

  close(results_file);

  compute_times(&min_time, &max_time, &avg_time, runtimes, numofsorters);

  t2 = (double)times(&tb2);

  /* Write sorters' times */
  MYWRITE(pipe_to_parent, &min_time, sizeof(double)); /* min_time */
  MYWRITE(pipe_to_parent, &max_time, sizeof(double)); /* max_time */
  MYWRITE(pipe_to_parent, &avg_time, sizeof(double)); /* avg_time */

  /* Write coach's time */
  runtime = COMP_RUNTIME(t1, t2, ticspersec);
  MYWRITE(pipe_to_parent, &runtime, sizeof(double));

  /* Write number of SIGUSR2 received */
  MYWRITE(pipe_to_parent, &sigusr2_received, sizeof(int));

  BEFORE_EXIT;

  exit(EXIT_SUCCESS);
}

void comp_records(int coachnum, int sorterindex, int numofsorters,
                  size_t numofrecords, size_t *records_start,
                  size_t *records_end) {
  *records_start = *records_end;

  if (coachnum == 0) {
    *records_end = numofrecords;
  } else if (coachnum == 1) {
    if (sorterindex == 0) {
      *records_end = numofrecords / 2;
    } else {
      *records_start = *records_end;
      *records_end = numofrecords;
    }
  } else if (coachnum == 2) {
    if (sorterindex == 0) {
      *records_end = numofrecords / 8;
    } else if (sorterindex != numofsorters - 1) {
      *records_end = *records_start + ((numofrecords / 8) << (sorterindex - 1));
    } else {
      *records_end = numofrecords;
    }
  } else if (coachnum == 3) {
    if (sorterindex <= 3) {
      *records_end = *records_start + numofrecords / 16;
    } else if (sorterindex <= 5) {
      *records_end = *records_start + numofrecords / 16 * 2;
    } else if (sorterindex <= 6) {
      *records_end = *records_start + numofrecords / 16 * 4;
    } else { /* last */
      *records_end = numofrecords;
    }
  }

  return;
}

char *get_sorter_path(char *prog_name) {
  char *ret, *curr;

  /* "coach" is shorter than "sorter" by 1 character, so we need 1 more to fit
   * the string */
  ret = malloc(sizeof(char) * (strlen(prog_name) + 1 + 1));
  strcpy(ret, prog_name);

  curr = &ret[strlen(ret)];

  for (size_t i = 0; i < strlen("coach"); i++) {
    curr--;
  }

  strcpy(curr, "sorter");

  return ret;
}

void compute_times(double *min_time, double *max_time, double *avg_time,
                   double *runtimes, int numofsorters) {
  int sorterindex;

  *min_time = DBL_MAX;
  *max_time = DBL_MIN;
  *avg_time = 0.0;

  for (sorterindex = 0; sorterindex < numofsorters; sorterindex++) {
    if (runtimes[sorterindex] < *min_time)
      *min_time = runtimes[sorterindex];
    if (runtimes[sorterindex] > *max_time)
      *max_time = runtimes[sorterindex];

    *avg_time += runtimes[sorterindex];
  }

  *avg_time /= (double)numofsorters;

  return;
}

void exec_sorter(char *sorter_path, char *filename, size_t record1,
                 size_t record2, int columnid, char algorithm,
                 int pipe_to_child) {
  char **const arguments = malloc(sizeof(char *) * 8);

  arguments[0] = sorter_path;
  arguments[1] = filename;
  arguments[2] = natural_to_string(&record1, SIZE_T);
  arguments[3] = natural_to_string(&record2, SIZE_T);
  arguments[4] = natural_to_string(&columnid, INT);
  arguments[5] = malloc(sizeof(char) * 2);
  arguments[5][0] = algorithm;
  arguments[5][1] = 0;
  arguments[6] = natural_to_string(&pipe_to_child, INT);
  arguments[7] = NULL;

  execv(sorter_path, arguments);

  for (int i = 0; i < 7; i++) {
    if (i != 1) /* filename is from argv */
      free(arguments[i]);
  }
  free(arguments);

  return;
}

void force_exit(char *err_str, int pipe_to_parent, pipe_t *pipes,
                pid_t *sorterspids, double *runtimes, Record **results,
                size_t *resultsmaxindexes, int numofsorters, int err,
                char *sorter_path, struct pollfd *fds, size_t *have_read) {
  int sorterindex, status = 0;
  size_t buff_size;

  for (sorterindex = 0; sorterindex < numofsorters; sorterindex++) {
    waitpid(sorterspids[sorterindex], &status, WNOHANG);

    if (!WIFEXITED(status)) {
      kill(sorterspids[sorterindex], SIGUSR1);
      waitpid(sorterspids[sorterindex], &status, 0);
    }
  }

  if (err_str == NULL) { /* error happened in coach */
    err_str = malloc(sizeof(char) * (strlen(strerror(err)) + 1));
  }

  buff_size = strlen(err_str);

  MYWRITE(pipe_to_parent, &buff_size, sizeof(size_t));
  MYWRITE(pipe_to_parent, err_str, buff_size);

  free(err_str);

  BEFORE_EXIT;

  exit(EXIT_FAILURE);
}

void before_exit(int pipe_to_parent, pipe_t *pipes, pid_t *sorterspids,
                 double *runtimes, Record **results, size_t *resultsmaxindexes,
                 int numofsorters, char *sorter_path, struct pollfd *fds,
                 size_t *have_read) {
  int sorterindex;

  for (sorterindex = 0; sorterindex < numofsorters; sorterindex++) {
    if (pipes[sorterindex].p[0] != -1) {
      close(pipes[sorterindex].p[0]);
      pipes[sorterindex].p[0] = -1;
    }
    if (results[sorterindex] != NULL) {
      free(results[sorterindex]);
    }
  }

  if (pipe_to_parent != -1)
    close(pipe_to_parent);
  if (pipes != NULL)
    free(pipes);
  if (sorterspids != NULL)
    free(sorterspids);
  if (runtimes != NULL)
    free(runtimes);
  if (results != NULL)
    free(results);
  if (resultsmaxindexes != NULL)
    free(resultsmaxindexes);
  if (sorter_path != NULL)
    free(sorter_path);
  if (fds != NULL)
    free(fds);
  if (have_read != NULL)
    free(have_read);

  return;
}

void sorter_finished(int sorterindex, int status, int pipe_to_parent,
                     pipe_t *pipes, pid_t *sorterspids, double *runtimes,
                     Record **results, size_t *resultsmaxindexes,
                     int numofsorters, char *sorter_path, struct pollfd *fds,
                     size_t *have_read) {
  int exitcode;
  char *err_str, *sorterindexstring, *aux;
  size_t buff_size;

  exitcode = WEXITSTATUS(status);
  if (exitcode == EXIT_SUCCESS) {
    close(pipes[sorterindex].p[0]);
    pipes[sorterindex].p[0] = -1;
  } else if (exitcode == EXIT_FAILURE) {
    sorterindex++;
    sorterindexstring = natural_to_string(&sorterindex, INT);
    sorterindex--;
    MYREAD(pipes[sorterindex].p[0], &buff_size, sizeof(size_t));
    aux = malloc(sizeof(char) * buff_size + sizeof(char));
    aux[buff_size] = 0;
    MYREAD(pipes[sorterindex].p[0], aux, buff_size);

    err_str =
        malloc(sizeof(char) * (strlen("Sorter ") + strlen(sorterindexstring) +
                               strlen(" : ") + strlen(aux) + 1));
    strcpy(err_str, "Sorter ");
    strcat(err_str, sorterindexstring);
    free(sorterindexstring);
    strcat(err_str, " : ");
    strcat(err_str, aux);
    free(aux);

    buff_size = strlen(err_str);
    MYWRITE(pipe_to_parent, &buff_size, sizeof(size_t));
    MYWRITE(pipe_to_parent, err_str, buff_size);

    FORCE_EXIT(err_str, EXIT_FAILURE);
  }

  return;
}