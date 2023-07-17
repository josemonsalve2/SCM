LDIMM R64B_1, 0; // For iteration variable -- kk
LDIMM R64B_2, 0; // For iteration variable -- jj
LDIMM R64B_3, 0; // For iteration variable -- ii
LDIMM R64B_4, 50;  // For number of iterations and offset calculation -- bot_arg_size
COD loadBenchAddr_64B R64B_5; // Basically a hacky way of fetching BENCH address so we can use it for allocation of new blocks
LDIMM R64B_7, 0; // stays 0 for null checking of submatrices
LDIMM R64B_8, 14580000; // heap pointer: initialized to the size genmat uses to account for BENCH already being allocated
LDIMM R64B_9, 40000; // should leave untouched. Used to increment heap pointer; equals size of submat bots_arg_size_1*bots_arg_size_1*4 (4 for size of float)
// LDIMM R64B_15, 2;

// main outer loop -- kk 
outer:
  BREQ R64B_4, R64B_1, exit;
  COD loadSubMat_2048L R2048L_1, R64B_1, R64B_1;      //Load submat for lu0 with both offsets kk
  COD lu0_2048L R2048L_1;                             //perform lu0 codelet -- r/w on R1
                                                      // first inner loop with iteration variable jj starting at kk+1
  COD storeSubMat_2048L R2048L_1, R64B_1, R64B_1;   // store the loaded submatrix back
  ADD R64B_2, R64B_1, 1; 
  inner_jj:
    BREQ R64B_4, R64B_2, inner_ii_head;               // branch for this loop
    MULT R64B_6, R64B_1, R64B_4;                      // kk * bot_arg_size
    ADD R64B_6, R64B_6, R64B_2;                       // kk * bot_arg_size + jj
    MULT R64B_6, R64B_6, 8;                           // account for size of float *
    LDADR R64B_6, R64B_6;                             // BENCH[kk * bot_arg_size + jj] (now since BENCH is at the root of SCM memory, we load by the offset only)
    BREQ R64B_6, R64B_7, inner_jj_tail;               // branch if submatrix is null -- branch to tail of loop
    COD loadSubMat_2048L R2048L_2, R64B_1, R64B_2;    // R2048L_1 already holds first submat, so fetch second submat to R2048L_2
    COD fwd_2048L R2048L_2, R2048L_1;               
    COD storeSubMat_2048L R2048L_2, R64B_1, R64B_2;   // store the loaded submatrix back
  inner_jj_tail:
    ADD R64B_2, R64B_2, 1; // jj++
    JMPLBL inner_jj;
  inner_ii_head:
    ADD R64B_3, R64B_1, 1;                            // ii = kk + 1
  inner_ii:
    BREQ R64B_4, R64B_3, inner_ii2_head;              // branch for this loop
    MULT R64B_6, R64B_3, R64B_4;                      // ii * bot_arg_size
    ADD R64B_6, R64B_6, R64B_1;                       // ii * bot_arg_size + kk
    MULT R64B_6, R64B_6, 8;                           // account for size of float *
    LDADR R64B_6, R64B_6;
    BREQ R64B_6, R64B_7, inner_ii_tail;               // branch if submatrix is null -- branch to tail of loop
    COD loadSubMat_2048L R2048L_2, R64B_3, R64B_1;    // load submat at ii*bot_arg_size+kk, so pass ii and kk
    COD bdiv_2048L R2048L_2, R2048L_1;
    COD storeSubMat_2048L R2048L_2, R64B_3, R64B_1;
  inner_ii_tail:
    ADD R64B_3, R64B_3, 1;
    JMPLBL inner_ii;
  inner_ii2_head:
    COD storeSubMat_2048L R2048L_1, R64B_1, R64B_1;   // done using submatrix at BENCH[kk*bot_arg_size+kk], so write it back
    ADD R64B_3, R64B_1, 1;                            // ii = kk + 1
  inner_ii2:
    BREQ R64B_4, R64B_3, outer_tail;                  // branch for this loop
    MULT R64B_6, R64B_3, R64B_4;                      // ii * bot_arg_size
    ADD R64B_6, R64B_6, R64B_1;                       // ii * bot_arg_size + kk
    MULT R64B_6, R64B_6, 8;                           // account for size of float *
    LDADR R64B_6, R64B_6;
    BREQ R64B_6, R64B_7, inner_ii2_tail;              // branch if submatrix is null -- branch to tail of loop
    ADD R64B_2, R64B_1, 1;                            // jj = kk + 1
    inner2_jj:
      BREQ R64B_4, R64B_2, inner_ii2_tail;            // branch for this loop
      MULT R64B_6, R64B_1, R64B_4;                    // kk * bot_arg_size
      ADD R64B_6, R64B_6, R64B_2;                     // kk * bot_arg_size + jj
      MULT R64B_6, R64B_6, 8;
      LDADR R64B_6, R64B_6;
      BREQ R64B_6, R64B_7, inner2_jj_tail;            // branch if submatrix is null -- branch to tail of loop
      // here need to calculate offset for ii*bot_arg_size+jj
      MULT R64B_6, R64B_3, R64B_4;                      // ii * bot_arg_size
      ADD R64B_6, R64B_6, R64B_2;                       // ii * bot_arg_size + jj
      MULT R64B_6, R64B_6, 8;                           // account for size of float *
      LDADR R64B_11, R64B_6;                            // load the submat pointer; we need to not overwrite R6 since we'll need it in allocate_clean_block
      BREQ R64B_11, R64B_7, allocate_clean_block;       // if submat pointer is null, jump to allocation function
      COD loadSubMat_2048L R2048L_3, R64B_3, R64B_2;    // loading submat at BENCH[ii*bot_arg_size+jj]; submat wasn't null so we have to load it
      submat_allocated:                                 // this is where allocation function will return to, or where code will continue if not null
      // need to load ii/kk, kk/jj, then do bmod, then store all 3 (including ii/jj)
      COD loadSubMat_2048L R2048L_1, R64B_3, R64B_1;  // loading submat at BENCH[ii*bot_arg_size+kk]
      COD loadSubMat_2048L R2048L_2, R64B_1, R64B_2;  // loading submat at BENCH[kk*bot_arg_size+jj]
      COD bmod_2048L R2048L_3, R2048L_1, R2048L_2;    // execute bmod codelet
      COD storeSubMat_2048L R2048L_1, R64B_3, R64B_1; // storing submat at BENCH[ii*bot_arg_size+kk]
      COD storeSubMat_2048L R2048L_2, R64B_1, R64B_2; // storing submat at BENCH[kk*bot_arg_size+jj]
      COD storeSubMat_2048L R2048L_3, R64B_3, R64B_2; // storing submat at BENCH[ii*bot_arg_size+jj]
    inner2_jj_tail:
      ADD R64B_2, R64B_2, 1;                          // increment jj
      JMPLBL inner2_jj;
  inner_ii2_tail:
    ADD R64B_3, R64B_3, 1;                            // increment ii
    //  BREQ R64B_3, R64B_15, exit;                     // early exit for stop condition INNER2JJTAIL
    JMPLBL inner_ii2;
outer_tail:
  ADD R64B_1, R64B_1, 1;                              // increment kk
  JMPLBL outer;

allocate_clean_block:
ADD R64B_10, R64B_5, R64B_8;                          // add SCM heap pointer (represented as offset) to BENCH base address to get valid external heap pointer
STADR R64B_10, R64B_6;                                // store that valid pointer at the element of BENCH that needed allocation
ADD R64B_8, R64B_8, R64B_9;                           // increment heap pointer by size of one submatrix so it is valid for next time
COD zero_2048L R2048L_3;                              // we don't need to load this submat since it's fresh but it's expected to be full of 0.0 floats
JMPLBL submat_allocated;                              // jump back to where we were. This is fine since allocate_clean_block is only called in one location

exit:
  // end program
COMMIT;