/****** run.c ******/
#include "arguments.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void printUsage(char *programName) { // write once the usage error message
  fprintf(stderr, "Usage:\n%s -f inputfile -h|q columnid [-h|q columnid]\n",
          programName);
}

void printMaxCoaches() { fprintf(stderr, "Maximum number of coaches is 4.\n"); }

void printMinCoachess() {
  fprintf(stderr, "Minimum number of coaches is 1.\n");
}

void printColumnId() {
  fprintf(stderr,
          "The column's id flag after a -h or -q must be an integer between %d "
          "and %d inclusive.\n",
          MIN_COLUMN, MAX_COLUMN);
}

/**
 * coachindex in Arguments_parser() shall remain the same if a coach is ignored,
 * but the message in printIgnoreCoach() will be wrong if the coaches that get
 * ignored are more than 1
 */
int already_ignored = 0;

void printIgnoreCoach(int coachindex) {
  fprintf(stderr,
          "Warning: Coach %d will be ignored because there is already a coach "
          "with the same column id.\n",
          coachindex + already_ignored);
  already_ignored++;
}

Arguments *Arguments_parser(int argc, char **argv) {
  if (argc < 4) { /** 1 == program name, 2 == -f, 3 == inputfile, 4 == -h|-q **/
    printUsage(argv[0]);
    return NULL;
  }

  Arguments *args;
  int argv_index, coachindex;
  int tmp_columnid;
  char tmp_type;
  char flag;
  char *endptr;

  args = malloc(sizeof(Arguments));
  args->filename = NULL;
  args->coaches = NULL;
  args->numofcoaches = 0;

  coachindex = 0;
  flag = 0;
  endptr = NULL;

  for (argv_index = 1; argv_index < argc;
       argv_index++) {                       /** skip program name **/
    if (!(strcmp(argv[argv_index], "-f"))) { /** read filename and skip flag **/
      if (args->filename != NULL ||
          argc <= argv_index + 1) { /** duplicate -f or no filename **/
        printUsage(argv[0]);
        Arguments_free(args);

        return NULL;
      }

      argv_index++;
      args->filename = malloc(sizeof(char) * (strlen(argv[argv_index]) + 1));
      strcpy(args->filename, argv[argv_index]);
    } else if (!(strcmp(argv[argv_index], "-h")) ||
               !(strcmp(argv[argv_index], "-q"))) {
      if (args->numofcoaches >= 4) {
        printMaxCoaches();
        Arguments_free(args);

        return NULL;
      }

      tmp_type = argv[argv_index][1]; /* skip the dash */

      if (argc > argv_index + 1 && argv[argv_index + 1][0] > '1' &&
          argv[argv_index + 1][0] <
              '9') { /* the argument after -h/-q is a number */
        argv_index++;
        tmp_columnid = (int)strtol(argv[argv_index], &endptr, 10);

        if (*endptr != '\0' || tmp_columnid > MAX_COLUMN ||
            tmp_columnid < MIN_COLUMN) { /* the string was not only a number */
          printColumnId();
          Arguments_free(args);

          return NULL;
        }
      } else {
        tmp_columnid = 1;
      }

      /* Check if the column id already exists in any coach */
      for (int i = 0; i < coachindex; i++) {
        if (args->coaches[i].columnid == tmp_columnid) {
          printIgnoreCoach(coachindex + 1);
          flag = 1;
          break;
        }
      }

      if (flag == 1) { /* Same column id */
        flag = 0;
        continue;
      }

      args->numofcoaches++;
      args->coaches =
          realloc(args->coaches, args->numofcoaches * sizeof(Coach));
      args->coaches[coachindex].type = tmp_type;
      args->coaches[coachindex].columnid = tmp_columnid;
      coachindex++;
    }
  }

  if (args->numofcoaches < 1) {
    printMinCoachess();
    Arguments_free(args);

    return NULL;
  }

  if (args->filename == NULL) {
    printUsage(argv[0]);
    Arguments_free(args);

    return NULL;
  }

  return args; // no errors found
}

void Arguments_free(Arguments *arguments) { // free any allocated memory
  free(arguments->filename);
  free(arguments->coaches);
  free(arguments);

  return;
}