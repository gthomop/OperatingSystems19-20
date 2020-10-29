/***** quicksort.h *****/
#ifndef _QUICKSORT_H
#define _QUICKSORT_H

#include <stddef.h> /** size_t **/

/* The array to be sorted must be an array of pointers */

/** (For ascending order) Should return:
 * 1 if key1 > key2,
 * 0 if key1 <= key2
 */
typedef char (*cmpFunction)(const void *, const void *);

void quicksort(void *array, size_t size, size_t elem_size, cmpFunction cmpfun);

#endif