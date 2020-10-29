/***** quicksort.c *****/
#include "quicksort.h"

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

size_t partitioning(void *array, size_t start, size_t end, size_t elem_size,
                    cmpFunction cmpfun) {
  char *arr = (char *)array;
  char *driver = arr + (end - 1) * elem_size;
  size_t next_lte_driver = start - 1;

  size_t now_comparing;

  for (now_comparing = start; now_comparing <= end - 1; now_comparing++) {
    if (!cmpfun(arr + (now_comparing - 1) * elem_size, driver)) {
      next_lte_driver++;
      SWAP(arr + (next_lte_driver - 1) * elem_size,
           arr + (now_comparing - 1) * elem_size);
    }
  }

  SWAP(arr + ((next_lte_driver - 1) + 1) * elem_size,
       arr + (end - 1) * elem_size);

  return next_lte_driver + 1;
}

void quicksort_(void *array, size_t start, size_t end, size_t elem_size,
                cmpFunction cmpfun) {
  if (start < end) {
    size_t part_point = partitioning(array, start, end, elem_size, cmpfun);
    quicksort_(array, start, part_point - 1, elem_size, cmpfun);
    quicksort_(array, part_point + 1, end, elem_size, cmpfun);
  }

  return;
}

void quicksort(void *array, size_t size, size_t elem_size, cmpFunction cmpfun) {
  temp = malloc(elem_size);
  quicksort_(array, 1, size, elem_size, cmpfun);

  free(temp);
  return;
}