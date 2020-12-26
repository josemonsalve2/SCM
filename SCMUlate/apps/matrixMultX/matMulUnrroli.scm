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
  BREQ R64B_3, R64B_10, 72; // i + 14
  ADD R64B_3, R64B_3, 1; // j++
  loop_i:
    BREQ R64B_2, R64B_11, 58; // k + 16
    ADD R64B_2, R64B_2, 1; // i++
    COD LoadSqTile_2048L R2048L_3, R64B_7, R64B_23; //Load C
    COD LoadSqTile_2048L R2048L_4, R64B_7, R64B_23; //Load C
    COD LoadSqTile_2048L R2048L_5, R64B_7, R64B_23; //Load C
    COD LoadSqTile_2048L R2048L_6, R64B_7, R64B_23; //Load C
    COD LoadSqTile_2048L R2048L_7, R64B_7, R64B_23; //Load C


    loop_k:
      BREQ R64B_4, R64B_12, 42; // it was 8
      ADD R64B_4, R64B_4, 4; // k++
      COD LoadSqTile_2048L R2048L_1, R64B_5, R64B_21; //Load A
      COD LoadSqTile_2048L R2048L_2, R64B_6, R64B_22; //Load B
      COD MatMult_2048L R2048L_4, R2048L_1, R2048L_2;
      ADD R64B_5, R64B_5, R64B_19; // *A + Off_ak
      ADD R64B_6, R64B_6, R64B_18; // *B + Off_bk
      LDIMM R64B_27, 0;
      LDIMM R64B_28, 0;
      ADD R64B_27, R64B_27, R64B_5; // *A + Off_ak
      ADD R64B_27, R64B_27, R64B_19; // *A + Off_ak
      ADD R64B_28, R64B_28, R64B_6; // *A + Off_ak
      ADD R64B_28, R64B_28, R64B_18; // *A + Off_ak
      LDIMM R64B_29, 0;
      LDIMM R64B_30, 0;
      ADD R64B_29, R64B_29, R64B_27; // *A + Off_ak
      ADD R64B_29, R64B_29, R64B_19; // *A + Off_ak
      ADD R64B_30, R64B_30, R64B_28; // *A + Off_ak
      ADD R64B_30, R64B_30, R64B_18; // *A + Off_ak
      LDIMM R64B_31, 0;
      LDIMM R64B_32, 0;
      ADD R64B_31, R64B_31, R64B_29; // *A + Off_ak
      ADD R64B_31, R64B_31, R64B_19; // *A + Off_ak
      ADD R64B_32, R64B_32, R64B_28; // *A + Off_ak
      ADD R64B_32, R64B_32, R64B_30; // *A + Off_ak

      COD LoadSqTile_2048L R2048L_1, R64B_27, R64B_21; //Load A
      COD LoadSqTile_2048L R2048L_2, R64B_28, R64B_22; //Load B
      COD MatMult_2048L R2048L_5, R2048L_1, R2048L_2;

      COD LoadSqTile_2048L R2048L_1, R64B_29, R64B_21; //Load A
      COD LoadSqTile_2048L R2048L_2, R64B_30, R64B_22; //Load B
      COD MatMult_2048L R2048L_6, R2048L_1, R2048L_2;

      COD LoadSqTile_2048L R2048L_1, R64B_31, R64B_21; //Load A
      COD LoadSqTile_2048L R2048L_2, R64B_32, R64B_22; //Load B
      COD MatMult_2048L R2048L_7, R2048L_1, R2048L_2;

      COD MatMultReduc_2048L R2048L_4, R2048L_4, R2048L_5;
      COD MatMultReduc_2048L R2048L_6, R2048L_6, R2048L_7;
      COD MatMultReduc_2048L R2048L_3, R2048L_4, R2048L_6;

      LDIMM R64B_5, 0;
      LDIMM R64B_6, 0;
      ADD R64B_5, R64B_5, R64B_31; // *A + Off_ak
      ADD R64B_6, R64B_6, R64B_32; // *A + Off_ak

      JMPLBL loop_k;

    COD StoreSqTile_2048L R2048L_3, R64B_7, R64B_23; //Store C

    LDIMM R64B_4, 0; // Reset k index loop
    ADD R64B_7, R64B_7, R64B_16; // Move C along i
    ADD R64B_25, R64B_25, R64B_16; // Move B along i
    LDIMM R64B_5, 0; // Reset inner count for A
    LDIMM R64B_6, 0; // Reset inner count for B
    ADD R64B_5, R64B_5, R64B_24; // Set to beginning of A same j
    ADD R64B_6, R64B_6, R64B_25; // Set to beginning of new i
    JMPLBL loop_i;

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


COMMIT;