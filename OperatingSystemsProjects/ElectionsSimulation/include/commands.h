#ifndef _COMMANDS_H
#define _COMMANDS_H

#include "postalcodes.h"
#include "votersbloom.h"
#include "votersrbt.h"

void lbf(VotersBloomFilter *voters_bloom, char *key);
void lrb(VotersRBT *voters_tree, char *key);
void ins(VotersRBT *voters_tree, VotersBloomFilter *voters_bloom,
         PostalCodesRBT *postalcodes, int *count, char *record);
void find(VotersRBT *voters_tree, VotersBloomFilter *voters_bloom, char *key);
void delete (VotersRBT *voters_tree, VotersBloomFilter *voters_bloom,
             PostalCodesRBT *postalCodes, char *key, int *count,
             int *havevoted);
void vote(VotersRBT *voters_tree, VotersBloomFilter *voters_bloom, char *key,
          int *havevoted);
void load(VotersRBT *voters_tree, VotersBloomFilter *voters_bloom,
          char *fileofkeys, int *havevoted);
void voted(PostalCodesRBT *postalCodes, char *codestr, int havevoted);
void votedperpc(PostalCodesRBT *postalCodes);

#endif