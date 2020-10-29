/***** bloom_functions.c *****/

#include "votersbloom.h"
#include "connlist.h"
#include "voter.h"
#include "votersrbt.h"

#include <stdlib.h>
#include <string.h>

size_t djb2Hash(const void *key);
size_t murmur3_32(const void *key);
size_t linear_hash(const void *key);
void votersbloom_insert(VotersBloomFilter *voters_bloom, char *key);
void votersbloom_remake(VotersBloomFilter *voters_bloom, int count,
                        VotersRBT *voters_tree);
void votersbloom_fill_traversal(VotersBloomFilter *voters_bloom,
                                VotersRBT_Node *root);

VotersBloomFilter *votersbloom_init(int count, int numofupdates) {
  VotersBloomFilter *voters_bloom = malloc(sizeof(VotersBloomFilter));

  voters_bloom->bloom = bloom_init(count, djb2Hash, murmur3_32, linear_hash);
  voters_bloom->num_of_updates = numofupdates;
  voters_bloom->updates_done = 0;

  return voters_bloom;
}

void votersbloom_insert_update(VotersBloomFilter *voters_bloom, char *key,
                               int count, VotersRBT *voters_tree) {
  votersbloom_insert(voters_bloom, key);

  if (voters_bloom->updates_done >= voters_bloom->num_of_updates) {
    votersbloom_remake(voters_bloom, count, voters_tree);
  }

  return;
}

void votersbloom_delete_update(VotersBloomFilter *voters_bloom, int count,
                               VotersRBT *voters_tree) {
  voters_bloom->updates_done++;
  if (voters_bloom->updates_done >= voters_bloom->num_of_updates) {
    votersbloom_remake(voters_bloom, count, voters_tree);
  }

  return;
}

char votersbloom_in(VotersBloomFilter *voters_bloom, char *key) {
  return bloom_in_filter(voters_bloom->bloom, key);
}

void votersbloom_fill(VotersBloomFilter *voters_bloom, VotersRBT *voters_tree) {
  ConnList *list_of_tree = rbt_get_all_nodes(voters_tree);

  ListNode *curr = list_of_tree->start_of_list;

  while (curr->next != NULL) {
    votersbloom_insert(voters_bloom, ((Voter *)curr->data)->id_num);
    curr = curr->next;
  }

  votersbloom_insert(voters_bloom, (void *)(((Voter *)curr->data)->id_num));

  connlist_free(list_of_tree);

  return;
}

void votersbloom_free(VotersBloomFilter *voters_bloom) {
  bloom_free(voters_bloom->bloom);
  free(voters_bloom);

  return;
}

/**********/

size_t linear_hash(const void *key) {
  return djb2Hash(key) + 33 * murmur3_32(key);
}

size_t djb2Hash(const void *key) {
  char *keystr = (char *)key;
  unsigned long hash = 5381;
  int c = 0;
  while ((c = *keystr++))
    hash = ((hash << 5) + hash) + c;

  return hash;
}

size_t murmur3_32(const void *key) {
  char *keystr = (char *)key;
  size_t len = strlen(keystr);
  size_t h = 33 /** = seed **/;

  if (len > 3) {
    size_t i = len >> 2;
    do {
      size_t k;
      memcpy(&k, keystr, sizeof(size_t));
      keystr += sizeof(size_t);
      k *= 0xcc9e2d51;
      k = (k << 15) | (k >> 17);
      k *= 0x1b873593;
      h ^= k;
      h = (h << 13) | (h >> 19);
      h = h * 5 + 0xe6546b64;
    } while (--i);
  }

  if (len & 3) {
    size_t i = len & 3;
    size_t k = 0;
    do {
      k <<= 8;
      k |= keystr[i - 1];
    } while (--i);
    k *= 0xcc9e2d51;
    k = (k << 15) | (k >> 17);
    k *= 0x1b873593;
    h ^= k;
  }

  h ^= len;
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;

  return h;
}

void votersbloom_insert(VotersBloomFilter *voters_bloom, char *key) {
  bloom_insert(voters_bloom->bloom, key);
  voters_bloom->updates_done++;

  return;
}

void votersbloom_remake(VotersBloomFilter *voters_bloom, int count,
                        VotersRBT *voters_tree) {
  bloom_free(voters_bloom->bloom);
  voters_bloom->bloom = bloom_init(count, djb2Hash, murmur3_32, linear_hash);
  votersbloom_fill(voters_bloom, voters_tree);

  return;
}