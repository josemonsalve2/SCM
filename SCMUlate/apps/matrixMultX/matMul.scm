LDIMM R64B_1, 0;     // 
LDOFF R64B_10, R64B_1, 0;    // Load M
LDOFF R64B_11, R64B_1, 8;   // Load N
LDOFF R64B_12, R64B_1, 16;  // Load K
LDOFF R64B_13, R64B_1, 24;  // Load Add_a
LDOFF R64B_14, R64B_1, 32;  // Load Add_b
LDOFF R64B_15, R64B_1, 40;  // Load Add_c
LDOFF R64B_16, R64B_1, 48;  // Load Off_cbi
LDOFF R64B_17, R64B_1, 56;  // Load Off_aj
LDOFF R64B_18, R64B_1, 64;  // Load Off_bk
LDOFF R64B_19, R64B_1, 72;  // Load Off_ak
LDOFF R64B_20, R64B_1, 80;  // Load Off_cj
LDOFF R64B_21, R64B_1, 88;  // Load Off_a
LDOFF R64B_22, R64B_1, 96;  // Load Off_b
LDOFF R64B_23, R64B_1, 104;  // Load Off_c

LDIMM R64B_2, 0; // For i iteration variable
LDIMM R64B_3, 0; // For j iteration variable
LDIMM R64B_4, 0; // For k iteration variable

LDIMM R64B_5, 0; // For A_off_cnt iteration variable
LDIMM R64B_6, 0; // For B_off_cnt iteration variable
LDIMM R64B_7, 0; // For C_off_cnt iteration variable

ADD R64B_5, R64B_5, R64B_13; // A For the inner K offset 
ADD R64B_6, R64B_6, R64B_14; // B For the inner K offset 
ADD R64B_7, R64B_7, R64B_15; // C For the inner J offset 

ADD R64B_24, R64B_24, R64B_13; // A For the outer J offset
ADD R64B_25, R64B_25, R64B_14; // B for the outer I offset
ADD R64B_26, R64B_26, R64B_15; // C for the outer I offset

// We are using j for the rows (A and C) and i for the cols (B and C)

loop_j:
  BREQ R64B_3, R64B_10, after_loop_j;
  ADD R64B_3, R64B_3, 1; // j++
  loop_i:
    BREQ R64B_2, R64B_11, after_loop_i;
    ADD R64B_2, R64B_2, 1; // i++
    COD LoadSqTile_2048L R2048L_3, R64B_7, R64B_23; //Load C

    loop_k:
      BREQ R64B_4, R64B_12, after_loop_k;
      ADD R64B_4, R64B_4, 1; // k++
      COD LoadSqTile_2048L R2048L_1, R64B_5, R64B_21; //Load A
      COD LoadSqTile_2048L R2048L_2, R64B_6, R64B_22; //Load B
      COD MatMult_2048L R2048L_3, R2048L_1, R2048L_2;
      ADD R64B_5, R64B_5, R64B_19; // *A + Off_ak
      ADD R64B_6, R64B_6, R64B_18; // *B + Off_bk
      JMPLBL loop_k;

after_loop_k:
    COD StoreSqTile_2048L R2048L_3, R64B_7, R64B_23; //Store C

    LDIMM R64B_4, 0; // Reset k index loop
    ADD R64B_7, R64B_7, R64B_16; // Move C along i
    ADD R64B_25, R64B_25, R64B_16; // Move B along i
    LDIMM R64B_5, 0; // Reset inner count for A
    LDIMM R64B_6, 0; // Reset inner count for B
    ADD R64B_5, R64B_5, R64B_24; // Set to beginning of A same j
    ADD R64B_6, R64B_6, R64B_25; // Set to beginning of new i
    JMPLBL loop_i;

after_loop_i:
  LDIMM R64B_2, 0; // Reset i index loop
  ADD R64B_24, R64B_24, R64B_17; // Move A along j
  ADD R64B_26, R64B_26, R64B_20; // Move C along j
  LDIMM R64B_7, 0; // Reset inner count C
  LDIMM R64B_25, 0; // Reset inner count B
  ADD R64B_7, R64B_7, R64B_26; // Set to beginning of C 
  ADD R64B_25, R64B_25, R64B_14; // Reset B completely
  LDIMM R64B_5, 0; // Reset inner count for A
  LDIMM R64B_6, 0; // Reset inner count for B
  ADD R64B_5, R64B_5, R64B_24; // Set to beginning of A same j
  ADD R64B_6, R64B_6, R64B_25; // Set to beginning of new i
  JMPLBL loop_j;

after_loop_j:

COMMIT;