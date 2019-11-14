/*****************************************************
 * Name: nqueens.c 
 * Author: Felipe Nathan Welter
 * Date: 13/11/2019
 *
 * Usage: mpirun -np <num processors> nqueens <n>
 * Description: Program that solves the problem of placing
 *  n chess queen on an n x n chessboard so that no two
 *  queens threaten each other. The solution is saved in
 *  a txt file, and the program is parallelized.
 *  The factoring logic is based on a program of Lucas
 *  Carpenter available on github as /lukas783.
 *
 *****************************************************/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>

static int tag_answer_count = 0;
static int tag_answer_int = 1;

typedef unsigned long long ull;

// Prototype declarations
int nqueens(int proc, ull i, ull n, int gs, int over);
ull factorial(ull n);
int formatMessage(int n, int *perm, char *msg);
int findPosition(int pos, int line, int size);

/**
 * int main(int argc, char*argv[])
 * Main function that control the threads that
*  calculates the possible answers.
 **/
int main(int argc, char*argv[]) {

  // declare variables
  ull i = 0 , n = 0, total = 0, subtot = 0;
  int answer_count = 0;
  double elapsed_time;

  int rank, sze;    
  //MPI_Status status, status2;
  MPI_Request recv_request;
  MPI_Request recv_request2;

  char filename[100];
  FILE *fmat;

  // initialize mpi
  MPI_Init(&argc, &argv);
  
  // set rank and size of all procs
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &sze);

  //set start time
  //MPI_Barrier(MPI_COMM_WORLD);
  elapsed_time = -MPI_Wtime();

  if (sze == 1){
    printf("warning: the program may not work well with one thread\n");
  }

  // check if the program has correct arguments
  if(argc != 2) {
    if(rank == 0)
      printf("Invalid arguments.\nFormat should be ./nqueens <n>\n\n");
  } else {

    // get the size of the chessboard
    n = atoi(argv[1]);

    // find the max size for the loop below
    ull max = factorial(n);

    // calculate the amount of calculation for each thread
    int group_size = max/(sze); 

    //send the leftover to the last thread
    int over = (sze == (rank+1) ? (max % sze) : 0);
    
    if (rank == 0){

      // set the file name and delete previous file
      sprintf(filename, "solution%lld.txt",n);
      remove(filename);
    }
    
    // call threads sending the amount of possibilities to calculate, get number of solutions found
    subtot += nqueens(rank, i, n, group_size, over);
    //MPI_Barrier(MPI_COMM_WORLD);
  
    // the master thread receives all the results and save in file
    if (rank == 0){

      for (int y = 0; y < sze; y++){
        // wait for the 1st response, with the number of solutions found
        MPI_Irecv(&answer_count,1,MPI_INT,y,tag_answer_count,MPI_COMM_WORLD,&recv_request);
        MPI_Wait(&recv_request, MPI_STATUS_IGNORE);
        //printf("[%d] from thread %d received the amount of %d\n", rank, y, answer_count);

        // if the thread found more than one valid solution, then format and append in file
        if (answer_count > 0){
          
          // array to allocate the valid positions of each solution, in sequence (position of each line from 0 to n)
          int *answer_int = (int*) calloc( answer_count * n,sizeof(int));
          
          // wait to receive all the answers as an integer array
          //MPI_Recv(&(answer_int[0]),(answer_count*n),MPI_INT,y,tag_answer_int,MPI_COMM_WORLD, &status2);
          MPI_Irecv(&(answer_int[0]),(answer_count*n),MPI_INT,y,tag_answer_int,MPI_COMM_WORLD,&recv_request2);
          MPI_Wait(&recv_request2, MPI_STATUS_IGNORE);

          //append the solutions sent by the thread in the end of file
          fmat = fopen(filename, "a");
          int position = 0;
          int index = 0;

          for (int t = 0; t < (answer_count * n); t++){
            position = findPosition(answer_int[t], t-(index*n), n);
            fprintf(fmat,"%d;", position);
            
            //skip line in the file for each group of positions that represents a unique solution
            if (( (t+1) % n) == 0) {
              fprintf(fmat,"\n");
              index++;
            }
          }

          fclose(fmat);
        }
      }
    }

    // reduce subtotal into grand total
    MPI_Reduce(&subtot, &total, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
  }

  //MPI_Barrier(MPI_COMM_WORLD);
  elapsed_time += MPI_Wtime();

  // print amount of solutions found and time
  if(rank == 0) {
    printf("Program executed in %.3f ms (%.2f sec ~ %.2f min)\n", 1000*elapsed_time, elapsed_time, elapsed_time/60);
    printf("Total number of solutions found: %llu\n\n", total);
    fflush(stdout);
  }

  //exit program
  MPI_Finalize();
  return 0;
}

/**
 * int nqueens(int proc, int*vals, ull i, ull n, int gs, int over )
 * find the permutations and validates if there is no queen in the
 * diagonal of the current queen. if there is, skip and test the 
 * next possibility. returns the amount of answers found.
 **/
int nqueens(int proc, ull i, ull n, int gs, int over) {

  // declare some variables and allocate array for factorial
  int a, b = 0;
  int answer_count = 0;
  int err = 0;
  int *fact = (int *)calloc(n, sizeof(int));

  MPI_Request send_request;
  MPI_Request send_request2;

  printf("[%d] thread started\n", proc);

  int *answer_list = (int*) calloc(gs+over,sizeof(int));

  int **perms = (int**) calloc(gs+over,sizeof(int *));
  for ( int t = 0; t < (gs+over); t++ ) {
    perms[t] = (int*) calloc (n, sizeof(int));
    if (perms[t] == NULL) {
      printf ("** Erro: Out of memory **\n");
    }
  }

  // calculate factorial based on i, store in perms
  fact[b] = 1;
  while(++b < (int)n) {
    fact[b] = fact[b-1]*b;
  }

  int lim = (gs+(gs*proc)) + over;
  for (int t = (proc*gs), p = 0; t < lim; t++, p++){ 
    i = t;

    for(b = 0; b < (int)n; ++b) {
      perms[p][b] = i / fact[n - 1 - b];
      i = i % fact[n - 1 - b];
    }

    for(b = n - 1; b > 0; --b) {
      for(a = b - 1; a >= 0; --a) {
        if(perms[p][a] <= perms[p][b]) {
          perms[p][b]++;
        }
      }
    }

  }

  // free up fact array now
  free(fact);

  for (int p = 0; p < (gs+over); p++){

    err = 0;
    
    // loop through all elements of the permutation
    for(ull j = 0; j < n; j++) {
      int val = perms[p][j];

      // check everything ahead of j
      if (err == 0){
        for(int k = j+1, dist = 1; k < (int)n; k++, dist++) {
          // check if the value +/- dist is equal (means its a diagnoal)
          if(val - dist == perms[p][k] || val + dist  == perms[p][k]) {
            //free(perm);
            //return 0;
            err++;
          }
        }
      }

      if (err == 0){
        // check everything behind j
        for(int k = j-1, dist = 1; k >= 0; k--, dist++) {
          // check if the value +/- dist is equal (means its a diagonal)
          if(val - dist == perms[p][k] || val + dist  == perms[p][k]) {
            //free(perm);
            //return 0;
            err++;
          }
        }
      }

      if (err > 0)
        break;

    }

    // if no error was found it means there is a solution, save its position
    if (err == 0){
      answer_list[answer_count] = p;
      answer_count++;
    }
  }

  printf("[%d] found %d solutions\n", proc, answer_count);
  // send the amount of valid solutions found, to the master
  MPI_Isend(&answer_count,1,MPI_INT,0,tag_answer_count,MPI_COMM_WORLD, &send_request);

  // if has any valid solution, concatenate in a single array, in sequence
  if (answer_count > 0){

    int *answer_int = (int*) calloc( answer_count * n,sizeof(int));
    int index = 0;

    for (int h = 0; h < answer_count; h++){
      int answer_pos = answer_list[h]; //get the position of the valid answer
      
      for (int t = 0; t < n; t++){
        answer_int[index++] = perms[answer_pos][t];
      }
    }

    // send an integer array of solutions to the master
    //MPI_Send(&(answer_int[0]),(answer_count*n),MPI_INT,0,tag_answer_int,MPI_COMM_WORLD);
    MPI_Isend(&(answer_int[0]),(answer_count*n),MPI_INT,0,tag_answer_int,MPI_COMM_WORLD, &send_request2);
  }

  printf("[%d] thread finished\n", proc);

  //free(perm);
  return answer_count;
}



/**
 * ull factorial(ull n)
 * - factorial is a one-line function to recursively calculate
 * the factorial for a given n value.
 **/
ull factorial(ull n) {
  return (n == 1 || n == 0) ? 1 : factorial( n - 1 ) * n;
} 

/**
 * int formatMessage(ull n)
 * format the message to a string, separated by ';' - not used
 **/
int formatMessage(int n, int *perm, char *msg){
  int index = 0;
  for (int i=0; i < n; i++){
    index += snprintf(&msg[index], 100,"%d", findPosition(perm[i],i,n));
    msg[index++] = ';';
  }
  msg[index] = '\0';
  //printf("formatted message: %s\n", msg);
  return 0;
}

/**
 * int findPosition(int pos, int line, int size)
 * calculate the sequential position of the queen in the chessboard, from 0 to (n x n -1)
 **/
int findPosition(int pos, int line, int size){
  return (size * line) + pos;
}