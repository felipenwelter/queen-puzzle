# PPA - n-Queen Puzzle

The *n* queens puzzle is the problem of placing *n* chess queens on an *n x n* chessboard so that no two queens threaten each other; thus, a solution requires that no two queens share the same row, column, or diagonal. For more info visit [wikipedia](https://en.wikipedia.org/wiki/Eight_queens_puzzle).


## Objective:
- Find all the solutions for the problem and save in a .txt file.
- Parallelize the solution to get a faster answer, if possible < 10min for a 16x16 chessboard.

## Files:

- The main implementation uses the file *nqueen.c*.
- The *mp* file contains the configuration for the MPI, determining the IP addresses and the slot amount for each node in the network.
- The solutions are saved in the files named as *solutionx.txt*, with *x* representing the size (number of lines and rows) of the chessboard.

## Compile and execution:

To compile, run the command:
> `make`

To run the program in sequential mode, just type (notice that x is the size of the chessboard):
> `./nqueen <x>`

To run the program parallelized, type:
> `mpirun -np <thread_num> --hostfile mp nqueen <chessboard_size>`