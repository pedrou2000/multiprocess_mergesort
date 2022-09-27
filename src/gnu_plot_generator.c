/*******************************************************
   Name: gnu_plot_generator.c
   Description: Program which automates the execution of 
   the algorithm saving the results into a different file.
   Author: Pedro Urbina; pedro.urbinar@estudiante.uam.es
   Date: 19/04/2020
*******************************************************/


#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <mqueue.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "sort.h"
#include "utils.h"

/**********************************************************
   Nombre: main
   Parametros de entrada:
	 -<file> : nombre del archivo con los datos a ordenar.
   -<gnu_plot_file> : nombre del archivo donde se guardaran los datos.
   -<n_levels> : numero de niveles del algoritmo.
	 -<n_processes_min> : numero de trabajadores minimo para ejecutar el algoritmo.
   -<n_processes_max> : numero de trabajadores maximo para ejecutar el algoritmo.
   -<delay> : microespera en ms que realizara cada trabajador.

   Parametros de salida: int, 0 en caso de que el programa se
   ejecute con normalidad y 1 en cualquier otro caso.

   Description: programa que prueba la ejecucion del algorimo
   implementado con varios procesos trabajando de manera simultanea
	 varias veces con distinto numero de trabajadores (de n_processes_min
 	 a n_processes_max), guardando el tiempo de ejecucion de cada uno
	 en el archivo especificado.
**********************************************************/
int main(int argc, char *argv[]) {
		int n_levels, n_processes_min, n_processes_max, delay;

		if (argc < 6) {
				fprintf(stderr, "Usage: %s <FILE> <GNU_PLOT_FILE> <N_LEVELS> <N_PROCESSES_MIN> <N_PROCESSES_MAX> [<DELAY>]\n", argv[0]);
				fprintf(stderr, "<FILE>: Data file\n");
				fprintf(stderr, "<GNU_PLOT_FILE>: File for results to be saved\n");
				fprintf(stderr, "<N_LEVELS>: Number of levels (1 - %d)\n", MAX_LEVELS);
				fprintf(stderr, "<N_PROCESSES_MIN>: Minimum number of processes\n");
				fprintf(stderr, "<N_PROCESSES_MAX>: Maximum number of processes\n");
				fprintf(stderr, "    [<DELAY>] :     Delay (ms)\n");
				exit(EXIT_FAILURE);
		}

		n_levels = atoi(argv[3]);
		n_processes_min = atoi(argv[4]);
		n_processes_max = atoi(argv[5]);
		if (argc > 4) {
				delay = 1e6 * atoi(argv[6]);
		}
		else {
				delay = 1e8;
		}
		FILE *f = fopen(argv[2], "w+");


		pid_t pid;
		for(int i = n_processes_min; i <= n_processes_max; i++) {
				printf("\n\n\n\nROUND %d\n", i);
				pid = fork();
				if(pid <  0) {
						perror("fork");
						exit(EXIT_FAILURE);
				} else if(pid == 0) {
						sort_multiple_processes_timed(argv[1], n_levels, i, delay, f);
				} else {
						wait(NULL);
				}
		}
		fclose(f);
		exit(EXIT_SUCCESS);
}
