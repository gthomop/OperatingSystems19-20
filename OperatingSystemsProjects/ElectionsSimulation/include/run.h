/****** run.h ******/
#ifndef _RUN_H
#define _RUN_H

// auxiliary struct for parsing program arguments
typedef struct CmdParams {
  int rightUsage;
  char *params[3];
} CmdParams;

CmdParams run(int argc, char **argv);
// free memory allocated for arguments' strings
void freeCmdParams(CmdParams *arguments);

#endif