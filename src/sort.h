/*******************************************************
   Name: sort.h
   Description: Contains the headers of the functions in
   sort.c together with an explanation of their functionality.
   It also contains the definition of the data structures used
   by the algorithm.
   Author: Pedro Urbina; pedro.urbinar@estudiante.uam.es
   Date: 19/04/2020
*******************************************************/

#ifndef _SORT_H
#define _SORT_H

#include <mqueue.h>
#include <stdio.h>
#include <semaphore.h>
#include <sys/types.h>
#include "global.h"

/* Constants. */
#define MAX_DATA 100000
#define MAX_LEVELS 10
#define MAX_PARTS 512
#define MAX_STRING 1024
#define SHM_NAME "/shm_sort"
#define MESSAGE_QUEUE "/cola_sort"

#define PLOT_PERIOD 1
#define NO_MID -1
#define SECS 1

/* Type definitions. */

/* Completed flag for the tasks. */
typedef enum {
		INCOMPLETE,
		SENT,
		PROCESSING,
		COMPLETED
} Completed;

/* Task. */
typedef struct {
		Completed completed;
		int ini;
		int mid;
		int end;
} Task;

/* Estructura que contiene los descriptores de fichero una tuberia bidireccional. */
typedef struct {
		int lee_trabajador[2];
		int lee_ilustrador[2];
} TuberiaBidir;

/* Structure for the sorting problem. */
typedef struct {
		/* Array donde se almacenan las pids de todos los procesos hijos del principal,
		   para poder acceder a ellas desde los manejadores. */
		pid_t pids[MAX_PARTS];
		Task tasks[MAX_LEVELS][MAX_PARTS];
		/* Semaforos para proteger el acceso concurente a las tareas. */
		sem_t protege_tasks[MAX_PARTS];
		/* Tuberias necesarias para la comunicacion entre trabajadores e ilustrador. */
		TuberiaBidir tuberias[MAX_PARTS];
		mqd_t queue;
		int data[MAX_DATA];
		int delay;
		int n_elements;
		int n_levels;
		int n_processes;
		pid_t ppid;
} Sort;

/* Prototypes. */

/**
 * Sorts an array using bubble-sort.
 * @method bubble_sort
 * @author Pedro Urbina 
 * @date   2020-04-09
 * @param  vector      Array with the data.
 * @param  n_elements  Number of elements in the array.
 * @param  delay       Delay for the algorithm.
 * @return             ERROR in case of error, OK otherwise.
 */
Status bubble_sort(int *vector, int n_elements, int delay);

/**
 * Merges two ordered parts of an array keeping the global order.
 * @method merge
 * @author Pedro Urbina 
 * @date   2020-04-09
 * @param  vector     Array with the data.
 * @param  middle     Division between the first and second parts.
 * @param  n_elements Number of elements in the array.
 * @param  delay      Delay for the algorithm.
 * @return            ERROR in case of error, OK otherwise.
 */
Status merge(int *vector, int middle, int n_elements, int delay);

/**
 * Computes the number of parts (division) for a certain level of the sorting
 * algorithm.
 * @method get_number_parts
 * @author Pedro Urbina 
 * @date   2020-04-09
 * @param  level            Level of the algorithm.
 * @param  n_levels         Total number of levels in the algorithm.
 * @return                  Number of parts in the level.
 */
int get_number_parts(int level, int n_levels);

/**
 * Initializes the sort structure.
 * @method init_sort
 * @date   2020-04-15
 * @author Pedro Urbina 
 * @param  file_name   File with the data.
 * @param  sort        Pointer to the sort structure.
 * @param  n_levels    Total number of levels in the algorithm.
 * @param  n_processes Number of processes.
 * @param  delay       Delay for the algorithm.
 * @return             NULL in case of error, pointer to the Sort structure
 *                     in the shared memory.
 */
Sort *init_sort(char *file_name, int n_levels, int n_processes, int delay);

/**
 * Finds the correct index for the semaphore which protects the task.
 * @method get_associated_semaphore
 * @date   2020-04-25
 * @author Pedro Urbina 
 * @param  level            Level of the algorithm.
 * @param  part             Part inside the level.
 * @return             -1 in case of error or an integer representing
 *                     the index of the semaphore in the shared memory
 *										 which will protect the task passed as argument.
 */
int get_associated_semaphore(int level, int part);

/**
 * Checks if a task is already completed.
 * @method check_task_ready
 * @date   2020-04-25
 * @author Pedro Urbina 
 * @param  level            Level of the algorithm.
 * @param  part             Part inside the level.
 * @return                  FALSE if it is not completed, TRUE otherwise.
 */
Bool check_task_completed(int level, int part);

/**
 * Checks if a task is ready to be solved.
 * @method check_task_ready
 * @date   2020-04-25
 * @author Pedro Urbina 
 * @param  level            Level of the algorithm.
 * @param  part             Part inside the level.
 * @return                  FALSE if it is not ready, TRUE otherwise.
 */
Bool check_task_ready(int level, int part);

/**
 * Solves a single task of the sorting algorithm.
 * @method solve_task
 * @date   2020-04-09
 * @param  sort       Pointer to the sort structure.
 * @param  level      Level of the algorithm.
 * @param  part       Part inside the level.
 * @return            ERROR in case of error, OK otherwise.
 */
Status solve_task(Sort *sort, int level, int part);

/**
 * Frees resources for the correct worker's finalization.
 * @method clean_up_trabajador
 * @date   2020-04-19
 * @author Pedro Urbina 
 * @return None.
 */
void clean_up_trabajador();

/**
 * Function used by the main process to do the worker's tasks after forking.
 * @method trabajador
 * @date   2020-04-19
 * @author Pedro Urbina 
 * @param  n_trabajador: an integer used to identify the worker.
 * @return None.
 */
void trabajador(int n_trabajador);

/**
 * Frees resources for the correct ilustrator's finalization.
 * @method clean_up_ilustrator
 * @date   2020-04-19
 * @author Pedro Urbina 
 * @return None.
 */
void clean_up_ilustrador();

/**
 * Function used by the main process to do the ilustrator's tasks after forking.
 * @method ilustrador
 * @date   2020-04-19
 * @author Pedro Urbina 
 * @return None.
 */
void ilustrador();

/**
 * Frees resources for the correct main process's finalization.
 * @method clean_up
 * @date   2020-04-19
 * @author Pedro Urbina 
 * @return None.
 */
void clean_up();

/**
 * Solves a sorting problem using a several processes.
 * @method sort_multiple_processes
 * @date   2020-04-19
 * @author Pedro Urbina 
 * @param  file_name        File with the data.
 * @param  n_levels         Total number of levels in the algorithm.
 * @param  n_processes      Number of processes.
 * @param  delay            Delay for the algorithm.
 * @return                  ERROR in case of error, OK otherwise.
 */
Status sort_multiple_processes(char *file_name, int n_levels, int n_processes, int delay);

/**
 * Solves a sorting problem using a several processes which can work on different levels concurrently.
 * @method sort_multiple_processes_improved
 * @date   2020-04-25
 * @author Pedro Urbina 
 * @param  file_name        File with the data.
 * @param  n_levels         Total number of levels in the algorithm.
 * @param  n_processes      Number of processes.
 * @param  delay            Delay for the algorithm.
 * @return                  ERROR in case of error, OK otherwise.
 */
Status sort_multiple_processes_improved(char *file_name, int n_levels, int n_processes, int delay);

/**
 * Solves a sorting problem using a several processes timing it.
 * @method sort_multiple_processes_timed
 * @date   2020-04-25
 * @author Pedro Urbina 
 * @param  file_name        File with the data.
 * @param  n_levels         Total number of levels in the algorithm.
 * @param  n_processes      Number of processes.
 * @param  delay            Delay for the algorithm.
 * @param  f            		Pointer to the already oppened file where results will be saved.
 * @return                  ERROR in case of error, OK otherwise.
 */
Status sort_multiple_processes_timed(char *file_name, int n_levels, int n_processes, int delay, FILE *f);


#endif
