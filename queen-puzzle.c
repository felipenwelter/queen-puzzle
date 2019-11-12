/*****************************************************
 * Name: nqueens.c 
 * Author: Lucas Carpenter
 * Date: 12/11/2018
 * 
 * For CSC410 Parallel Computing course, Final program
 *
 * Compiling: mpic++ -Wall -o nqueens nqueens.c
 * Usage: mpirun -np <num processors> ./nqueens <n>

 * Description: Program takes in the size of the board
 *  as an n value, MPI is initialized and loops from 
 *  rank to factorial(n) incrementing by size each iteration.
 *  Inside the loop the i-th permutation of a sorted array of 
 *  incrementing integers is calculated and a check is done 
 *  to make sure that no 'queen' is on any diagonal. If any of 
 *  the checks fail, then that iteration short-circuits and 
 *  checks the next iteration. If the loop reaches the end, 
 *  then the counter is incremented. The time of execution and 
 *  number of solutions found are then output.
 *
 *****************************************************/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

typedef unsigned long long ull;

// Prototype declarations
int nqueens(int proc, ull i, ull n);
ull factorial(ull n);

/**
 * int main(int argc, char*argv[])
 * - The starting point of the program, calculates 
 * the number of solutions for the n-queens problem
 * given an input n for an n x n board using MPI.
 **/
int main(int argc, char*argv[]) {

  // declare some variables
  int rnk, sze;    
  ull i = 0 , n = 0, total = 0, subtot = 0;
  double elapsed_time;
  MPI_Status status;

  // initialize mpi
  MPI_Init(&argc, &argv);

  // set rank and size of all procs
  MPI_Comm_rank(MPI_COMM_WORLD, &rnk);
  MPI_Comm_size(MPI_COMM_WORLD, &sze);

  // set start time
  MPI_Barrier(MPI_COMM_WORLD);
  elapsed_time = -MPI_Wtime();

  // find out if program has correct command line arguments, prompt if not
  if(argc != 2) {
    if(rnk == 0)
      printf("Invalid number of command line arguments.\nFormat should be ./nqueens <n>\n\n");
  } else {
    // set value of n from command line and create an array of length n
    n = atoi(argv[1]);

    // encontra o número de combinações possíveis
    ull max = factorial(n);
    //printf("max (fatorial de %d) é %lld\n", n, max);




    if (rnk != 0){
      // start at the rnkth permutation and move up sze permutations at a time till at end
      for( i = rnk; i < max; i+=(sze-1) ) {
        subtot += nqueens(rnk, i, n);
      }
    }else{
      int *perm = (int *)calloc(n, sizeof(int));
      char msg[100];
      
      for( i = rnk+1; i < max; i+=(sze-1) ) {
        //printf("aguardando receber de %d\n", i);
        MPI_Recv(msg,100,MPI_CHAR,1,0,MPI_COMM_WORLD,&status);
        
        
        if (msg[0] != 'x'){
          printf("%s\n",msg);
        }
        

      }

      



    }

    // reduce subtotal into grand total
    MPI_Reduce(&subtot, &total, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);


  }

  // find ending time
  MPI_Barrier(MPI_COMM_WORLD);
  elapsed_time += MPI_Wtime();

  // if root rank, output solutions found and time taken
  if(0 == rnk) {
    printf("Program executed in %8.3f ms\n", 1000*elapsed_time);
    printf("Total number of solutions found: %llu\n\n", total);
    fflush(stdout);
  }

  //exit program
  MPI_Finalize();
  return 0;
}

/**
 * int nqueens(int proc, int*vals, ull i, ull n)
 * - nqueens takes an MPI rank, the current ith permutation
 * and the size of the board and finds the i8th permutation, 
 * and validates that no queens are in the diagonal of the 
 * current queen, if there is, we short-circuit and return 0,
 * otherwise we return 1; 
 **/
int nqueens(int proc, ull i, ull n) {

  // declare some variables and allocate arrays
  int a, b = 0;
  int *fact = (int *)calloc(n, sizeof(int));
  int *perm = (int *)calloc(n, sizeof(int));
  char msg[100];

//printf("sou o processo %d e estou calculando a combinacao %d para tamanho %d\n", proc, i, n);

  // calculate factorial based on i, store in perm
  fact[b] = 1;
  while(++b < (int)n) {
    fact[b] = fact[b-1]*b;
    //printf("fact[%d] = %lld\n", b, fact[b]);
  }
  
  //monta o grupo de combinações base
  for(b = 0; b < (int)n; ++b) {
    perm[b] = i / fact[n - 1 - b];
    i = i % fact[n - 1 - b];
    //printf("%lld ",perm[b]);
  }
  //printf("\n");

  // percorre da direita para a esquerda, e sempre que encontra
  // alguma linha em que a rainha esteja na mesma coluna ou em alguma coluna
  // à esquerda, move a rainha mais adiante. exemplo:
  // orginal: 0 1 1 0
  // posicionamento: [0] 1  1 (x) | 0 1 1 1
  // posicionamento: [0][1](x) 1  | 0 1 3 1
  // posicionamento: [0](x) 2  1  | 0 2 3 1
  for(b = n - 1; b > 0; --b) {
    for(a = b - 1; a >= 0; --a) {
      if(perm[a] <= perm[b]) {
	      perm[b]++;
      }
    }
  }

  //apenas imprime a combinacao encontrada
  //for (int x = 0; x < n; x++)
  //  printf("%lld ", perm[x]);
  //printf("\n");

  // free up fact array now
  free(fact);

  // loop through all elements of the permutation
  for(ull j = 0; j < n; j++) {
    int val = perm[j];

    // check everything ahead of j
    for(int k = j+1, dist = 1; k < (int)n; k++, dist++) {
      // check if the value +/- dist is equal (means its a diagnoal)
      if(val - dist == perm[k] || val + dist  == perm[k]) {
        //printf("vou enviar erro %d\n", proc);
        //perm[0] = -1;
        //MPI_Send(perm,n,MPI_INT,0,0,MPI_COMM_WORLD);
        msg[0] = 'x';
        MPI_Send(msg,100,MPI_CHAR,0,0,MPI_COMM_WORLD);
        //free(perm);
        //printf("erro\n");
        return 0;
      }
    }

    // check everything behind j
    for(int k = j-1, dist = 1; k >= 0; k--, dist++) {
      // check if the value +/- dist is equal (means its a diagonal)
      if(val - dist == perm[k] || val + dist  == perm[k]) {
        //printf("vou enviar erro %d\n", proc);
        //perm[0] = -1;
        //MPI_Send(perm,n,MPI_INT,0,0,MPI_COMM_WORLD);
        msg[0] = 'x';
        MPI_Send(msg,100,MPI_CHAR,0,0,MPI_COMM_WORLD);
        //free(perm);
        //printf("erro\n");
        return 0;
      }
    }
  }

  //iff we made it to here, free and return 1
  int index = 0;
  for (int i=0; i < n; i++){
    index += sprintf(&msg[index], "%d", perm[i]);
    msg[index++] = '#';
  }

  //printf("to com meu char aqui convertido: %s",msg);
  MPI_Send(msg,100,MPI_CHAR,0,0,MPI_COMM_WORLD);

  //free(perm);
  return 1;
}
 
/**
 * ull factorial(ull n)
 * - factorial is a one-line function to recursively calculate
 * the factorial for a given n value.
 **/
ull factorial(ull n) {
  return (n == 1 || n == 0) ? 1 : factorial( n - 1 ) * n;
}