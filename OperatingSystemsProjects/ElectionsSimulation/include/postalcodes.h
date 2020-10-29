/***** postalcodes.h *****/
#ifndef _POSTALCODES_H
#define _POSTALCODES_H

#include "rbt.h"
#include "voterslist.h"

typedef struct RBT PostalCodesRBT;
typedef struct RBT_Node PostalCodesRBT_Node;

typedef struct PostalCode { /** the data struct member of the PostalCodesRBT **/
  int code;
  VotersList *voters;
  int havevoted;
  int voters_number;
} PostalCode;

PostalCodesRBT *postalcodes_init();
char postalcodes_insert(PostalCodesRBT *postalCodes, int code, Voter *v);
char postalcodes_remove_voter(PostalCodesRBT *postalCodes, Voter *v);
char postalcodes_delete(PostalCodesRBT *postalCodes, int code);
int postalcodes_get_votes(PostalCodesRBT *postalCodes, int code);
void postalcodes_free(PostalCodesRBT *postalCodes);
#endif