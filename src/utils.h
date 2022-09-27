/*******************************************************
   Name: utils.h
   Description: Header file for utils.c. 
   Author: Pedro Urbina; pedro.urbinar@estudiante.uam.es
   Date: 19/04/2020
*******************************************************/

#ifndef _UTILS_H
#define _UTILS_H

#include "global.h"

/* Macros. */

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

/* Constants. */

#define MAX_SIZE_PLOT 50

/* Prototypes. */

/**
 * Computes the integer logarithm with base 2.
 * @method compute_log
 * @date   2020-04-09
 * @author Pedro Urbina
 * @param  n           Number to compute the logarithm.
 * @return             Logarithm with base 2.
 */
int compute_log(int n);

/**
 * Clears the screen in UNIX.
 * @method clear_screen
 * @date   2020-04-09
 * @author Pedro Urbina
 */
void clear_screen();

/**
 * Prints a vector in text mode.
 * @method print_vector
 * @date   2020-04-09
 * @author Pedro Urbina
 * @param  data         Array with the data.
 * @param  n_elements   Number of elements in the array.
 * @return              ERROR in case of error, OK otherwise.
 */
Status print_vector(int *data, int n_elements);

/**
 * Plots a vector, in text or graphical mode depending on its size.
 * @method plot_vector
 * @date   2020-04-09
 * @author Pedro Urbina
 * @param  data        Array with the data.
 * @param  n_elements  Number of elements in the array.
 * @return             ERROR in case of error, OK otherwise.
 */
Status plot_vector(int *data, int n_elements);

/**
 * Puts a process to sleep less than 1 second.
 * @method fast_sleep
 * @date   2020-04-09
 * @author Pedro Urbina
 * @param  nsec       Number of nanoseconds.
 */
void fast_sleep(int nsec);

#endif
