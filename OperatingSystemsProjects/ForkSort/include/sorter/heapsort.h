/***** heapsort.c *****/
#ifndef _HEAPSORT_H
#define _HEAPSORT_H

#include <stddef.h> /** size_t **/

/* The array size is identical to the heap size */

/** (For ascending order) Should return:
 * 1 if key1 > key2,
 * 0 if key1 <= key2
 */
typedef char (*cmpFunction)(const void * /* key1 */, const void * /* key2 */);

void heapsort(void *array, size_t size, size_t elem_size, cmpFunction cmpfun);

#endif