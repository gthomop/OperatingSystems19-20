/***** votersrbt.h *****/
#ifndef _VOTERSRBT_H
#define _VOTERSRBT_H

#include "rbt.h"
#include "voter.h"

typedef RBT VotersRBT;
typedef RBT_Node VotersRBT_Node;

VotersRBT *votersrbt_init();
char votersrbt_insert(VotersRBT *tree, Voter *v);
Voter *votersrbt_find(VotersRBT *tree, char *idnum);
char votersrbt_delete(VotersRBT *tree, char *idnum);
void votersrbt_free(VotersRBT *tree);

#endif