/***** heapsort.c *****/
#include "heapsort.h"

#include <stdlib.h>
#include <string.h>

/** Algorithms below are adjusted versions of the respective algorithms
 * found in the book "Introduction to algorithms" by Cormen, Leiserson, Rivest
 * and Stein.
 */

/** The algorithms are based on 1 indexing, so the array access has
 * to be done with 0 based indexing
 */

/****************
 * Cast of array to char* serves to avoid -Wpointer-arith warnings
 * since void pointer arithmetic is only a gcc feature
 ****************/

void *temp;

#define SWAP(x, y)                                                             \
  memcpy(temp, x, elem_size);                                                  \
  memcpy(x, y, elem_size);                                                     \
  memcpy(y, temp, elem_size);

#define LEFT(i) (i << 1)

#define RIGHT(i) LEFT(i) + 1

void restore_heap(void *array, size_t size, size_t i, size_t elem_size,
                  cmpFunction cmpfun) {
  char *arr = (char *)array;
  size_t l = LEFT(i);
  size_t r = RIGHT(i);
  size_t max;

  if (l <= size &&
      cmpfun(arr + (l - 1) * elem_size, arr + (i - 1) * elem_size)) {
    max = l;
  } else {
    max = i;
  }

  if (r <= size &&
      cmpfun(arr + (r - 1) * elem_size, arr + (max - 1) * elem_size)) {
    max = r;
  }

  if (max != i) {
    SWAP(arr + (i - 1) * elem_size, arr + (max - 1) * elem_size);
    restore_heap(array, size, max, elem_size, cmpfun);
  }

  return;
}

void build_heap(void *array, size_t size, size_t elem_size,
                cmpFunction cmpfun) {
  long int i; /* size_t would underflow on the last decrement where i == 0 */
  for (i = size / 2; i >= 1; i--) {
    restore_heap(array, size, i, elem_size, cmpfun);
  }

  return;
}

void heapsort(void *array, size_t size, size_t elem_size, cmpFunction cmpfun) {
  temp = malloc(elem_size);
  build_heap(array, size, elem_size, cmpfun);

  char *arr = (char *)array;

  size_t heap_size; /** i and heap_size **/

  for (heap_size = size; heap_size >= 2; heap_size--) {
    SWAP(arr + (1 - 1) * elem_size, arr + (heap_size - 1) * elem_size);
    /** heap_size -- **/
    restore_heap(array, heap_size - 1, 1, elem_size, cmpfun);
  }

  free(temp);

  return;
}