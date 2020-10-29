/***** votersbloomfilter.h *****/
#ifndef _VOTERSBLOOMFILTER_H
#define _VOTERSBLOOMFILTER_H

#include "bloom.h"
#include "votersrbt.h"

#include <stddef.h>

typedef struct VotersBloomFilter VotersBloomFilter;

struct VotersBloomFilter {
  BloomFilter *bloom;
  int updates_done;
  int num_of_updates;
};

VotersBloomFilter *votersbloom_init(int count, int numofupdates);
void votersbloom_insert_update(VotersBloomFilter *voters_bloom, char *key,
                               int count, VotersRBT *voters_tree);
void votersbloom_delete_update(VotersBloomFilter *voters_bloom, int count,
                               VotersRBT *voters_tree);
char votersbloom_in(VotersBloomFilter *voters_bloom, char *key);
void votersbloom_fill(VotersBloomFilter *voters_bloom, VotersRBT *voters_tree);
void votersbloom_free(VotersBloomFilter *voters_bloom);

#endif