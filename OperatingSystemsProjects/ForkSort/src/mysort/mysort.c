/***** mysort.c *****/
#include "arguments.h"
#include "num_to_string.h"
#include "pipe_t.h"

#include <errno.h>
#include <fcntl.h>
#include <float.h> /* double max-min */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

char *get_coach_path(char *prog_name);
void print_results(char *prog_name, int numofcoaches, double **runtimes,
                   int *sigusr2_received, double runtime);
void force_exit(char *err_str, char *filename, Arguments *args, pipe_t *pipes,
                pid_t *coachespids, double **runtimes, int *sigusr2_received,
                int numofcoaches, char *coach_path);
void before_exit(char *filename, Arguments *args, pipe_t *pipes,
                 pid_t *coachespids, double **runtimes, int *sigusr2_received,
                 int numofcoaches, char *coach_path);
void exec_coach(char *coach_path, Coach *coach, char *filename,
                off_t numofrecords, int coachindex, int pipe_to_child);
void coach_finished(pid_t coachpid, int status, char *filename, Arguments *args,
                    pipe_t *pipes, pid_t *coachespids, double **runtimes,
                    int *sigusr2_received, int numofcoaches, char *coach_path);

#define FORCE_EXIT(ERR_STR)                                                    \
  force_exit(ERR_STR, filename, args, pipes, coachespids, runtimes,            \
             sigusr2_received, numofcoaches, coach_path);
#define BEFORE_EXIT                                                            \
  before_exit(filename, args, pipes, coachespids, runtimes, sigusr2_received,  \
              numofcoaches, coach_path);

#define MYREAD(PIPEDESC, BUFF, BUFF_SIZE)                                      \
  if (read(PIPEDESC, BUFF, BUFF_SIZE) == -1) {                                 \
    err_str = malloc(sizeof(char) * strlen(strerror(errno)) + 1);              \
    strcpy(err_str, strerror(errno));                                          \
    FORCE_EXIT(err_str);                                                       \
  }
#define COMP_RUNTIME(T1, T2, TICSPERSEC) (T2 - T1) / TICSPERSEC

int main(int argc, char **argv) {
  /* TIME VARIABLES **/
  double t1, t2, runtime;
  struct tms tb1, tb2;
  double ticspersec;
  /* ARGUMENTS VARIABLES **/
  Arguments *args;
  char *filename;
  /* FILE VARIABLES **/
  int fd;
  off_t numofrecords;
  struct stat buf; /* checks if a file is regular file or directory */
  /* COACH HANDLING VARIABLES **/
  int numofcoaches, coachindex, fork_ret, status, tmp_pipe;
  pipe_t *pipes;
  pid_t *coachespids, coachpid;
  double **runtimes;
  int *sigusr2_received;
  Coach *tmp_coach;
  char *coach_path;
  /* ERROR HANDLING **/
  char *err_str;

  /* TIME INITS **/
  t1 = (double)times(&tb1);
  ticspersec = (double)sysconf(_SC_CLK_TCK);

  /* ARGUMENTS INITS **/
  if ((args = Arguments_parser(argc, argv)) == NULL) {
    exit(EXIT_FAILURE);
  }

  if ((filename = realpath(args->filename, NULL)) == NULL) {
    fprintf(stderr, "%s - realpath() : %s\n", args->filename, strerror(errno));
    Arguments_free(args);

    exit(EXIT_FAILURE);
  }

  if (stat(filename, &buf) == -1) {
    fprintf(stderr, "%s - stat() : %s\n", filename, strerror(errno));
    Arguments_free(args);
    free(filename);

    exit(EXIT_FAILURE);
  }

  if (!S_ISREG(buf.st_mode)) {
    fprintf(stderr, "%s : is not a regular file.\n", filename);
    Arguments_free(args);
    free(filename);

    exit(EXIT_FAILURE);
  }

  /* FILE INITS **/
  if ((fd = open(filename, O_RDONLY)) == -1) {
    fprintf(stderr, "%s - open() : %s\n", filename, strerror(errno));
    Arguments_free(args);
    free(filename);

    exit(EXIT_FAILURE);
  }
  if ((numofrecords = lseek(fd, 0, SEEK_END)) == -1) {
    fprintf(stderr, "%s - lseek() : %s\n", filename, strerror(errno));
    Arguments_free(args);
    free(filename);
    close(fd);

    exit(EXIT_FAILURE);
  }
  numofrecords /= sizeof(Record);

  close(fd);
  fd = -1;

  /* COACH HANDLING INITS **/
  numofcoaches = args->numofcoaches;
  pipes = malloc(sizeof(pipe_t) * numofcoaches);
  coachespids = malloc(sizeof(pid_t) * numofcoaches);
  runtimes = malloc(sizeof(double *) * numofcoaches);
  for (coachindex = 0; coachindex < numofcoaches; coachindex++) {
    pipes[coachindex].p[0] = -1;
    pipes[coachindex].p[1] = -1;
    runtimes[coachindex] = malloc(
        sizeof(double) * 4); /* coach_time, min_time, max_time, avg_time **/
  }
  sigusr2_received = malloc(sizeof(int) * numofcoaches);
  coach_path = get_coach_path(argv[0]);

  /* Start coaches and pass parameters **/
  for (coachindex = 0; coachindex < numofcoaches; coachindex++) {
    if (pipe(pipes[coachindex].p) == -1) {
      err_str = malloc(sizeof(char) * (strlen(strerror(errno)) + 1));
      strcpy(err_str, strerror(errno));
      FORCE_EXIT(err_str);
    }

    if ((fork_ret = fork()) == -1) {
      err_str = malloc(sizeof(char) * (strlen(strerror(errno)) + 1));
      strcpy(err_str, strerror(errno));
      FORCE_EXIT(err_str);
    } else if (fork_ret == 0) {
      tmp_pipe = pipes[coachindex].p[1];
      pipes[coachindex].p[1] = -1;
      tmp_coach = malloc(sizeof(Coach));
      memcpy(tmp_coach, &args->coaches[coachindex], sizeof(Coach));
      before_exit(NULL, args, pipes, coachespids, runtimes, sigusr2_received,
                  numofcoaches, NULL);

      exec_coach(coach_path, tmp_coach, filename, numofrecords, coachindex,
                 tmp_pipe);

      /* exec failed **/
      size_t buff_size = strlen(strerror(errno));
      err_str = malloc(sizeof(char) * (buff_size + 1));
      strcpy(err_str, strerror(errno));

      write(tmp_pipe, &buff_size, sizeof(size_t));
      write(tmp_pipe, err_str, buff_size);

      free(err_str);

      close(tmp_pipe);
      exit(EXIT_FAILURE);
    } else {
      /* No further need of write end of pipe **/
      close(pipes[coachindex].p[1]);
      pipes[coachindex].p[1] = -1;
      coachespids[coachindex] = fork_ret;
    }
  }
  /* now not needed **/
  Arguments_free(args);
  args = NULL;
  free(coach_path);
  coach_path = NULL;

  coachindex = numofcoaches;
  coachpid = -1;

  while (coachindex > 0) {
    coachpid = wait(&status);
    coach_finished(coachpid, status, filename, args, pipes, coachespids,
                   runtimes, sigusr2_received, numofcoaches, coach_path);
    coachindex--;
  }

  /**** COACHES FINISHED ****/
  /* Gather results and print them **/
  t2 = (double)times(&tb2);
  runtime = COMP_RUNTIME(t1, t2, ticspersec);
  print_results(argv[0], numofcoaches, runtimes, sigusr2_received, runtime);

  BEFORE_EXIT;

  exit(EXIT_SUCCESS);
}

char *get_coach_path(char *prog_name) {
  char *ret, *curr;

  ret = malloc(
      sizeof(char) * strlen(prog_name) +
      sizeof(char)); //"coach" is shorter than "mysort" so this buffer is enough
  strcpy(ret, prog_name);

  curr = &ret[strlen(prog_name)];
  for (size_t i = 0; i < strlen("mysort"); i++) {
    curr--;
  }

  strcpy(curr, "coach");

  return ret;
}

void print_results(char *prog_name, int numofcoaches, double **runtimes,
                   int *sigusr2_received, double runtime) {
  int coachindex;
  double min_time, max_time, avg_time;
  min_time = DBL_MAX;
  max_time = DBL_MIN;
  avg_time = 0;

  printf("***** %s *****\n", prog_name);
  printf("Coordinator run time: %.2lf sec\n", runtime);
  printf("Results for %d coach(es):\n", numofcoaches);

  for (coachindex = 0; coachindex < numofcoaches; coachindex++) {
    printf("Coach %d: Run time: %lf\n", coachindex + 1,
           runtimes[coachindex][0]);
    printf("\tMinimum run time of sorters: %.2lf sec\n",
           runtimes[coachindex][1]);
    printf("\tMaximum run time of sorters: %.2lf sec\n",
           runtimes[coachindex][2]);
    printf("\tAverage run time of sorters: %.2lf sec\n",
           runtimes[coachindex][3]);
    printf("\tNumber of SIGUSR2 signals received: %d\n",
           sigusr2_received[coachindex]);
    printf("\n");

    if (runtimes[coachindex][0] < min_time) {
      min_time = runtimes[coachindex][0];
    }
    if (runtimes[coachindex][0] > max_time) {
      max_time = runtimes[coachindex][0];
    }
    avg_time += runtimes[coachindex][0];
  }

  avg_time /= numofcoaches;

  printf("Minimum run time of coaches: %.2lf sec\n", min_time);
  printf("Maximum run time of coaches: %.2lf sec\n", max_time);
  printf("Average run time of coaches: %.2lf sec\n", avg_time);

  return;
}

void force_exit(char *err_str, char *filename, Arguments *args, pipe_t *pipes,
                pid_t *coachespids, double **runtimes, int *sigusr2_received,
                int numofcoaches, char *coach_path) {
  int coachindex, status = 0;

  for (coachindex = 0; coachindex < numofcoaches; coachindex++) {
    waitpid(coachespids[coachindex], &status, WNOHANG);

    if (!WIFEXITED(status)) {
      kill(coachespids[coachindex], SIGUSR1);
      waitpid(coachespids[coachindex], &status, 0);
    }
  }

  if (err_str == NULL) {
    BEFORE_EXIT;

    exit(EXIT_FAILURE);
  }

  fprintf(stderr, "%s\n", err_str);

  free(err_str);
  BEFORE_EXIT;

  exit(EXIT_FAILURE);
}

void before_exit(char *filename, Arguments *args, pipe_t *pipes,
                 pid_t *coachespids, double **runtimes, int *sigusr2_received,
                 int numofcoaches, char *coach_path) {
  int coachindex;

  for (coachindex = 0; coachindex < numofcoaches; coachindex++) {
    if (pipes[coachindex].p[0] != -1) {
      close(pipes[coachindex].p[0]);
    }
    if (pipes[coachindex].p[1] != -1) {
      close(pipes[coachindex].p[1]);
    }

    free(runtimes[coachindex]);
  }

  if (filename != NULL)
    free(filename);
  if (args != NULL)
    Arguments_free(args);
  if (coach_path != NULL)
    free(coach_path);
  if (pipes != NULL)
    free(pipes);
  if (runtimes != NULL)
    free(runtimes);
  if (coachespids != NULL)
    free(coachespids);
  if (sigusr2_received != NULL)
    free(sigusr2_received);

  return;
}

void exec_coach(char *coach_path, Coach *coach, char *filename,
                off_t numofrecords, int coachindex, int pipe_to_child) {
  char **const args = malloc(sizeof(char *) * 8);

  args[0] = coach_path;
  args[1] = filename;
  args[2] = natural_to_string(&numofrecords, INT);
  args[3] = natural_to_string(&coach->columnid, INT);
  args[4] = malloc(sizeof(char) * 2);
  args[4][0] = coach->type;
  args[4][1] = 0;
  args[5] = natural_to_string(&coachindex, INT);
  args[6] = natural_to_string(&pipe_to_child, INT);
  args[7] = NULL;

  free(coach);

  execv(coach_path, args);

  for (int i = 0; i < 7; i++) {
    free(args[i]);
  }
  free(args);

  exit(errno);

  return;
}

void coach_finished(pid_t coachpid, int status, char *filename, Arguments *args,
                    pipe_t *pipes, pid_t *coachespids, double **runtimes,
                    int *sigusr2_received, int numofcoaches, char *coach_path) {
  int coachindex, exitcode, tmp_pipe;
  char *err_str, *coachindexstring, *aux;
  size_t buff_size;

  for (coachindex = 0; coachindex < numofcoaches; coachindex++) {
    if (coachespids[coachindex] == coachpid) {
      exitcode = WEXITSTATUS(status);
      tmp_pipe = pipes[coachindex].p[0];

      if (exitcode == EXIT_SUCCESS) {
        /* Read times **/
        MYREAD(tmp_pipe, &runtimes[coachindex][1],
               sizeof(double)); /* min_time*/
        MYREAD(tmp_pipe, &runtimes[coachindex][2],
               sizeof(double)); /* max_time */
        MYREAD(tmp_pipe, &runtimes[coachindex][3],
               sizeof(double)); /* avg_time */

        /* Read coach time **/
        MYREAD(tmp_pipe, &runtimes[coachindex][0], sizeof(double));

        /* Read number of SIGUSR2 received **/
        MYREAD(tmp_pipe, &sigusr2_received[coachindex], sizeof(int));

        close(tmp_pipe);
        pipes[coachindex].p[0] = -1;

        break;
      } else if (exitcode == EXIT_FAILURE) {
        coachindex++;
        coachindexstring = natural_to_string(&coachindex, INT);
        coachindex--;
        MYREAD(tmp_pipe, &buff_size, sizeof(size_t));
        aux = malloc(sizeof(char) * (buff_size + 1));

        MYREAD(tmp_pipe, aux, buff_size);

        err_str =
            malloc(sizeof(char) * (strlen("Coach ") + strlen(coachindexstring) +
                                   strlen(" : ") + strlen(aux) + 1));
        strcpy(err_str, "Coach ");
        strcat(err_str, coachindexstring);
        free(coachindexstring);
        strcat(err_str, " : ");
        strcat(err_str, aux);
        free(aux);

        FORCE_EXIT(err_str);
      }
    }
  }

  return;
}
