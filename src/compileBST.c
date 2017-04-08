/*! \file compileBST.c
 *  \brief	   This implements the applyPatch program.
 *  \author    Lucie Pansart
 *  \author    Jean-Louis Roch
 *  \version   1.0
 *  \date      30/9/2016
 *  \warning   Usage: compileBST n originalFile
 *  \copyright GNU Public License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>
#include <string.h>

// (un)comment this define to toggle debug print
//#define DEBUG

#ifdef DEBUG
#define IF_DEBUG(_p_) { _p_ ;}
#else
#define IF_DEBUG(_p_) {}
#endif

void print_usage(FILE* f) {
    fprintf(f, "Usage: compileBST <n> <file>\n");
}

void print_strtol_error(FILE* f, const int err, const char *str) {
    switch (err) {
    case EINVAL :
        fprintf(f, "%s does not match a long integer value. \n", str);
        break;
    case ERANGE :
        fprintf(f, "%s does not fit in a long int :\n" "out of bound [%ld;%ld]\n", str, LONG_MIN, LONG_MAX);
        break;
    default:
        break;
    }
}
typedef struct ctx_abr {
    long size;
    long*  elements;  // Stockage des valeurs lues dans le fichier
    long** weights;   // Stockage des poids et des racines
    long*  sum_proba; // Stockage de la somme des poids de l'élément 0 à n
} ctx_abr;

void free_ctx_abr(ctx_abr *c) {
    free(c->sum_proba);
    free(c->elements);
    free(c->weights[0]);
    free(c->weights);
    free(c);
}

ctx_abr * new_ctx_abr(const long size) {
    ctx_abr * c = malloc(sizeof(struct ctx_abr));
    if (c != NULL) {
        c->size = size;
        c->elements = malloc(size * sizeof(long));
        c->sum_proba = malloc(size * sizeof(long));
        // create a two dimensional array with contiguous data.
        c->weights = malloc(size * sizeof(long *));
        c->weights[0] = malloc(size * size * sizeof(long));
        for (long i = 1; i < size; i++) {
            // updating pointers to differents rows
            c->weights[i] = c->weights[i-1] + size; // pointer arithmetics
        }
        if (c->elements == NULL || c->sum_proba == NULL
            || c->weights == NULL || c->weights[0] == NULL ) {
            perror(__func__);
            fprintf(stderr, "ERROR can't create a new struct ctx_abr !\n");
            free_ctx_abr(c);
            c = NULL;
        } else {
            // initialize weights value with -1
            memset(c->weights[0], -1L, size * size * sizeof(long));
        }
    }
    return c;
}

void init_ctx(ctx_abr *c) {
    long i, n = c->size;
    long sum = 0;
    for (i = 0; i < n; i++) {
        sum += c->elements[i];
        c->sum_proba[i] = sum;
        // a ABR with only one element have a wheight of it's element proba
        c->weights[i][i] = c->elements[i];
    }
}

void parser(FILE* freqFile, int n, int* tab){
  int c;
  int poids = 0;
  int i = 0;

  while(i < n)
    {
      //if( feof(freqFile) ) {break ;}

      poids = 0;
       c = fgetc(freqFile);
       c = fgetc(freqFile);
       while (c != 32){
         poids = poids*10 + (c - 48);
         c = fgetc(freqFile);
       }
       printf(" %i ", poids);
       tab[i] = poids;
       i++;
    }
}

void optABR(int* valeurs, int n, int m, int** poidsABR, int* poids, int N){
  int poidsMin;
  int racineMin;
  int i = n;
  while (i < m){
    if(poidsABR[n][i-1] == -1){
      optABR(valeurs,n,i-1,poidsABR,poids,N);
    }
    if(poidsABR[i+1][m] == -1){
      optABR(valeurs,i+1,m,poidsABR,poids,N);
    }
    if (poidsMin > poids[m] - poids[n] + poidsABR[n][i-1] + poidsABR[i+1][m] || i == n){
      poidsMin = poids[m] - poids[n] + poidsABR[n][i-1] + poidsABR[i+1][m];
      racineMin = i;
    }
  }
  poidsABR[n][m] = poidsMin;
  poidsABR[N-n][N-m] = racineMin;

}



/**
 * Main function
 * \brief Main function
 * \param argc  A count of the number of command-line arguments
 * \param argv  An argument vector of the command-line arguments.
 * \warning Must be called with a positive long integer, n,  and a filename, freqFile, as commandline parameters and in the given order.
 * \returns { 0 if succeeds and prints C code implementing an optimal ABR on stdout; exit code otherwise}
 */
int main (int argc, char *argv[]) {
  long n = 0 ; // Number of elements in the dictionary
  FILE *freqFile = NULL ; // File that contains n positive integers defining the relative frequence of dictionary elements

  if(argc != 3){
      print_usage(stderr);
      exit(EXIT_FAILURE); /* indicate failure.*/
  }


  // Conversion of argv[1] en long
  n = strtol(argv[1], NULL, 10);
  // Traitement des erreurs
  if (errno != EXIT_SUCCESS) {
      print_strtol_error(stderr, errno, argv[1]);
      perror(__func__);
      exit(EXIT_FAILURE);
  }
  if (n < 0) {
      fprintf(stderr, "%s cannot be converted into a positive integer.\n", argv[1]);
      exit(EXIT_FAILURE);
  } else if (n < 3) {
      fprintf(stderr, "The number of elements in the dictionary must be greater than 2.\n");
      exit(EXIT_FAILURE);
  }
  fprintf(stderr, "Number of elements in the dictionary: %ld\n", n);

  // Opening dictionary file
  freqFile = fopen(argv[2] , "r" );
  if (freqFile == NULL) {
      fprintf(stderr, "FAIL to open file '%s' !\n", argv[2]);
      exit(EXIT_FAILURE);
  }

  int valeurs[n]; //Stockage des valeurs lues dans le fichier
  int poidsABR[n][n]; //Stockage des poids et des racines
  int poids[n]; //Stockage de la somme des poids de l'élément 0 à n

  //initialisation des tableaux
  parser(freqFile, n, valeurs);
  int i, j;
  poids[0] = valeurs[0];
  for(i = 1; i < n; i++){
      poids[i] = valeurs[i] + poids[i-1];
  }
  for (i = 0; i < n; i++){
    for (j = 0; j < n; i++){
      poidsABR[i][j] = -1;
    }
  }

  optABR(valeurs, 0, n, poidsABR, poids, n);


  // TO BE COMPLETED
  fclose(freqFile);


  return 0;
}
