/******* voter.h ******/
#ifndef _VOTER_H
#define _VOTER_H

typedef struct PostalCode PostalCode;
typedef struct RBT PostalCodesRBT;

/* hasVoted */
#define YES 1
#define NO 0

typedef struct Voter {
  char *id_num;
  char *name;
  char *surname;
  int age;
  char sex;
  char hasvoted;
  PostalCode *post;
} Voter;

Voter *voter_init(char *record, int *post);
void voter_free(Voter *v);

#endif