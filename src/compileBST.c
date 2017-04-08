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
        c->sum_proba = malloc((1 + size) * sizeof(long));
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
    c->sum_proba[0]=0;
    for (i = 0; i < n; i++) {
        sum += c->elements[i];
        c->sum_proba[1+i] = sum;
        // a ABR with only one element have a wheight of it's element proba
        c->weights[i][i] = c->elements[i];
    }
}

void print_ctx(ctx_abr *c) {
    long i, j, n = c->size;
    puts("elements :");
    for (i = 0; i < n; i++) {
        printf("%li ",c->elements[i]);
    }
    puts("\nsum_proba :");
    for (i = 0; i < n; i++) {
        printf("%li ",c->sum_proba[i]);
    }
    puts("\nweights :");
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            printf("%li ",c->weights[i][j]);
        }
        puts("");
    }
    puts("");
}

int parser(ctx_abr *c, FILE* freqFile){
    long i = 0, n = c->size;
    long poids = 0;
    char word[256]; // will fail to read words of more than 256 chars !
    while(i < n && !feof(freqFile)) {
        fscanf(freqFile, "%s ", word);
        poids = strtol(word, NULL, 10);
        if (errno != EXIT_SUCCESS) {
            print_strtol_error(stderr, errno, word);
            perror(__func__);
            exit(EXIT_FAILURE);
        }
        c->elements[i] = poids;
        i++;
    }
    return (i == n)?0:-1;
}

long optABR(ctx_abr *c, long n, long m){
    long poids, abr_left, abr_right, poidsMin = LONG_MAX;
    long racineMin, sum_proba;
    long r = n;
    if (n > m) {
        fprintf(stderr, "ERROR n > m : %li - %li !\n", n, m);
        return 0;
    }
    IF_DEBUG(printf("optABR %li - %li\n",n,m);)
    if (n == m) {
        return c->elements[n];
    }
    sum_proba = c->sum_proba[m+1] - c->sum_proba[n];

    // special case r == n => no abr_left
    abr_right = c->weights[r+1][m];
    if (abr_right == -1){
        abr_right = optABR(c, r+1, m);
    } else IF_DEBUG(printf("allRdy %li - %li   =   %li\n",r+1,m,abr_right));
    poids = sum_proba + abr_right;
    if (poids < poidsMin){
        poidsMin = poids;
        racineMin = r;
    }
    // r in ]n, m[
    for (r++; r < m; r++) {
        abr_left = c->weights[n][r-1];
        abr_right = c->weights[r+1][m];
        if (abr_left == -1) {
            abr_left = optABR(c, n, r-1);
        } else IF_DEBUG(printf("allRdy %li - %li   =   %li\n",n,r-1,abr_left););
        if (abr_right == -1){
            abr_right = optABR(c, r+1, m);
        } else IF_DEBUG(printf("allRdy %li - %li   =   %li\n",r+1,m,abr_right););
        poids = sum_proba + abr_left + abr_right;
        if (poids < poidsMin){
            poidsMin = poids;
            racineMin = r;
        }
    }
    // special case r == m => no abr_right
    abr_left = c->weights[n][r-1];
    if (abr_left == -1){
        abr_left = optABR(c, n, r-1);
    } else IF_DEBUG(printf("allRdy %li - %li   =   %li\n",n,r-1,abr_left);)
    poids = sum_proba + abr_left;
    if (poids < poidsMin){
        poidsMin = poids;
        racineMin = r;
    }

    c->weights[n][m] = poidsMin;
    c->weights[m][n] = racineMin;

    return poidsMin;
}

long static get_root(ctx_abr *c, long n, long m) {
    if (n > m) {
        return -1;
    } else if (n == m) {
        return n;
    } else {
        return c->weights[m][n];
    }
}

void print_out_abr_rec(ctx_abr *c, FILE *out, long n, long m) {
    long r;
    if (n > m) {
        return;
    }
    r = c->weights[m][n];
    if (n == m) {
        // feuille
        fprintf(out, "{-1, -1}");
        if ((n+1) != c->size) {
            fprintf(out, ",\n");
        }
    } else if ((n + 1) == m) { // m - n == 1
        if (r == n) {
            fprintf(out, "{-1, %li},\n", get_root(c, r+1, m));
            print_out_abr_rec(c, out, r+1, m);
        } else {
            print_out_abr_rec(c, out, n, r-1);
            fprintf(out, "{%li, -1}", get_root(c, n, r-1));
            if (m != c->size) {
                fprintf(out, ",\n");
            }
        }
    } else {
        // parcourt infixée
        print_out_abr_rec(c, out, n, r-1);
        fprintf(out, "{%li, %li}", get_root(c, n, r-1), get_root(c, r+1, m));
        if ((n+1) != c->size) {
            fprintf(out, ",\n");
        }
        print_out_abr_rec(c, out, r+1, m);
    }
}

void print_out_abr(ctx_abr *c, FILE *out) {
    long i, size = c->size;
    fprintf(out, "static int BSTdepth = %li;\n", c->weights[0][size - 1]);
    fprintf(out, "static int BSTroot = %li;\n", c->weights[size - 1][0]);
    fprintf(out, "static int BSTtree[%li][2] = {\n", size);
    print_out_abr_rec(c, out, 0, size - 1);
    fprintf(out, "};\n");

    fprintf(out, "static int p[%li] = {\n", size);
    for (i = 0; i < size; i++) {
        fprintf(out, "%li", c->elements[i]);
        if (i+1 != size) {
            fprintf(out, ",\n");
        }
    }
    fprintf(out, "};\n");
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

  ctx_abr * ctx = new_ctx_abr(n);

  // lecture du fichier
  parser(ctx, freqFile);
  fclose(freqFile);

  // initialisation de la somme des poids
  init_ctx(ctx);

  IF_DEBUG(print_ctx(ctx););

  optABR(ctx, 0, ctx->size-1);

  IF_DEBUG(print_ctx(ctx););

  print_out_abr(ctx, stdout);

  free_ctx_abr(ctx);

  return 0;
}
