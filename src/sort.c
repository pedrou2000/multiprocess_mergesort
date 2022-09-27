/*******************************************************
   Name: sort.c
   Description: Contains the main functions for the execution
   of the algorithm.
   Author: Pedro Urbina; pedro.urbinar@estudiante.uam.es
   Date: 19/04/2020
*******************************************************/

#define _POSIX_C_SOURCE 200112L

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <mqueue.h>
#include <semaphore.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include "sort.h"
#include "utils.h"


/* Puntero a la memoria compartida, que es el mismo para todos
   los procesos de este programa ya que son hijos y heredan el
   espacio de memorias virtuales. */
Sort *sort = NULL;

/* Array donde los trabajadores guardan informacion acerca de la tarea
   que estan realizando, para poderla mandar por la tuberia directamente desde
   el manejador de la señal SIGALRM. */
volatile sig_atomic_t pipe_message[2] = {-1, -1};

/* Variable global que almacena un entero que se le asigna a cada hijo
   del proceso principal para poder acceder a sus propias tuberias en los
   manejadores o diferenciar si es el ilustrador o un trabajador. */
volatile sig_atomic_t num_trabajador = -2;

/* Puntero a la memoria reservada para la tabla auxiliar del algoritmo
   mergesort, ya que en caso de recibir SIGTERM hay que liberar esa zona
   de memoria antes de terminar la ejecucion del proceso. */
int *aux_table = NULL;





/******************************************************************************
   Nombre: manejador
   Parametros de entrada: entero representado el numero de la señal que lo llama.
   Parametros de salida: Ninguno
   Description: Funcion que recoge el funcionamiento de cada manejador de señal
                 asociado a la señal recibida como argumento.
******************************************************************************/
void manejador (int sig) {
		/* La señal SIGUSR1 tan solo despierta al proceso principal del sigsuspend. */
		if (sig == SIGUSR1) {

		}

		/* Al recibir la señal SIGINT el proceso principal libera los recursos,
		   manda la señal SIGTERM a todos sus hijos, cierra el semaforo
		   global y espera a que acaben sus hijos para tambien el terminar
		   correctamente. */
		if (sig == SIGINT) {
				if(sort == NULL) exit(EXIT_SUCCESS);

				int aux = 0, n_sem = 1 << (sort->n_levels - 1);
				char error_message_kill[200] = "Error when sending SIGTERM to child processes.\n";
				char error_message_waitpid[200] = "Error when waiting for child processes.\n";
				char error_message_sem_destroy[200] = "Error when destroying a semaphore.\n";
				char error_message_mq_close[200] = "Error when closing the message queue.\n";
				char error_message_munmap[200] = "Error when unmaping the shared memory adress.\n";

				/* Si los hijos han sido incializados, les mandamos SIGTERM y les esperamos. */
				if(sort->pids[0] != -1) {
						for(int z = 0; z <= sort->n_processes; z++) {
								if(kill(sort->pids[z], SIGTERM) == -1) {
										aux = 1;
										write (2, (void*)error_message_kill, sizeof(error_message_kill));
								}
						}
						if(waitpid(-1, NULL, 0) == -1) {
								aux = 1;
								write (2, (void*)error_message_waitpid, sizeof(error_message_waitpid));
						}
				}
				for(int z = 0; z < n_sem; z++) {
						if(sem_destroy(&(sort->protege_tasks[z])) != 0) {
								aux = 1;
								write (2, (void*)error_message_sem_destroy, sizeof(error_message_sem_destroy));
						}
				}
				if(mq_close(sort->queue) != 0) {
						aux = 1;
						write (2, (void*)error_message_mq_close, sizeof(error_message_mq_close));
				}
				if(munmap(sort, sizeof(Sort)) == -1) {
						aux = 1;
						write (2, (void*)error_message_munmap, sizeof(error_message_munmap));
				}

				if (aux == 1) {
						exit(EXIT_FAILURE);
				}
				exit(EXIT_SUCCESS);
		}

		/* La señal SIGTERM simplemente acaba la ejecucion del proceso que la reciba,
		   liberando correctamente los recursos. */
		if (sig == SIGTERM) {
				int aux = 0;
				char error_message_close[200] = "Error when closing a pipe.\n";
				char error_message_mq_close[200] = "Error when closing the message queue.\n";
				char error_message_munmap[200] = "Error when unmaping the shared memory adress.\n";

				if(num_trabajador == (sig_atomic_t)-2) {
						aux = 1;
				} else if(num_trabajador == (sig_atomic_t)-1) {
						for(int i = 0; i < sort->n_processes; i++) {
								if(close(sort->tuberias[i].lee_trabajador[1]) == -1) {
										aux = 1;
										write (2, (void*)error_message_close, sizeof(error_message_close));
								}
								if(close(sort->tuberias[i].lee_ilustrador[0]) == -1) {
										aux = 1;
										write (2, (void*)error_message_close, sizeof(error_message_close));
								}
						}
						if(munmap(sort, sizeof(Sort)) == -1) aux = 1;
				} else {
						if(aux_table != NULL) free(aux_table);

						if(mq_close(sort->queue) == -1) {
								aux = 1;
								write (2, (void*)error_message_mq_close, sizeof(error_message_mq_close));
						}
						if(close(sort->tuberias[num_trabajador].lee_trabajador[0]) == -1) {
								aux = 1;
								write (2, (void*)error_message_close, sizeof(error_message_close));
						}

						if(close(sort->tuberias[num_trabajador].lee_ilustrador[1]) ==-1) {
								aux = 1;
								write (2, (void*)error_message_close, sizeof(error_message_close));
						}
						if(munmap(sort, sizeof(Sort)) == -1) {
								aux = 1;
								write (2, (void*)error_message_munmap, sizeof(error_message_munmap));
						}
				}
				if(aux == 1) {
						exit(EXIT_FAILURE);
				}
				exit(EXIT_SUCCESS);
		}

		/* El trabajador escribe en el pipe la informacion que ha almacenado previamente
		   en variables globales ya que no se pueden pasar argumentos a los manejadores
		   de señales. Despues de esto espera a leer el 1 de la otra tuberia y establece
		   la siguiente alarma. */
		if (sig == SIGALRM) {
				int aux = errno;
				char error_message_write[200] = "A worker had an ERROR when writing on the pipe. He will send the signal SIGINT to the main process in order to finish the execution.\n";
				char error_message_read[200] = "A worker had an ERROR when reading from the pipe. He will send the signal SIGINT to the main process in order to finish the execution.\n";

				if(write (sort->tuberias[num_trabajador].lee_ilustrador[1], (void*)pipe_message, 2*sizeof(int)) == -1) {
						write (2, (void*)error_message_write, sizeof(error_message_write));
						kill(sort->ppid, SIGINT);
						return;
				}
				ssize_t nbytes = 0;
				int readed = 0;
				do {
						nbytes = read(sort->tuberias[num_trabajador].lee_trabajador[0], &readed, sizeof(int));
						if(nbytes == -1) {
								write (2, (void*)error_message_read, sizeof(error_message_read));
								kill(sort->ppid, SIGINT);
								return;
						}
				} while(nbytes < 4 && readed != 1);
				alarm(SECS);
				errno = aux;
		}
}




Status bubble_sort(int *vector, int n_elements, int delay) {
		int i, j;
		int temp;
		sigset_t set;
		sigemptyset(&set);
		sigaddset(&set, SIGALRM);

		if ((!(vector)) || (n_elements <= 0)) {
				return ERROR;
		}

		for (i = 0; i < n_elements - 1; i++) {
				for (j = 0; j < n_elements - i - 1; j++) {
						/* Delay. */
						fast_sleep(delay);
						if (vector[j] > vector[j+1]) {
								temp = vector[j];


								/* Bloqueamos la señal SIGALRM temporalmente para modificar los
								   datos y que no haya problemas de lecturas incositentes por parte
								   del ilustrador. */
								if (sigprocmask(SIG_BLOCK, &set, NULL) < 0) {
										perror("sigprocmask");
										return ERROR;
								}

								vector[j] = vector[j + 1];
								vector[j + 1] = temp;

								if (sigprocmask(SIG_UNBLOCK, &set, NULL) < 0) {
										perror("sigprocmask");
										return ERROR;
								}

						}
				}
		}

		return OK;
}

Status merge(int *vector, int middle, int n_elements, int delay) {
		int i, j, k, l, m;
		sigset_t set;
		sigemptyset(&set);
		sigaddset(&set, SIGTERM);

		/* Ahora debemos almacenar el puntero a la tabla auxiliar en una variable
		   global para poder liberarla en caso de recibir SIGTERM. */
		if (sigprocmask(SIG_BLOCK, &set, NULL) < 0) {
				perror("sigprocmask");
				return ERROR;
		}
		if (!(aux_table = (int *)malloc(n_elements * sizeof(int)))) {
				return ERROR;
		}
		if (sigprocmask(SIG_UNBLOCK, &set, NULL) < 0) {
				perror("sigprocmask");
				return ERROR;
		}

		for (i = 0; i < n_elements; i++) {
				aux_table[i] = vector[i];
		}


		sigemptyset(&set);
		sigaddset(&set, SIGALRM);
		i = 0; j = middle;
		for (k = 0; k < n_elements; k++) {
				/* Delay. */
				fast_sleep(delay);

				/* Bloqueamos la señal SIGALRM temporalmente para modificar los
				   datos y que no haya problemas de lecturas incositentes por parte
				   del ilustrador. */
				if (sigprocmask(SIG_BLOCK, &set, NULL) < 0) {
						perror("sigprocmask");
						return ERROR;
				}

				if ((i < middle) && ((j >= n_elements) || (aux_table[i] < aux_table[j]))) {
						vector[k] = aux_table[i];
						i++;
				}
				else {
						vector[k] = aux_table[j];
						j++;
				}

				/* This part is not needed, and it is computationally expensive, but
				   it allows to visualize a partial mixture. */
				m = k + 1;
				for (l = i; l < middle; l++) {
						vector[m] = aux_table[l];
						m++;
				}
				for (l = j; l < n_elements; l++) {
						vector[m] = aux_table[l];
						m++;
				}

				/* Desbloqueamos la señal SIGALRM cuando ya no hay peligro de que el
				   ilustrador se encuentre datos inconsistentes. */
				if (sigprocmask(SIG_UNBLOCK, &set, NULL) < 0) {
						perror("sigprocmask");
						return ERROR;
				}
		}

		free((void *)aux_table);
		aux_table = NULL;
		return OK;
}

int get_number_parts(int level, int n_levels) {
		/* The number of parts is 2^(n_levels - 1 - level). */
		return 1 << (n_levels - 1 - level);
}

Sort *init_sort(char *file_name, int n_levels, int n_processes, int delay) {
		char string[MAX_STRING];
		FILE *file = NULL;
		int i, j, log_data, fd_shm;
		int block_size, modulus;

		if (!file_name) {
				fprintf(stderr, "init_sort - Incorrect arguments\n");
				return NULL;
		}


		/* Creamos la zona de memoria compartida, truncandola a 0 Bytes si ya existe.
		   Ya podemos hacer shm_unlink ya que ningun proceso mas (que no sea hijo y por
		   tanto la herede) la va a abrir. */
		fd_shm = shm_open (SHM_NAME, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
		if (fd_shm == -1) {
				fprintf(stderr, "Error creating the shared memory segment\n");
				return NULL;
		}
		shm_unlink(SHM_NAME);


		/* Le damos a la zona de memoria comparida el tamaño adecuado
		   para almacenar nuestra estructura. */
		if (ftruncate(fd_shm, sizeof(Sort)) == -1) {
				fprintf(stderr, "Error resizing the shared memory segment\n");
				close(fd_shm);
				return NULL;
		}

		/* Añadimos la memoria compartida a nuestro espacio de direcciones virtuales. */
		sort = mmap(NULL, sizeof(*sort), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
		if (sort == MAP_FAILED) {
				fprintf(stderr, "Error mapping the shared memory segment\n");
				close(fd_shm);
				return NULL;
		}
		close(fd_shm);


		/* At most MAX_LEVELS levels. */
		sort->n_levels = MAX(1, MIN(n_levels, MAX_LEVELS));
		/* At most MAX_PARTS processes can work together. */
		sort->n_processes = MAX(1, MIN(n_processes, MAX_PARTS));
		/* The main process PID is stored. */
		sort->ppid = getpid();
		/* Delay for the algorithm in ns (less than 1s). */
		sort->delay = MAX(1, MIN(999999999, delay));

		if (!(file = fopen(file_name, "r"))) {
				perror("init_sort - fopen");
				munmap(sort, sizeof(*sort));
				return NULL;
		}

		/* The first line contains the size of the data, truncated to MAX_DATA. */
		if (!(fgets(string, MAX_STRING, file))) {
				fprintf(stderr, "init_sort - Error reading file\n");
				fclose(file);
				munmap(sort, sizeof(*sort));
				return NULL;
		}
		sort->n_elements = atoi(string);
		if (sort->n_elements > MAX_DATA) {
				sort->n_elements = MAX_DATA;
		}

		/* The remaining lines contains one integer number each. */
		for (i = 0; i < sort->n_elements; i++) {
				if (!(fgets(string, MAX_STRING, file))) {
						fprintf(stderr, "init_sort - Error reading file\n");
						fclose(file);
						munmap(sort, sizeof(*sort));
						return NULL;
				}
				sort->data[i] = atoi(string);
		}
		fclose(file);

		/* Each task should have at least one element. */
		log_data = compute_log(sort->n_elements);
		if (n_levels > log_data) {
				n_levels = log_data;
		}
		sort->n_levels = n_levels;

		/* The data is divided between the tasks, which are also initialized. */
		block_size = sort->n_elements / get_number_parts(0, sort->n_levels);
		modulus = sort->n_elements % get_number_parts(0, sort->n_levels);
		sort->tasks[0][0].completed = INCOMPLETE;
		sort->tasks[0][0].ini = 0;
		sort->tasks[0][0].end = block_size + (modulus > 0);
		sort->tasks[0][0].mid = NO_MID;
		for (j = 1; j < get_number_parts(0, sort->n_levels); j++) {
				sort->tasks[0][j].completed = INCOMPLETE;
				sort->tasks[0][j].ini = sort->tasks[0][j - 1].end;
				sort->tasks[0][j].end = sort->tasks[0][j].ini \
				                        + block_size + (modulus > j);
				sort->tasks[0][j].mid = NO_MID;
		}
		for (i = 1; i < n_levels; i++) {
				for (j = 0; j < get_number_parts(i, sort->n_levels); j++) {
						sort->tasks[i][j].completed = INCOMPLETE;
						sort->tasks[i][j].ini = sort->tasks[i - 1][2 * j].ini;
						sort->tasks[i][j].mid = sort->tasks[i - 1][2 * j].end;
						sort->tasks[i][j].end = sort->tasks[i - 1][2 * j + 1].end;
				}
		}

		/* Creamos la cola de mensajes y como ningun proceso que no sea hijo la va
		   a abrir podemos borrar su nombre. */
		struct mq_attr attributes = {.mq_flags = 0, .mq_maxmsg = 10, .mq_curmsgs = 0, .mq_msgsize = 2*sizeof(int)};
		sort->queue = mq_open(MESSAGE_QUEUE, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attributes);
		if (sort->queue == (mqd_t)-1) {
				perror("mq_open");
				if(munmap(sort, sizeof(Sort)) == -1) perror("munmap");
				return NULL;
		}
		if(mq_unlink(MESSAGE_QUEUE) != 0) {
				perror("mq_unlink");
				if(mq_close(sort->queue) != 0) perror("mq_close");
				if(munmap(sort, sizeof(Sort)) == -1) perror("munmap");
				return NULL;
		}

		/* Abrimos todas las tuberias necesarias. */
		for (j = 0; j < sort->n_processes; j++) {
				if(pipe(sort->tuberias[j].lee_trabajador) == -1) {
						perror("pipe");
						for(int i = 0; i < j; i++) {
								if(close(sort->tuberias[i].lee_trabajador[0]) != 0) perror("close");
								if(close(sort->tuberias[i].lee_trabajador[1]) != 0) perror("close");
								if(close(sort->tuberias[i].lee_ilustrador[0]) != 0) perror("close");
								if(close(sort->tuberias[i].lee_ilustrador[1]) != 0) perror("close");
						}
						if(mq_close(sort->queue) != 0) perror("mq_close");
						if(munmap(sort, sizeof(Sort)) == -1) perror("munmap");
						return NULL;
				}
				if(pipe(sort->tuberias[j].lee_ilustrador) == -1) {
						perror("pipe");
						for(int i = 0; i < j; i++) {
								if(close(sort->tuberias[i].lee_trabajador[0]) != 0) perror("close");
								if(close(sort->tuberias[i].lee_trabajador[1]) != 0) perror("close");
								if(close(sort->tuberias[i].lee_ilustrador[0]) != 0) perror("close");
								if(close(sort->tuberias[i].lee_ilustrador[1]) != 0) perror("close");
						}
						if(close(sort->tuberias[j].lee_trabajador[0]) != 0) perror("close");
						if(close(sort->tuberias[j].lee_trabajador[1]) != 0) perror("close");
						if(mq_close(sort->queue) != 0) perror("mq_close");
						if(munmap(sort, sizeof(Sort)) == -1) perror("munmap");
						return NULL;
				}
		}

		/* Inicializamos tantos semaforos sin nombre como partes en el primer nivel
		   para proteger cada tarea del trabajo (se van reutilizandoo en cada nivel). */
		int n_sem = get_number_parts(0, sort->n_levels);
		for(i = 0; i < n_sem; i++) {
				if (sem_init(&(sort->protege_tasks[i]), 1, 1) == -1) {
						perror("sem_init");
						for(int i = 0; i < sort->n_processes; i++) {
								if(close(sort->tuberias[i].lee_trabajador[0]) != 0) perror("close");
								if(close(sort->tuberias[i].lee_trabajador[1]) != 0) perror("close");
								if(close(sort->tuberias[i].lee_ilustrador[0]) != 0) perror("close");
								if(close(sort->tuberias[i].lee_ilustrador[1]) != 0) perror("close");
						}
						for(int z = 0; z < i; z++) if(sem_destroy(&(sort->protege_tasks[z])) != 0) perror("sem_destroy");
						if(mq_close(sort->queue) != 0) perror("mq_close");
						if(munmap(sort, sizeof(Sort)) == -1) perror("munmap");
						return NULL;
				}
		}

		/* Cuando creemos los hijos se guardaran sus pids pero mientras se inicializan a
		   -1 para evitar posibles errores al recibir SIGINT. */
		for(i = 0; i < MAX_PARTS; i++) {
				sort->pids[i] = -1;
		}

		return sort;
}

int get_associated_semaphore(int level, int part){
		if(level < 0 || part < 0 || level > MAX_LEVELS || part > MAX_PARTS) return -1;
		if(level == 0) return part;
		/* El semaforo asociado a cada tarea es 2^(nivel-1)*parte */
		return (1 << (level - 1)) * part;
}

Bool check_task_completed(int level, int part){
		int sem = get_associated_semaphore(level, part);
		if(sem_wait(&sort->protege_tasks[sem]) != 0) {
				perror("sem_wait");
				clean_up();
				exit(EXIT_FAILURE);
		}
		if(sort->tasks[level][part].completed == COMPLETED) {
				sem_post(&sort->protege_tasks[sem]);
				return TRUE;
		}
		sem_post(&sort->protege_tasks[sem]);
		return FALSE;
}

Bool check_task_ready(int level, int part) {
		if (!(sort)) {
				return FALSE;
		}

		if ((level < 0) || (level >= sort->n_levels) \
		    || (part < 0) || (part >= get_number_parts(level, sort->n_levels))) {
				return FALSE;
		}

		int sem = get_associated_semaphore(level, part);
		if(sem_wait(&sort->protege_tasks[sem]) != 0) {
				perror("sem_wait");
				clean_up();
				exit(EXIT_FAILURE);
		}
		if (sort->tasks[level][part].completed != INCOMPLETE) {
				sem_post(&sort->protege_tasks[sem]);
				return FALSE;
		}
		sem_post(&sort->protege_tasks[sem]);


		/* The tasks of the first level are always ready. */
		if (level == 0) {
				return TRUE;
		}

		/* Other tasks depend on the hierarchy. */
		sem = get_associated_semaphore(level-1, 2*part);
		if(sem_wait(&sort->protege_tasks[sem]) != 0) {
				perror("sem_wait");
				clean_up();
				exit(EXIT_FAILURE);
		}
		if (sort->tasks[level - 1][2 * part].completed != COMPLETED) {
				sem_post(&sort->protege_tasks[sem]);
				return FALSE;
		}
		sem_post(&sort->protege_tasks[sem]);

		sem = get_associated_semaphore(level-1, 2*part + 1);
		if(sem_wait(&sort->protege_tasks[sem]) != 0) {
				perror("sem_wait");
				clean_up();
				exit(EXIT_FAILURE);
		}
		if (sort->tasks[level - 1][2 * part + 1].completed != COMPLETED) {
				sem_post(&sort->protege_tasks[sem]);
				return FALSE;
		}
		sem_post(&sort->protege_tasks[sem]);

		return TRUE;
}

Status solve_task(Sort *sort, int level, int part) {
		/* In the first level, bubble-sort. */
		if (sort->tasks[level][part].mid == NO_MID) {
				return bubble_sort( \
						sort->data + sort->tasks[level][part].ini, \
						sort->tasks[level][part].end - sort->tasks[level][part].ini, \
						sort->delay);
		}
		/* In other levels, merge. */
		else {
				return merge( \
						sort->data + sort->tasks[level][part].ini, \
						sort->tasks[level][part].mid - sort->tasks[level][part].ini, \
						sort->tasks[level][part].end - sort->tasks[level][part].ini, \
						sort->delay);
		}
}


void clean_up_trabajador() {
		if(mq_close(sort->queue) != 0) perror("mq_close");
		if(close(sort->tuberias[num_trabajador].lee_trabajador[0]) != 0) perror("close");
		if(close(sort->tuberias[num_trabajador].lee_ilustrador[1]) != 0) perror("close");
		if(munmap(sort, sizeof(Sort)) == -1) perror("munmap");
}


void trabajador(int n_trabajador){
		int message[2], received = 0, aux = 0, j;
		sigset_t set;

		/* Bloqueamos la señal SIGTERM temporalmente para prepararnos para su
		   posible recepcion. */
		sigemptyset(&set);
		sigaddset(&set, SIGTERM);
		if (sigprocmask(SIG_BLOCK, &set, NULL) < 0) {
				perror("sigprocmask");
				aux = 1;
		}

		/* Establecemos nuestro propio manejador de señales SIGTERM y SIGALRM. */
		struct sigaction act;
		act.sa_handler = manejador;
		sigemptyset(&(act.sa_mask));
		act.sa_flags = 0;
		if (sigaction(SIGTERM, &act, NULL) < 0) {
				perror("sigaction");
				aux = 1;
		}
		if (sigaction(SIGALRM, &act, NULL) < 0) {
				perror("sigaction");
				aux = 1;
		}

		num_trabajador = (sig_atomic_t)n_trabajador;

		/* Cerramos los descriptores de fichero que no debemos usar. */
		for (j = 0; j < sort->n_processes; j++) {
				if(j != n_trabajador) {
						if(close(sort->tuberias[j].lee_trabajador[0]) != 0) {
								perror("close");
								aux = 1;
						}
						if(close(sort->tuberias[j].lee_trabajador[1]) != 0) {
								perror("close");
								aux = 1;
						}
						if(close(sort->tuberias[j].lee_ilustrador[0]) != 0) {
								perror("close");
								aux = 1;
						}
						if(close(sort->tuberias[j].lee_ilustrador[1]) != 0) {
								perror("close");
								aux = 1;
						}
				}
		}
		if(close(sort->tuberias[n_trabajador].lee_trabajador[1]) != 0) {
				aux = 1;
				perror("close");
		}
		if(close(sort->tuberias[n_trabajador].lee_ilustrador[0]) != 0) {
				aux = 1;
				perror("close");
		}
		if(aux == 1) {
				clean_up_trabajador();
				exit(EXIT_FAILURE);
		}

		/* Ya tenemos todas las variables usadas en el manejador preparadas para
		   la posible rececpcion de SIGTERM. */
		if (sigprocmask(SIG_UNBLOCK, &set, NULL) < 0) {
				perror("sigprocmask");
				clean_up_trabajador();
				exit(EXIT_FAILURE);
		}

		/* Dejamos la mascara preparada para bloquear SIGALRM antes del loop. */
		sigemptyset(&set);
		sigaddset(&set, SIGALRM);

		/* Establecemos la primera alarma, las demas se establecen en el propio
		   manejador de la alarma. */
		if (alarm(SECS)) {
				fprintf(stderr, "Existe una alarma previa establecida\n");
		}

		/* Comienza el verdadero trabajo de los hijos. */
		int associated_sem = -1;
		while(1) {
				/* Bloqueamos la señal SIGALRM para modificar las variables globlales
				   que se usaran de argumento para informar al ilustrador del trabajo
				   que esta realizando este hijo en el manejador de SIGALRM. */
				if (sigprocmask(SIG_BLOCK, &set, NULL) < 0) {
						perror("sigprocmask");
						clean_up_trabajador();
						exit(EXIT_FAILURE);
				}
				pipe_message[0] = (sig_atomic_t)-1;
				pipe_message[1] = (sig_atomic_t)-1;
				if (sigprocmask(SIG_UNBLOCK, &set, NULL) < 0) {
						perror("sigprocmask");
						clean_up_trabajador();
						exit(EXIT_FAILURE);
				}

				/* Esperamos la llegada de un mensaje teniendo cuidado con las posibles
				   interrupciones de la alarma. */
				do {
						received = mq_receive(sort->queue, (char*) message, 2*sizeof(int), NULL);
						if(received != 2*sizeof(int)) {
								if(errno != EINTR) {
										perror("mq_receive");
										clean_up_trabajador();
										exit(EXIT_FAILURE);
								}
						}
				} while(received != 2*sizeof(int));

				associated_sem = get_associated_semaphore(message[0], message[1]);
				if (associated_sem == -1) {
						fprintf(stderr, "Error when getting the associated semaphore.\n");
						clean_up_trabajador();
						exit(EXIT_FAILURE);
				}

				/* Volvemos a bloquear SIGALRM momentaneamente para actualizar la
				   informacion que le llegara al ilustrador. */
				if (sigprocmask(SIG_BLOCK, &set, NULL) < 0) {
						perror("sigprocmask");
						clean_up_trabajador();
						exit(EXIT_FAILURE);
				}
				pipe_message[0] = (sig_atomic_t)message[0];
				pipe_message[1] = (sig_atomic_t)message[1];
				if (sigprocmask(SIG_UNBLOCK, &set, NULL) < 0) {
						perror("sigprocmask");
						clean_up_trabajador();
						exit(EXIT_FAILURE);
				}

				/* Cambiamos el estado de la tarea, teniendo cuidado con los posibles
				   problemas de concuerrencia y de llegadas de señales inoportunas. */
				do {
						aux = sem_wait(&sort->protege_tasks[associated_sem]);
						if(aux != 0) {
								if(errno != EINTR) {
										perror("sem_wait");
										clean_up_trabajador();
										exit(EXIT_FAILURE);
								}
						}
				} while(aux != 0);
				sort->tasks[message[0]][message[1]].completed = PROCESSING;
				sem_post(&sort->protege_tasks[associated_sem]);

				/* Reolvemos la tarea. */
				if(solve_task(sort, message[0], message[1]) == ERROR) {
						fprintf(stderr, "Error in solve_task function. \n");
						clean_up_trabajador();
						exit(EXIT_FAILURE);
				}

				/* Cambiamos el estado de la tarea, teniendo cuidado con los posibles
				   problemas de concuerrencia y de llegadas de señales inoportunas. */
				do {
						aux = sem_wait(&sort->protege_tasks[associated_sem]);
						if(aux != 0) {
								if(errno != EINTR) {
										perror("sem_wait");
										clean_up_trabajador();
										exit(EXIT_FAILURE);
								}
						}
				} while(aux != 0);
				sort->tasks[message[0]][message[1]].completed = COMPLETED;
				sem_post(&sort->protege_tasks[associated_sem]);

				/* Enviamos la señal SIGUSR1 al padre para avisar de que hemos finalizado la tarea. */
				if(kill(sort->ppid, SIGUSR1) == -1) {
						clean_up_trabajador();
						perror("kill");
						exit(EXIT_FAILURE);
				}
		}
}


void clean_up_ilustrador(){
		for(int i = 0; i < sort->n_processes; i++) {
				if(close(sort->tuberias[i].lee_trabajador[1]) != 0) perror("close");
				if(close(sort->tuberias[i].lee_ilustrador[0]) != 0) perror("close");
		}
		if(munmap(sort, sizeof(Sort)) == -1) perror("munmap");
}


void ilustrador(){
		int i = 0, aux = 0;
		sigset_t set;

		/* Bloqueamos la señal SIGTERM temporalmente para prepararnos para su
		   posible recepcion. */
		sigemptyset(&set);
		sigaddset(&set, SIGTERM);
		if (sigprocmask(SIG_BLOCK, &set, NULL) < 0) {
				perror("sigprocmask");
				aux = 1;
		}

		/* El ilustrador sera el tabajador -1.  */
		num_trabajador = (sig_atomic_t)-1;

		/* Cerramos los descriptores de fichero que no debemos usar. */
		for(i = 0; i < sort->n_processes; i++) {
				if(close(sort->tuberias[i].lee_ilustrador[1]) != 0) {
						perror("close");
						aux = 1;
				}
				if(close(sort->tuberias[i].lee_trabajador[0]) != 0) {
						perror("close");
						aux = 1;
				}
		}
		if(aux == 1) {
				clean_up_ilustrador();
				exit(EXIT_FAILURE);
		}

		/* Establecemos nuestro propio manejador de señales SIGTERM. */
		struct sigaction act;
		act.sa_handler = manejador;
		sigemptyset(&(act.sa_mask));
		act.sa_flags = 0;
		if (sigaction(SIGTERM, &act, NULL) < 0) {
				perror("sigaction");
				clean_up_ilustrador();
				exit(EXIT_FAILURE);
		}

		/* Como ya esta todo preparado para que el manejador de SIGTERM funcione
		   correctamente, podemos desbloquear la señal. */
		if (sigprocmask(SIG_UNBLOCK, &set, NULL) < 0) {
				perror("sigprocmask");
				clean_up_ilustrador();
				exit(EXIT_FAILURE);
		}


		/* Comienza el trabajo real del ilustrador. */
		int one = 1, received_info[sort->n_processes][2];
		ssize_t nbytes;
		while(1) {
				/* Leemos el estado de cada hijo. */
				for(i = 0; i < sort->n_processes; i++) {
						do {
								nbytes = read(sort->tuberias[i].lee_ilustrador[0], received_info[i], 2*sizeof(int));
								if(nbytes == -1) {
										perror ("read");
										clean_up_ilustrador();
										exit(EXIT_FAILURE);
								}
						} while (nbytes != 8);
				}

				/* Mostramos el estado del sistema y la informacion de cada trabajador. */
				plot_vector(sort->data, sort->n_elements);
				printf("\n\n\nWORKERS ACTIVITY: \n\n");
				for(i = 0; i < sort->n_processes; i++) {
						if((received_info[i][0] != -1) && (received_info[i][1] != -1)) {
								printf("Worker number %d is working on level %d, part %d.\n", i+1,
								       received_info[i][0], received_info[i][1]);
						} else {
								printf("Worker number %d is currently waiting to receive a task.\n", i+1);
						}
				}

				/* Enviamos el mensaje de respuesta a los trabajadores, un simple 1. */
				for(i = 0; i < sort->n_processes; i++) {
						if(write(sort->tuberias[i].lee_trabajador[1], &one, sizeof(int)) == -1) {
								perror ("write");
								clean_up_ilustrador();
								exit(EXIT_FAILURE);
						}
				}
		}
}


void clean_up(){
		int n_sem = get_number_parts(0, sort->n_levels);
		for(int z = 0; z <= sort->n_processes; z++) if(kill(sort->pids[z], SIGTERM) == -1) perror("kill");
		if(waitpid(-1, NULL, 0) == -1) fprintf(stderr, "Error on waitpid.\n");
		for(int z = 0; z < n_sem; z++) if(sem_destroy(&(sort->protege_tasks[z])) != 0) perror("sem_destroy");
		if(mq_close(sort->queue) != 0) perror("mq_close");
		if(munmap(sort, sizeof(Sort)) == -1) perror("munmap");
}


Status sort_multiple_processes(char *file_name, int n_levels, int n_processes, int delay) {
		int i = 0, j = 0, k = 0, n_parts = 0;
		sigset_t set;

		/* Establecemos el manejador de las señales SIGINT y SIGUSR1. */
		struct sigaction act;
		act.sa_handler = manejador;
		sigemptyset(&(act.sa_mask));
		act.sa_flags = 0;
		if (sigaction(SIGUSR1, &act, NULL) < 0) {
				perror("sigaction");
				return ERROR;
		}
		/* Bloqueamos la señal SIGUSR1 en el manejador de SIGINT. */
		sigaddset(&(act.sa_mask), SIGUSR1);
		if (sigaction(SIGINT, &act, NULL) < 0) {
				perror("sigaction");
				return ERROR;
		}


		/* Bloqueamos la señal SIGINT momentaneamente para inicializar correctamente
		   la memoria compartida y que no haya problemas si llega SIGINT. */
		sigemptyset(&set);
		sigaddset(&set, SIGINT);
		if (sigprocmask(SIG_BLOCK, &set, NULL) < 0) {
				perror("sigprocmask");
				clean_up();
				return ERROR;
		}

		/* The data is loaded and the structure initialized. */
		sort = init_sort(file_name, n_levels, n_processes, delay);
		if (sort == NULL) {
				fprintf(stderr, "sort_multiple_processes - init_sort\n");
				return ERROR;
		}

		if (sigprocmask(SIG_UNBLOCK, &set, NULL) < 0) {
				perror("sigprocmask");
				clean_up();
				return ERROR;
		}


		/* Bloquemaos la señal SIGUSR1 para que solo pueda ser recibida cuando se
		   desbloquea en el sigsuspend y ya dejamos creada la mascara adecuada. */
		sigemptyset(&set);
		sigaddset(&set, SIGUSR1);
		if (sigprocmask(SIG_BLOCK, &set, NULL) < 0) {
				perror("sigprocmask");
				munmap(sort, sizeof(Sort));
				return ERROR;
		}
		sigset_t sigsuspendmask;
		sigfillset(&sigsuspendmask);
		sigdelset(&sigsuspendmask, SIGUSR1);
		sigdelset(&sigsuspendmask, SIGINT);


		/* Bloqueamos la señal SIGINT nuevamente para rellenar el campo pids con
		   los pids de los procesos que vamos creando. */
		sigemptyset(&set);
		sigaddset(&set, SIGINT);
		if (sigprocmask(SIG_BLOCK, &set, NULL) < 0) {
				perror("sigprocmask");
				clean_up();
				return ERROR;
		}

		/* Creamos al ilustrador. */
		pid_t pid = 0;
		pid = fork();
		if(pid <  0) {
				perror("fork");
				clean_up();
				return ERROR;
		} else if(pid == 0) {
				ilustrador();
		}
		sort->pids[0] = pid;

		/* Creamos los trabajadores. */
		for (j = 0; j < sort->n_processes; j++) {
				pid = fork();
				if(pid <  0) {
						perror("fork");
						clean_up();
						return ERROR;
				} else if(pid == 0) {
						trabajador(j);
				} else {
						sort->pids[j+1] = pid;
				}
		}

		/* Cerramos las tuberias que no vamos a usar en el proceso principal. */
		for (j = 0; j < sort->n_processes; j++) {
				if(close(sort->tuberias[j].lee_trabajador[0]) != 0) {
						perror("close");
						clean_up();
						return ERROR;
				}
				if(close(sort->tuberias[j].lee_trabajador[1]) != 0) {
						perror("close");
						clean_up();
						return ERROR;
				}
				if(close(sort->tuberias[j].lee_ilustrador[0]) != 0) {
						perror("close");
						clean_up();
						return ERROR;
				}
				if(close(sort->tuberias[j].lee_ilustrador[1]) != 0) {
						perror("close");
						clean_up();
						return ERROR;
				}
		}

		if (sigprocmask(SIG_UNBLOCK, &set, NULL) < 0) {
				perror("sigprocmask");
				clean_up();
				return ERROR;
		}


		plot_vector(sort->data, sort->n_elements);
		printf("\nStarting algorithm with %d levels and %d processes...\n", sort->n_levels, sort->n_processes);

		/* For each level, and each part, the corresponding task is solved. */
		int message[2] = {0, 0};
		for (i = 0; i < sort->n_levels; i++) {
				n_parts = get_number_parts(i, sort->n_levels);

				/* Introducimos en la cola de mensajes un array de dos enteros indicando
				   la tarea que ha de realizarse. */
				message[0] = i;
				for (j = 0; j < n_parts; j++) {
						message[1] = j;
						sort->tasks[i][j].completed = SENT;
						if (mq_send(sort->queue, (char*) message, 2*sizeof(int), 1) == -1) {
								perror("mq_send");
								clean_up();
								return ERROR;
						}
				}

				/* Comprobamos si los trabajadores han terminado sus tareas. */
				while(k != n_parts) {
						/* Esperamos a recibir una señal SIGUSR1 por parte de los hijos (o SIGINT). */
						sigsuspend(&sigsuspendmask);

						/* Comprobamos si todas las tareas del nivel estan acabadas. */
						for(k = 0; k < n_parts; k++) {
								if(check_task_completed(i, k) != TRUE) break;
						}
				}
		}

		/* Imprimimos el resultado final del array y acabamos correctamente. */
		plot_vector(sort->data, sort->n_elements);
		printf("\nAlgorithm completed\n");

		clean_up();
		return OK;
}

Status sort_multiple_processes_improved(char *file_name, int n_levels, int n_processes, int delay) {
		int i = 0, j = 0, n_parts = 0;
		sigset_t set;

		/* Establecemos el manejador de las señales SIGINT y SIGUSR1. */
		struct sigaction act;
		act.sa_handler = manejador;
		sigemptyset(&(act.sa_mask));
		act.sa_flags = 0;
		if (sigaction(SIGUSR1, &act, NULL) < 0) {
				perror("sigaction");
				return ERROR;
		}
		/* Bloqueamos la señal SIGUSR1 en el manejador de SIGINT. */
		sigaddset(&(act.sa_mask), SIGUSR1);
		if (sigaction(SIGINT, &act, NULL) < 0) {
				perror("sigaction");
				return ERROR;
		}


		/* Bloqueamos la señal SIGINT momentaneamente para inicializar correctamente
		   la memoria compartida y que no haya problemas si llega SIGINT. */
		sigemptyset(&set);
		sigaddset(&set, SIGINT);
		if (sigprocmask(SIG_BLOCK, &set, NULL) < 0) {
				perror("sigprocmask");
				clean_up();
				return ERROR;
		}

		/* The data is loaded and the structure initialized. */
		sort = init_sort(file_name, n_levels, n_processes, delay);
		if (sort == NULL) {
				fprintf(stderr, "sort_multiple_processes - init_sort\n");
				return ERROR;
		}

		if (sigprocmask(SIG_UNBLOCK, &set, NULL) < 0) {
				perror("sigprocmask");
				clean_up();
				return ERROR;
		}


		/* Bloquemaos la señal SIGUSR1 para que solo pueda ser recibida cuando se
		   desbloquea en el sigsuspend y ya dejamos creada la mascara adecuada. */
		sigemptyset(&set);
		sigaddset(&set, SIGUSR1);
		if (sigprocmask(SIG_BLOCK, &set, NULL) < 0) {
				perror("sigprocmask");
				munmap(sort, sizeof(Sort));
				return ERROR;
		}
		sigset_t sigsuspendmask;
		sigfillset(&sigsuspendmask);
		sigdelset(&sigsuspendmask, SIGUSR1);
		sigdelset(&sigsuspendmask, SIGINT);


		/* Bloqueamos la señal SIGINT nuevamente para rellenar el campo pids con
		   los pids de los procesos que vamos creando. */
		sigemptyset(&set);
		sigaddset(&set, SIGINT);
		if (sigprocmask(SIG_BLOCK, &set, NULL) < 0) {
				perror("sigprocmask");
				clean_up();
				return ERROR;
		}

		/* Creamos al ilustrador. */
		pid_t pid = 0;
		pid = fork();
		if(pid <  0) {
				perror("fork");
				clean_up();
				return ERROR;
		} else if(pid == 0) {
				ilustrador();
		}
		sort->pids[0] = pid;

		/* Creamos los trabajadores. */
		for (j = 0; j < sort->n_processes; j++) {
				pid = fork();
				if(pid <  0) {
						perror("fork");
						clean_up();
						return ERROR;
				} else if(pid == 0) {
						trabajador(j);
				} else {
						sort->pids[j+1] = pid;
				}
		}

		/* Cerramos las tuberias que no vamos a usar en el proceso principal. */
		for (j = 0; j < sort->n_processes; j++) {
				if(close(sort->tuberias[j].lee_trabajador[0]) != 0) {
						perror("close");
						clean_up();
						return ERROR;
				}
				if(close(sort->tuberias[j].lee_trabajador[1]) != 0) {
						perror("close");
						clean_up();
						return ERROR;
				}
				if(close(sort->tuberias[j].lee_ilustrador[0]) != 0) {
						perror("close");
						clean_up();
						return ERROR;
				}
				if(close(sort->tuberias[j].lee_ilustrador[1]) != 0) {
						perror("close");
						clean_up();
						return ERROR;
				}
		}

		if (sigprocmask(SIG_UNBLOCK, &set, NULL) < 0) {
				perror("sigprocmask");
				clean_up();
				return ERROR;
		}


		plot_vector(sort->data, sort->n_elements);
		printf("\nStarting algorithm with %d levels and %d processes...\n", sort->n_levels, sort->n_processes);

		/* For each level, and each part, the corresponding task is solved. */
		int message[2] = {0, 0}, count = 0, completed = 0;
		while(1) {
				count = 0;
				completed = 0;
				for (i = 0; i < sort->n_levels; i++) {
						n_parts = get_number_parts(i, sort->n_levels);
						count += n_parts;
						message[0] = i;
						for (j = 0; j < n_parts; j++) {
								if(check_task_ready(i, j) == TRUE) {
										message[1] = j;
										/* Si la tarea esta lista es que ningun trabajador la esta modificando. */
										sort->tasks[i][j].completed = SENT;
										if (mq_send(sort->queue, (char*) message, 2*sizeof(int), 1) == -1) {
												perror("mq_send");
												clean_up();
												return ERROR;
										}
								} else if(check_task_completed(i, j) == TRUE) {
										completed++;
								}
						}
				}
				if(count == completed) break;

				sigsuspend(&sigsuspendmask);
		}

		/* Imprimimos el resultado final del array y acabamos correctamente. */
		plot_vector(sort->data, sort->n_elements);
		printf("\nAlgorithm completed\n");

		clean_up();
		return OK;
}

Status sort_multiple_processes_timed(char *file_name, int n_levels, int n_processes, int delay, FILE *f) {
		struct timeval t_ini, t_fin;

		gettimeofday(&t_ini, NULL);
		if(sort_multiple_processes(file_name, n_levels, n_processes, delay) == ERROR) {
				fprintf(stderr, "FUNCTION FAILED, press any key to continue.\n");
				getchar();
		}
		gettimeofday(&t_fin, NULL);

		double milsec = ((t_fin.tv_sec - t_ini.tv_sec)*1000000L + t_fin.tv_usec - t_ini.tv_usec)/1000.0;

		fprintf(f, "%d %lf\n", n_processes, milsec);
		printf("%lf milliseconds\n", milsec);

		exit(EXIT_SUCCESS);
}
