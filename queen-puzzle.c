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
  int rank, sze;    
  ull i = 0 , n = 0, total = 0, subtot = 0;
  MPI_Status status, status2;
  MPI_Request recv_request, recv_request2;
  double elapsed_time;

  char filename[100];
  FILE *fmat;

  // initialize mpi
  MPI_Init(&argc, &argv);
  
  // set rank and size of all procs
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &sze);

  //set start time
  MPI_Barrier(MPI_COMM_WORLD);
  elapsed_time = -MPI_Wtime();


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
    int over = (sze == (rank+1) ? (max % sze) : 0); //send the leftover to the last
    
    if (rank == 0){
      //printf("the group size is %d. we got %lld possibilities divided in %d threads\n", group_size, max, sze);

      sprintf(filename, "solution%lld.txt",n);
      int ret = remove(filename);

      //delete the previous file
      if(ret == 0) {
        printf("File deleted successfully\n");
      } else {
        printf("Error: unable to delete the file\n");
      }
    }
    
    // start at the rankth permutation and move up sze permutations at a time till at end
      subtot += nqueens(rank, i, n, group_size, over);
      int answer_count = 0;

      

      if (rank == 0){

        for (int y = 0; y < sze; y++){
          printf("[%d] is waiting for the 1st response from %d\n",rank, y);
          MPI_Irecv(&answer_count,1,MPI_INT,y,tag_answer_count,MPI_COMM_WORLD,&recv_request);
          MPI_Wait(&recv_request, &status);

          printf("[%d] get the 1st response from %d, that is %d\n", rank, y,answer_count);
          if (answer_count > 0){

            int *answer_int = (int*) calloc( answer_count * n,sizeof(int));
            printf("[%d] is waiting for the 2st response from %d\n",rank, y);
            MPI_Irecv(&(answer_int[0]),(answer_count*n),MPI_INT,y,tag_answer_int,MPI_COMM_WORLD,&recv_request2);
            MPI_Wait(&recv_request2, &status2);
            printf("[%d] get the 2st response from %d\n", rank, y);
            //printf("   recebido: ");
            //for (int t = 0; t < (answer_count * n); t++){
            //  printf("%d ", answer_int[t]);
            //}
            //printf("\n");
            
            


            //append the solutions sent by the thread in the end of file
            fmat = fopen(filename, "a");
            int position = 0;
            int index = 0;

            for (int t = 0; t < (answer_count * n); t++){
              position = findPosition(answer_int[t], t-(index*n) ,n);
              fprintf(fmat,"%d;", position);
              //printf("%d mod %d = %d\t", t+1, answer_count, ((t+1) % answer_count));
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

  MPI_Barrier(MPI_COMM_WORLD);
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
  //printf("from proc %d, received: i = %lld n = %lld and gs = %lld and over = %d\n", proc, i, n, gs, over);
  // declare some variables and allocate arrays
  int a, b = 0;
  int *fact = (int *)calloc(n, sizeof(int));

  MPI_Request send_request, send_request2;

  //int *perm = (int *)calloc(n, sizeof(int));

  printf("[%d] started calculations\n", proc);
  //double timex = -MPI_Wtime();

  int *answer_list = (int*) calloc(gs+over,sizeof(int));

  int **perms = (int**) calloc(gs+over,sizeof(int *));
  for ( int t = 0; t < (gs+over); t++ ) {
    perms[t] = (int*) calloc (n, sizeof(int));
    if (perms[t] == NULL) {
      printf ("** Erro: Memoria Insuficiente **\n");
    }
  }

  // calculate factorial based on i, store in perm
  fact[b] = 1;
  while(++b < (int)n) {
    fact[b] = fact[b-1]*b;
  }

  int lim = (gs+(gs*proc)) + over;
  for (int t = (proc*gs), p = 0; t < lim; t++, p++){ 
    i = t;
    //printf("[%d] t = %d, p = %d, lim = %lld\n", proc, t, p, (gs+(gs*proc)));

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
  
  /*/apenas imprime a combinacao encontrada
  for (int p = 0; p < (gs+over); p++){
    printf("[%d] ",proc);
    for (int x = 0; x < n; x++){
      printf("%d ", perms[p][x]);
    }
    printf("\n");
  }*/
          

  // free up fact array now
  free(fact);


  int answer_count = 0;
  for (int p = 0; p < (gs+over); p++){

    int err = 0;
    
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

    if (err == 0){

      answer_list[answer_count] = p;
      answer_count++;

    }

    //iff we made it to here, free and return 1
    //free(perm);
    //return 1;
  }



  //timex += MPI_Wtime();
  //printf("[%d] took %.6f seconds to calculate\n", proc, timex);

  MPI_Isend(&answer_count,1,MPI_INT,0,tag_answer_count,MPI_COMM_WORLD, &send_request);

  //printf("[%d] passed the 1st send\n",proc);
  if (answer_count > 0){
    
    int *answer_int = (int*) calloc( answer_count * n,sizeof(int));
    int index = 0;

    for (int h = 0; h < answer_count; h++){
      int answer_pos = answer_list[h];
      //printf("[%d] encontrei uma resposta na posicao %d\n", proc,answer_pos);
      
      for (int t = 0; t < n; t++){
        answer_int[index++] = perms[answer_pos][t];
      }

    }

    //for (int t = 0; t < (answer_count * n); t++){
    //  printf("%d ", answer_int[t]);
    //}
    //printf("\n");

    MPI_Isend(&(answer_int[0]),(answer_count*n),MPI_INT,0,tag_answer_int,MPI_COMM_WORLD, &send_request2);
    //printf("[%d] passed the 2st send\n",proc);
  }

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

int findPosition(int pos, int line, int size){
  return (size * line) + pos;
}