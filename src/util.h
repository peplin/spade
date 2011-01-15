#ifndef _UTIL_H_
#define _UTIL_H_

#include <pthread.h>

/**
 * 15-213 ProxyLab
 *
 * Christopher Peplin
 * cpeplin@andrew.cmu.edu
 *
 * util.h/.c, general utility methods
 */

/* Returns the max of two comparable values */
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
/* Returns the min of two comparable values */
#define MIN(a,b) (((a) > (b)) ? (b) : (a))

/* Using the mutex, does an atomic printf so output is never interleaved */
void atomic_printf(pthread_mutex_t* mutex, char* format, ...);

/* Checks that the result isn't < 0 and prints the error string if it is.
 * Uses atomic_printf for thread safe printing
 */ 
int check_error(int result, const char* function, pthread_mutex_t* mutex,
        int verbosity);

/*
 * get_filetype - derive file type from file name
 * Borrowed from the Tiny web server.
 */
void get_filetype(char *filename, char *filetype);

#endif // _UTIL_H_
