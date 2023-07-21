/**********************************************************************************************/
/*  This program is part of the Barcelona OpenMP Tasks Suite                                  */
/*  Copyright (C) 2009 Barcelona Supercomputing Center - Centro Nacional de Supercomputacion  */
/*  Copyright (C) 2009 Universitat Politecnica de Catalunya                                   */
/*                                                                                            */
/*  This program is free software; you can redistribute it and/or modify                      */
/*  it under the terms of the GNU General Public License as published by                      */
/*  the Free Software Foundation; either version 2 of the License, or                         */
/*  (at your option) any later version.                                                       */
/*                                                                                            */
/*  This program is distributed in the hope that it will be useful,                           */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of                            */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                             */
/*  GNU General Public License for more details.                                              */
/*                                                                                            */
/*  You should have received a copy of the GNU General Public License                         */
/*  along with this program; if not, write to the Free Software                               */
/*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA            */
/**********************************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h> 
#include <string.h>
#include <math.h>
#include <libgen.h>
//#include "bots.h"
#include "sparselu.h"

/***********************************************************************
 * checkmat: 
 **********************************************************************/
// appears to just check the matrix -- can be a single long codelet to start?
int checkmat (float *M, float *N)
{
   int i, j;
   float r_err;

   for (i = 0; i < bots_arg_size_1; i++)
   {
      for (j = 0; j < bots_arg_size_1; j++)
      {
         r_err = M[i*bots_arg_size_1+j] - N[i*bots_arg_size_1+j];
         if ( r_err == 0.0 ) continue;

         if (r_err < 0.0 ) r_err = -r_err;

         if ( M[i*bots_arg_size_1+j] == 0 ) 
         {
           bots_message("Checking failure: A[%d][%d]=%f  B[%d][%d]=%f; \n",
                    i,j, M[i*bots_arg_size_1+j], i,j, N[i*bots_arg_size_1+j]);
           return FALSE;
         }  
         r_err = r_err / M[i*bots_arg_size_1+j];
         if(r_err > EPSILON)
         {
            bots_message("Checking failure: A[%d][%d]=%f  B[%d][%d]=%f; Relative Error=%f\n",
                    i,j, M[i*bots_arg_size_1+j], i,j, N[i*bots_arg_size_1+j], r_err);
            return FALSE;
         }
      }
   }
   return TRUE;
}
/***********************************************************************
 * genmat: 
 **********************************************************************/
// generates matrix at M, port directly to codelet
void genmat (float *M[])
{
   int null_entry, init_val, i, j, ii, jj;
   float *p;

   init_val = 1325;

   /* generating the structure */
   for (ii=0; ii < bots_arg_size; ii++) // crosses full matrix size: ii/jj indicate address of submatrices?
   {
      for (jj=0; jj < bots_arg_size; jj++)
      {
         /* computing null entries */
         null_entry=FALSE;
         if ((ii<jj) && (ii%3 !=0)) null_entry = TRUE; // deciding with ifs whether submatrix has data or not
         if ((ii>jj) && (jj%3 !=0)) null_entry = TRUE; 
	 if (ii%2==1) null_entry = TRUE; 
	 if (jj%2==1) null_entry = TRUE; 
	 if (ii==jj) null_entry = FALSE; 
	 if (ii==jj-1) null_entry = FALSE; 
         if (ii-1 == jj) null_entry = FALSE;  
         /* allocating matrix */
         if (null_entry == FALSE){
            M[ii*bots_arg_size+jj] = (float *) malloc(bots_arg_size_1*bots_arg_size_1*sizeof(float));
	    if (M[ii*bots_arg_size+jj] == NULL) // error checking for malloc / mem space
            {
               bots_message("Error: Out of memory\n");
               exit(101);
            }
            /* initializing matrix */
            p = M[ii*bots_arg_size+jj];
            for (i = 0; i < bots_arg_size_1; i++) // setting values for valid submatrices
            {
               for (j = 0; j < bots_arg_size_1; j++)
               {
	            init_val = (3125 * init_val) % 65536;
      	            (*p) = (float)((init_val - 32768.0) / 16384.0);
                    p++;
               }
            }
         }
         else
         {
            M[ii*bots_arg_size+jj] = NULL;
         }
      }
   }
}
/***********************************************************************
 * print_structure: 
 **********************************************************************/
// direct to codelet
void print_structure(char *name, float *M[])
{
   int ii, jj;
   bots_message("Structure for matrix %s @ 0x%p\n",name, M);
   for (ii = 0; ii < bots_arg_size; ii++) {
     for (jj = 0; jj < bots_arg_size; jj++) {
        if (M[ii*bots_arg_size+jj]!=NULL) {bots_message("x");}
        else bots_message(" ");
     }
     bots_message("\n");
   }
   bots_message("\n");
}
/***********************************************************************
 * allocate_clean_block: 
 **********************************************************************/
// clear candidate for a memory codelet 
float * allocate_clean_block()
{
  int i,j;
  float *p, *q;

  p = (float *) malloc(bots_arg_size_1*bots_arg_size_1*sizeof(float));
  q=p;
  if (p!=NULL){
     for (i = 0; i < bots_arg_size_1; i++) 
        for (j = 0; j < bots_arg_size_1; j++){(*p)=0.0; p++;}
	
  }
  else
  {
      bots_message("Error: Out of memory\n");
      exit (101);
  }
  return (q);
}

/***********************************************************************
 * lu0: 
 **********************************************************************/
void lu0(float *diag)
{
   int i, j, k;

   for (k=0; k<bots_arg_size_1; k++)
      for (i=k+1; i<bots_arg_size_1; i++)
      {
         diag[i*bots_arg_size_1+k] = diag[i*bots_arg_size_1+k] / diag[k*bots_arg_size_1+k];
         for (j=k+1; j<bots_arg_size_1; j++)
            diag[i*bots_arg_size_1+j] = diag[i*bots_arg_size_1+j] - diag[i*bots_arg_size_1+k] * diag[k*bots_arg_size_1+j];
      }
}

/***********************************************************************
 * bdiv: 
 **********************************************************************/
void bdiv(float *diag, float *row)
{
   int i, j, k;
   for (i=0; i<bots_arg_size_1; i++)
      for (k=0; k<bots_arg_size_1; k++)
      {
         row[i*bots_arg_size_1+k] = row[i*bots_arg_size_1+k] / diag[k*bots_arg_size_1+k];
         for (j=k+1; j<bots_arg_size_1; j++)
            row[i*bots_arg_size_1+j] = row[i*bots_arg_size_1+j] - row[i*bots_arg_size_1+k]*diag[k*bots_arg_size_1+j];
      }
}
/***********************************************************************
 * bmod: 
 **********************************************************************/
void bmod(float *row, float *col, float *inner)
{
   int i, j, k;
   for (i=0; i<bots_arg_size_1; i++)
      for (j=0; j<bots_arg_size_1; j++)
         for (k=0; k<bots_arg_size_1; k++)
            inner[i*bots_arg_size_1+j] = inner[i*bots_arg_size_1+j] - row[i*bots_arg_size_1+k]*col[k*bots_arg_size_1+j];
}
/***********************************************************************
 * fwd: 
 **********************************************************************/
void fwd(float *diag, float *col)
{
   int i, j, k;
   for (j=0; j<bots_arg_size_1; j++)
      for (k=0; k<bots_arg_size_1; k++) 
         for (i=k+1; i<bots_arg_size_1; i++)
            col[i*bots_arg_size_1+j] = col[i*bots_arg_size_1+j] - diag[i*bots_arg_size_1+k]*col[k*bots_arg_size_1+j];
}


void sparselu_init (float ***pBENCH, char *pass)
{
   *pBENCH = (float **) malloc(bots_arg_size*bots_arg_size*sizeof(float *));
   genmat(*pBENCH);
   print_structure(pass, *pBENCH);
}

void sparselu_par_call(float **BENCH)
{
   int ii, jj, kk;

   bots_message("Computing SparseLU Factorization (%dx%d matrix with %dx%d blocks) ",
           bots_arg_size,bots_arg_size,bots_arg_size_1,bots_arg_size_1);
#pragma omp parallel
#pragma omp single nowait
#pragma omp task untied
   // memory codelets needed to move data from memory to registers
   for (kk=0; kk<bots_arg_size; kk++) // loop in scm program
   {
      // memory codelet here to load this data into registers
      lu0(BENCH[kk*bots_arg_size+kk]); // could be a codelet -- might be difficult with program structure
      for (jj=kk+1; jj<bots_arg_size; jj++)
         // these if statements should probably be some combination of memory codelets and branch instructions
         // so memory codelet here will check this condition and load submatrices if valid; note that
         // it only has to load one submatrix -- the one at kk*bots_arg_size+jj since the other was loaded before lu0?
         // wait -- can we use SCM instructions to check if this value is null? We should be able to, since we have kk in
         // a register already, jj will also be in a register and bots_arg_size is fixed. LD instruction + branch instruction,
         // then memory codelet then computation codelet
         /* MEMCOD checkAndLoad destReg, kk, jj -- might have to hardcode BENCH but I think that's fine */
         if (BENCH[kk*bots_arg_size+jj] != NULL)
            #pragma omp task untied firstprivate(kk, jj) shared(BENCH)
         {
            fwd(BENCH[kk*bots_arg_size+kk], BENCH[kk*bots_arg_size+jj]); //fwd call will be a codelet
         }
      for (ii=kk+1; ii<bots_arg_size; ii++) 
         if (BENCH[ii*bots_arg_size+kk] != NULL)
            #pragma omp task untied firstprivate(kk, ii) shared(BENCH)
         {
            bdiv (BENCH[kk*bots_arg_size+kk], BENCH[ii*bots_arg_size+kk]); //bdiv call will be a codelet
         }

      #pragma omp taskwait // barrier - good place to split this into codelets

      for (ii=kk+1; ii<bots_arg_size; ii++)
         if (BENCH[ii*bots_arg_size+kk] != NULL) // these if statements will each be a memory codelet
            for (jj=kk+1; jj<bots_arg_size; jj++)
               if (BENCH[kk*bots_arg_size+jj] != NULL) // this mem codelet should also check the below condition and perform allocation
               #pragma omp task untied firstprivate(kk, jj, ii) shared(BENCH)
               {
                     // we don't want memory codelets to be performing memory allocation, only memory movement
                     // so, in the main program outside of the SCM machine, we iterate through and allocate these 
                     // blocks beforehand, so we don't have to do it during
                     if (BENCH[ii*bots_arg_size+jj]==NULL) BENCH[ii*bots_arg_size+jj] = allocate_clean_block(); //allocate_clean_block -- mem codelet
                     //void bmod(float *row, float *col, float *inner)
                     /* float *inner = this->getParams().getParamValueAs<float*>(1);
                        float *row = this->getParams().getParamValueAs<float*>(2);
                        float *col = this->getParams().getParamValueAs<float*>(3);
                     */
                     bmod(BENCH[ii*bots_arg_size+kk], BENCH[kk*bots_arg_size+jj], BENCH[ii*bots_arg_size+jj]); //bmod call will be a codelet
               }

      #pragma omp taskwait // end barrier
   }
   bots_message(" completed!\n");
}


void sparselu_seq_call(float **BENCH)
{
   int ii, jj, kk;

   for (kk=0; kk<bots_arg_size; kk++)
   {
      lu0(BENCH[kk*bots_arg_size+kk]);
      for (jj=kk+1; jj<bots_arg_size; jj++)
         if (BENCH[kk*bots_arg_size+jj] != NULL)
         {
            fwd(BENCH[kk*bots_arg_size+kk], BENCH[kk*bots_arg_size+jj]);
         }
      for (ii=kk+1; ii<bots_arg_size; ii++)
         if (BENCH[ii*bots_arg_size+kk] != NULL)
         {
            bdiv (BENCH[kk*bots_arg_size+kk], BENCH[ii*bots_arg_size+kk]);
         }
      for (ii=kk+1; ii<bots_arg_size; ii++)
         if (BENCH[ii*bots_arg_size+kk] != NULL)
            for (jj=kk+1; jj<bots_arg_size; jj++)
               if (BENCH[kk*bots_arg_size+jj] != NULL)
               {
                     if (BENCH[ii*bots_arg_size+jj]==NULL) BENCH[ii*bots_arg_size+jj] = allocate_clean_block();
                     bmod(BENCH[ii*bots_arg_size+kk], BENCH[kk*bots_arg_size+jj], BENCH[ii*bots_arg_size+jj]);
               }

   }
}

void sparselu_fini (float **BENCH, char *pass)
{
   print_structure(pass, BENCH);
}

int sparselu_check(float **SEQ, float **BENCH)
{
   int ii,jj,ok=1;

   for (ii=0; ((ii<bots_arg_size) && ok); ii++)
   {
      for (jj=0; ((jj<bots_arg_size) && ok); jj++)
      {
         if ((SEQ[ii*bots_arg_size+jj] == NULL) && (BENCH[ii*bots_arg_size+jj] != NULL)) ok = FALSE;
         if ((SEQ[ii*bots_arg_size+jj] != NULL) && (BENCH[ii*bots_arg_size+jj] == NULL)) ok = FALSE;
         if ((SEQ[ii*bots_arg_size+jj] != NULL) && (BENCH[ii*bots_arg_size+jj] != NULL))
            ok = checkmat(SEQ[ii*bots_arg_size+jj], BENCH[ii*bots_arg_size+jj]);
      }
   }
   if (ok) return BOTS_RESULT_SUCCESSFUL;
   else return BOTS_RESULT_UNSUCCESSFUL;
}

