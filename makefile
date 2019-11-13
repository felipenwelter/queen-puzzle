# GNU Makefile

CC = mpicc
CCFLAGS = -Wall -O3
LDFLAGS =
TARGET = nqueen help

all: $(TARGET)

%.o: %.c
	$(CC) $(CCFLAGS) -c $<

%: %.o
	$(CC) $(LDFLAGS) $^ -o $@

nqueen: queen-puzzle.c
			$(CC) $(CCFLAGS) queen-puzzle.c -o $@ $(LDFLAGS)

clean:
	rm -f *.o *~ $(TARGET)

help:
	@echo
	@echo
	@echo "####### Call examples #######"
	@echo "./nqueen 4"
	@echo "mpirun -np 8 --hostfile mp nqueen 8"