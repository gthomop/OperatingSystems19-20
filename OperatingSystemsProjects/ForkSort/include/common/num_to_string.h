/***** num_to_string.h *****/
#ifndef _NUM_TO_STRING_H
#define _NUM_TO_STRING_H

#define LONG 0
#define INT 1
#define SIZE_T 2
#define DOUBLE 3
#define FLOAT 4

/** Both functions take a pointer to a respective-type number
 * and allocate a new char array to store the string.
 * This array must be freed by the user
 */
char *natural_to_string(void *p, char type);
char *real_to_string(void *p, char type);

#endif