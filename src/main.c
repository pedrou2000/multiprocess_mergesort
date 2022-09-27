/*******************************************************
   Name: main.c
   Description: Main program for the execution of the algorithm.
   Author: Pedro Urbina; pedro.urbinar@estudiante.uam.es
   Date: 19/04/2020
*******************************************************/


#include <stdio.h>
#include <stdlib.h>
#include "sort.h"

/**********************************************************
   Nombre: main
   Parametros de entrada:
   -<file> : nombre del archivo con los datos a ordenar.
   -<n_levels> : numero de niveles del algoritmo.
   -<n_processes> : numero de trabajadores para ejecutar el algoritmo.
   -<delay> : microespera en ms que realizara cada trabajador.

   Parametros de salida: int, 0 en caso de que el programa se
   ejecute con normalidad y 1 en cualquier otro caso.

   Description: programa que prueba la ejecucion del algorimo
   implementado con varios procesos trabajando de manera simultanea.
**********************************************************/
int main(int argc, char *argv[]) {
    int n_levels, n_processes, delay;

    if (argc < 4) {
        fprintf(stderr, "Usage: %s <FILE> <N_LEVELS> <N_PROCESSES> [<DELAY>]\n", argv[0]);
        fprintf(stderr, "    <FILE> :        Data file\n");
        fprintf(stderr, "    <N_LEVELS> :    Number of levels (1 - %d)\n", MAX_LEVELS);
        fprintf(stderr, "    <N_PROCESSES> : Number of processes (1 - %d)\n", MAX_PARTS);
        fprintf(stderr, "    [<DELAY>] :     Delay (ms)\n");
        exit(EXIT_FAILURE);
    }

    n_levels = atoi(argv[2]);
    n_processes = atoi(argv[3]);
    if (argc > 4) {
        delay = 1e6 * atoi(argv[4]);
    }
    else {
        delay = 1e8;
    }

    return sort_multiple_processes(argv[1], n_levels, n_processes, delay);
}
