# GNU Makefile

CC = mpicc
CCFLAGS = -Wall -O3
LDFLAGS =
#LDFLAGS = -llmpe -lmpe
TARGET = queen help

all: $(TARGET)

%.o: %.c
	$(CC) $(CCFLAGS) -c $<

%: %.o
	$(CC) $(LDFLAGS) $^ -o $@

queen: queen-puzzle.c
			$(CC) $(CCFLAGS) queen-puzzle.c -o $@ $(LDFLAGS)


clean:
	rm -f *.o *~ $(TARGET)

help:
	@echo
	@echo
	@echo "####### Exemplo de Execução #######"
	@echo "./queen 4" para executar sequencial
	@echo "mpirun -np 8 --hostfile mp queen 8" para executar paralelo