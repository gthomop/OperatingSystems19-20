/***** bloom.h *****/
/**
 * Bloom Filter interface.
 * The user must define 3 hash functions
 * that get an argument (the key)
 * and return a size_t number.
 * This return value is limited to the
 * capacity of the filter with modulo (%)
 */

#ifndef _BLOOM_H
#define _BLOOM_H

#include <stddef.h> /** size_t **/

typedef size_t (*hash1)(const void *);
typedef size_t (*hash2)(const void *);
typedef size_t (*hash3)(const void *);

typedef struct BloomFilter {
  char *bit_string;
  size_t size;
  hash1 h1;
  hash2 h2;
  hash3 h3;
} BloomFilter;

/** Returns NULL if not all the functions are
 * set or the count is <= 0
 */
BloomFilter *bloom_init(int count, hash1 h1, hash2 h2, hash3 h3);
/** Uses the 3 user-defined hash functions and passes the key as
 * their argument
 */
void bloom_insert(BloomFilter *bf, void *key);
/** Works the same as bloom_insert.
 * Returns 1 if the key is found in the bloom_filter,
 * else 0.
 */
char bloom_in_filter(BloomFilter *bf, void *key);
/** Frees the allocated memory for the bloom_filter
 */
void bloom_free(BloomFilter *bf);

#endif