LDIMM R64B_1, 0;     //

// MATRIX DIMMENSIONS
LDOFF R64B_10, R64B_1, 0;   // Load M in tiles
LDOFF R64B_11, R64B_1, 8;   // Load N in tiles
LDOFF R64B_12, R64B_1, 16;  // Load K in tiles

// MATRIX ADDRESSES IN MEMORY
LDOFF R64B_13, R64B_1, 24;  // Load Address_a 
LDOFF R64B_14, R64B_1, 32;  // Load Address_b
LDOFF R64B_15, R64B_1, 40;  // Load Address_c

// OFFSETS Tiles
LDOFF R64B_16, R64B_1, 48;  // Load Off_cbj in bytes
LDOFF R64B_17, R64B_1, 56;  // Load Off_aj in bytes
LDOFF R64B_18, R64B_1, 64;  // Load Off_bk in bytes
LDOFF R64B_19, R64B_1, 72;  // Load Off_ak in bytes
LDOFF R64B_20, R64B_1, 80;  // Load Off_cj in bytes

// Offsets for third arg of matmul
LDOFF R64B_21, R64B_1, 88;  // Load Off_a in elements
LDOFF R64B_22, R64B_1, 96;  // Load Off_b in elements
LDOFF R64B_23, R64B_1, 104;  // Load Off_c in elements

// ITERATION VARIABLES
LDIMM R64B_2, 0; // For i iteration variable. Move tile by tile
LDIMM R64B_3, 0; // For j iteration variable. Move tile by tile
LDIMM R64B_4, 0; // For k iteration variable. Move tile by tile


// Offset pointing to the first value of the tile in mem
LDIMM R64B_5, 0; // For A_off_cnt iteration variable. Points to beginning of tile in bytes
LDIMM R64B_6, 0; // For B_off_cnt iteration variable. Points to beginning of tile in bytes
LDIMM R64B_7, 0; // For C_off_cnt iteration variable. Points to beginning of tile in bytes

ADD R64B_5, R64B_5, R64B_13; // Initialize it to the beginning of A
ADD R64B_6, R64B_6, R64B_14; // Initialize it to the beginning of A
ADD R64B_7, R64B_7, R64B_15; // Initialize it to the beginning of A

// References to the beginning of row to restart inner counter
ADD R64B_24, R64B_24, R64B_13; // ref to beginning of the row in A
ADD R64B_25, R64B_25, R64B_14; // ref to beginning of the row in B
ADD R64B_26, R64B_26, R64B_15; // ref to beginning of the row in C

// We are using j for the rows (A and C) and i for the cols (B and C)

loop_j:
  BREQ R64B_3, R64B_10, after_loop_j; // if (j == M) jump out of loop
  ADD R64B_3, R64B_3, 1; // j++
  loop_k:
    BREQ R64B_4, R64B_12, after_loop_k; // if (k == K) jump out of loop
    ADD R64B_4, R64B_4, 1; // k++

    // Load the tile of A
    COD LoadSqTile_2048L R2048L_1, R64B_5, R64B_21; //Load A tile

    loop_i:
      BREQ R64B_2, R64B_11, after_loop_i; // if (i == N) jump out of loop
      ADD R64B_2, R64B_2, 1; // i++

      // Load tiles of B and C
      COD LoadSqTile_2048L R2048L_2, R64B_6, R64B_22; // Load B tile
      COD LoadSqTile_2048L R2048L_3, R64B_7, R64B_23; // Load C tile

      // Do actual MM
      COD MatMult_2048L R2048L_3, R2048L_1, R2048L_2;

      // Store partial result of C
      COD StoreSqTile_2048L R2048L_3, R64B_7, R64B_23; // Store C tile

      // Move on i tile by tile over B and C
      ADD R64B_6, R64B_6, R64B_16; // Move B along i. Increase by tile size in row major
      ADD R64B_7, R64B_7, R64B_16; // Move C along i. Increase by tile size in row major

      JMPLBL loop_i; // go to beginning of loop

    after_loop_i:

    // Advance A, tile by tile, over K.
    ADD R64B_5, R64B_5, R64B_19; // *A + Off_ak (Increase by tile size in row major)

    // Advance B, tile by tile, over K. Reset the 
    // pointer that moves B, tile by tile, over i direction
    ADD R64B_25, R64B_25, R64B_18; // Set to beginning of B for new k
    LDIMM R64B_6, 0; // Reset inner count for B along i
    ADD R64B_6, R64B_6, R64B_25; // Set count to beginning

    // Reset the counter for C, will stay in the same j
    LDIMM R64B_7, 0; // Reset inner count C
    ADD R64B_7, R64B_7, R64B_26; // Set to beginning of C in same j

    JMPLBL loop_k;

  after_loop_k:

  // Advance A over j a whole tile, and Reset the 
  // pointer that moves A tile by tile over k
  ADD R64B_24, R64B_24, R64B_17; // Move A along j. (increase by tile size*tile size*K in row major)
  LDIMM R64B_5, 0; // Reset inner count for A
  ADD R64B_5, R64B_5, R64B_24; // Set to beginning of A

  // Reset B to the beginning of the B matrix, then reset counters
  LDIMM R64B_25, 0; // Reset the Ref
  ADD R64B_25, R64B_25, R64B_14; // Move ref to beginning of B
  LDIMM R64B_6, 0; // Reset inner count for B along i
  ADD R64B_6, R64B_6, R64B_25; // Set to beginning of matrix B

  // Advance C over j a whole tile, and reset pointer that moves
  // C tile by tile over i
  ADD R64B_26, R64B_26, R64B_20; // Move C along j
  LDIMM R64B_7, 0; // Reset inner count C
  ADD R64B_7, R64B_7, R64B_26; // Set to beginning of C at j

  JMPLBL loop_j;
after_loop_j:



COMMIT;

