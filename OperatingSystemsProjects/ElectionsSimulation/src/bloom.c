/***** bloom.c *****/

#include "bloom.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

size_t bloom_comp_size(int count);
void bloom_set(BloomFilter *bf, size_t place);
char bloom_get(BloomFilter *bf, size_t place);

BloomFilter *bloom_init(int count, hash1 h1, hash2 h2, hash3 h3) {
  if (h1 == NULL || h2 == NULL || h3 == NULL || count <= 0) {
    return NULL;
  }

  BloomFilter *ret = malloc(sizeof(BloomFilter));
  ret->size = bloom_comp_size(count);
  size_t to_alloc = ret->size / 8;
  if (ret->size % 8) {
    to_alloc++;
  }

  ret->bit_string = calloc(to_alloc, sizeof(char));

  ret->h1 = h1;
  ret->h2 = h2;
  ret->h3 = h3;

  return ret;
}

void bloom_insert(BloomFilter *bf, void *key) {
  size_t h1 = (bf->h1)(key) % bf->size;
  bloom_set(bf, h1);
  size_t h2 = (bf->h2)(key) % bf->size;
  bloom_set(bf, h2);
  size_t h3 = (bf->h3)(key) % bf->size;
  bloom_set(bf, h3);

  return;
}

char bloom_in_filter(BloomFilter *bf, void *key) {
  size_t h1 = (bf->h1)(key) % bf->size;
  size_t h2 = (bf->h2)(key) % bf->size;
  size_t h3 = (bf->h3)(key) % bf->size;
  if (bloom_get(bf, h1) && bloom_get(bf, h2) && bloom_get(bf, h3)) {
    return 1;
  }

  return 0;
}

void bloom_free(BloomFilter *bf) {
  free(bf->bit_string);
  free(bf);

  return;
}
/***************/

size_t bloom_comp_size(int count) {
  size_t size = 3 * count - 1, bound;
  int flag = 0;

  do {
    flag = 0;
    size++;
    bound = size / 2;
    for (int i = 2; i < bound; i++) {
      if (size % i == 0) {
        flag = 1;
        break;
      }
    }
  } while (flag == 1);

  return size;
}

void bloom_set(BloomFilter *bf, size_t place) {
  size_t bloom_byte = place / 8;
  int exp = place % 8;

#include <stdio.h>
  printf("Bloom = %lu\n", bloom_byte);

  char *bit = bf->bit_string + bloom_byte;

  *bit |= (1 << exp);

  return;
}

char bloom_get(BloomFilter *bf, size_t place) {
  size_t bloom_byte = place / 8;
  int exp = place % 8;

  char *bit = bf->bit_string + bloom_byte;

  return *bit & (1 << exp);
}