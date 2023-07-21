#ifndef SPARSELU_H
#define SPARSELU_H

#define EPSILON 1.0E-6

int checkmat (float *M, float *N);
void genmat (float *M[]);
void lu_genmat (float *M[]);
void print_structure(char *name, float *M[]);
float * allocate_clean_block();
float * lu_allocate_clean_block();
void lu_pre_allocate(float **BENCH);
void lu0(float *diag);
void bdiv(float *diag, float *row);
void bmod(float *row, float *col, float *inner);
void fwd(float *diag, float *col);

void sparselu_init (float ***pBENCH, char *pass); 
//void sparselu(float **BENCH);
void lu_sparselu_init (float ***pBENCH, char *pass);
void sparselu_fini (float **BENCH, char *pass); 

void sparselu_seq_call(float **BENCH);
void sparselu_par_call(float **BENCH);

int sparselu_check(float **SEQ, float **BENCH);

typedef enum {
    FULL = 0,           // run the whole program
    LU0 = 1,            // stop after first lu0 call/codelet
    FWD = 2,            // stop after first fwd call/codelet
    BDIV = 3,           // stop after first bdiv call/codelet
    ALLOC = 4,          // stop after performing first alloc_clean_block
    BMOD = 5,           // stop after first bmod call/codelet
    INNER2JJTAIL = 6,   // stop after first completion of inner JJ loop
    OUTERTAIL = 7       // stop after first iteration of outer is completed
} stop_point;

#endif
