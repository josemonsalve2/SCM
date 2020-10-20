LDIMM R64B_1, 0;     // 
LDOFF R64B_10, R64B_1, 0;    // Load M
LDOFF R64B_11, R64B_1, 64;   // Load N
LDOFF R64B_12, R64B_1, 128;  // Load K
LDOFF R64B_13, R64B_1, 192;  // Load Add_a
LDOFF R64B_14, R64B_1, 276;  // Load Add_b
LDOFF R64B_15, R64B_1, 320;  // Load Add_c
LDOFF R64B_16, R64B_1, 384;  // Load Off_cbi
LDOFF R64B_17, R64B_1, 448;  // Load Off_aj
LDOFF R64B_18, R64B_1, 512;  // Load Off_bk
LDOFF R64B_19, R64B_1, 576;  // Load Off_ak
LDOFF R64B_20, R64B_1, 640;  // Load Off_cj
LDOFF R64B_21, R64B_1, 704;  // Load Off_a
LDOFF R64B_22, R64B_1, 768;  // Load Off_b
LDOFF R64B_23, R64B_1, 832;  // Load Off_c

LDIMM R64B_4, 0; // For k iteration variable

COD LoadSqTile_2048L R2048L_3, R64B_15, R64B_23; //Load C
loop:
  BREQ R64B_4, R64B_12, 8;
  ADD R64B_4, R64B_4, 1; // k++
  COD LoadSqTile_2048L R2048L_1, R64B_13, R64B_21; //Load A
  COD LoadSqTile_2048L R2048L_2, R64B_14, R64B_22; //Load B
  COD MatMult_2048L R2048L_3, R2048L_1, R2048L_2;
  ADD R64B_13, R64B_13, R64B_19; // *A + Off_ak
  ADD R64B_14, R64B_14, R64B_18; // *B + Off_bk
  JMPLBL loop;

COD StoreSqTile_2048L R2048L_3, R64B_15, R64B_23; //Load C

COMMIT;