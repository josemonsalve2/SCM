LDIMM R64B_1, 0; // For iteration variable -- kk
LDIMM R64B_2, 0; // For iteration variable -- jj
LDIMM R64B_3, 0; // For iteration variable -- ii
LDIMM R64B_4, 50;  // For number of iterations and offset calculation -- bot_arg_size
COD loadBenchAddress_64B R64B_5; // Basically a hacky way of fetching BENCH address so we can manipulate it to check if submatrices are NULL or not
LDIMM R64B_7, 0; // stays 0 for null checking of submatrices

// ALL the offset calculation to check for NULL submatrices needs to be updated
// Check that they add to the BENCH address in R64B_5 AND that they multiply by 8 bytes; assembly does not know about pointer arithmetic

// main outer loop -- kk 
outer:
  BREQ R64B_4, R64B_1, exit;
  COD loadSubMat_2048L R2048L_1, R64B_1, R64B_1;      //Load submat for lu0 with both offsets kk
  COD lu0_2048L R2048L_1;                             //perform lu0 codelet -- r/w on R1
                                                      // first inner loop with iteration variable jj starting at kk+1
  ADD R64B_2, R64B_1, 1; 
  inner_jj:
    BREQ R64B_4, R64B_2, inner_ii_head;               // branch for this loop
    MULT R64B_6, R64B_1, R64B_4;                      // kk * bot_arg_size
    ADD R64B_6, R64B_6, R64B_2;                       // kk * bot_arg_size + jj
    MULT R64B_6, R64B_6, 8;
    LDOFF R64B_6, R64B_5, R64B_6;                     // BENCH[kk * bot_arg_size + jj]
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
    MULT R64B_6, R64B_6, 8;
    LDOFF R64B_6, R64B_5, R64B_6;                     // BENCH[ii * bot_arg_size + kk]
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
    MULT R64B_6, R64B_6, 8;
    LDOFF R64B_6, R64B_5, R64B_6;                     // BENCH[ii * bot_arg_size + kk]
    BREQ R64B_6, R64B_7, inner_ii2_tail;              // branch if submatrix is null -- branch to tail of loop
    ADD R64B_2, R64B_1, 1;                            // jj = kk + 1
    inner2_jj:
      BREQ R64B_4, R64B_2, inner_ii2_tail;            // branch for this loop
      // need to calculate offset with kk and jj
      MULT R64B_6, R64B_1, R64B_4;                    // kk * bot_arg_size
      ADD R64B_6, R64B_6, R64B_2;                     // kk * bot_arg_size + jj
      MULT R64B_6, R64B_6, 8;
      LDOFF R64B_6, R64B_5, R64B_6;                   // BENCH[kk * bot_arg_size + jj]
      BREQ R64B_6, R64B_7, inner2_jj_tail;            // branch if submatrix is null -- branch to tail of loop
      // removed checking for last submatrix because it should've been preallocated
      // need to load ii/kk, kk/jj, then do bmod, then store all 3 (including ii/jj)
      COD loadSubMat_2048L R2048L_1, R64B_3, R64B_1;  // loading submat at BENCH[ii*bot_arg_size+kk]
      COD loadSubMat_2048L R2048L_2, R64B_1, R64B_2;  // loading submat at BENCH[kk*bot_arg_size+jj]
      COD bmod_2048L R2048L_3, R2048L_1, R2048L_2;    // execute bmod codelet
      COD storeSubMat_2048L R2048L_1, R64B_3, R64B_1; // storing submat at BENCH[ii*bot_arg_size+kk]
      COD storeSubMat_2048L R2048L_2, R64B_1, R64B_2; // storing submat at BENCH[kk*bot_arg_size+jj]
      COD storeSubMat_2048L R2048L_3, R64B_3, R64B_2;
    inner2_jj_tail:
      ADD R64B_2, R64B_2, 1;                          // increment jj
      JMPLBL inner2_jj;
  inner_ii2_tail:
    ADD R64B_3, R64B_3, 1;                            // increment ii
    JMPLBL inner_ii2;
outer_tail:
  ADD R64B_1, R64B_1, 1;                              // increment kk
  JMPLBL outer;

exit:
  // end program
COMMIT;