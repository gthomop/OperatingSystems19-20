/***** voterslist.h *****/
#ifndef _VOTERSLIST_H
#define _VOTERSLIST_H

#include "connlist.h"
#include "voter.h"

typedef ConnList VotersList;

VotersList *voterslist_init();
char voterslist_insert(VotersList *voters, Voter *v);
char voterslist_delete(VotersList *voters, Voter *v);
void voterslist_free(VotersList *voterslist);

#endif