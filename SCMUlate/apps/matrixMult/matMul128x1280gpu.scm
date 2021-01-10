LDIMM R64B_1, 0; // Loading base address A
LDIMM R64B_2, 1310720; // Loading base address B
LDIMM R64B_3, 2621440; // Loading base address C

LDIMM R64B_4, 0; // For i iteration variable
LDIMM R64B_5, 10; // For number of iterations

LDIMM R64B_8, 1024; // For offset A
LDIMM R64B_9, 131072; // For offset B

COD LoadSqTileGPU_2048L R2048L_3, R64B_3, 128; //Load C
loop:
  BREQ R64B_4, R64B_5, afterLoop;
  ADD R64B_4, R64B_4, 1;
  COD LoadSqTileGPU_2048L R2048L_1, R64B_1, 1280; //Load A
  COD LoadSqTileGPU_2048L R2048L_2, R64B_2, 128; //Load B
  COD MatMultGPU_2048L R2048L_3, R2048L_1, R2048L_2;
  ADD R64B_1, R64B_1, R64B_8; // *A + 1024
  ADD R64B_2, R64B_2, R64B_9; // *B + 131072
  JMPLBL loop;

afterLoop:
COD StoreSqTileGPU_2048L R2048L_3, R64B_3, 128; //Load C

COMMIT;