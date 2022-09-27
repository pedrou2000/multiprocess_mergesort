SRC=./
INC=./
OBJ=./

CC=gcc
CFLAGS=-Wall -g -I $(INC)
LIBRARIES=-lrt -pthread

ARG_FILE=./Data/DataSmall.dat
ARG_N_LEVELS=5
ARG_N_PROCESSES=10
ARG_DELAY=100

.PHONY: clean_objects clean_program clean_doc clean run runv doc

##############################################

all: sort gnu sorti

sort: $(OBJ)/main.o $(OBJ)/sort.o $(OBJ)/utils.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBRARIES)

sorti: $(OBJ)/main_optional_part.o $(OBJ)/sort.o $(OBJ)/utils.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBRARIES)

gnu: $(OBJ)/gnu_plot_generator.o $(OBJ)/sort.o $(OBJ)/utils.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBRARIES)

##############################################

$(OBJ)/main.o: main.c sort.h global.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ)/main_optional_part.o: main_optional_part.c sort.h global.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ)/gnu_plot_generator.o: gnu_plot_generator.c sort.h global.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ)/sort.o: sort.c sort.h global.h utils.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ)/utils.o: utils.c utils.h global.h
	$(CC) $(CFLAGS) -c $< -o $@

##############################################

clean_objects:
	@echo "Cleaning objects..."
	@rm -f $(OBJ)/*.o

clean_program:
	@echo "Cleaning program..."
	@rm -f sort gnu sorti

clean: clean_objects clean_program

##############################################

run: sort
	@echo "Launching program..."
	@sleep 1
	@./sort $(ARG_FILE) $(ARG_N_LEVELS) $(ARG_N_PROCESSES) $(ARG_DELAY)

runv: sort
	@echo "Launching program with valgrind..."
	@sleep 1
	@valgrind --leak-check=full --show-leak-kinds=all ./sort $(ARG_FILE) $(ARG_N_LEVELS) $(ARG_N_PROCESSES) $(ARG_DELAY)

run_small: sort
	@./sort ./Data/DataSmall.dat 5 10 100

run_medium: sort
	@./sort ./Data/DataMedium.dat 10 15 15

run_large: sort
	@./sort ./Data/DataLarge.dat 10 30 15

runi: sorti
	@echo "Launching program (optional part)..."
	@sleep 1
	@./sorti $(ARG_FILE) $(ARG_N_LEVELS) $(ARG_N_PROCESSES) $(ARG_DELAY)

runvi: sorti
	@echo "Launching program (optional part) with valgrind..."
	@sleep 1
	@valgrind --leak-check=full --show-leak-kinds=all ./sorti $(ARG_FILE) $(ARG_N_LEVELS) $(ARG_N_PROCESSES) $(ARG_DELAY)

gnu_small: gnu
	@./gnu ./Data/DataMedium.dat results.txt 10 1 30 1
