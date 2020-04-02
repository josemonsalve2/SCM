LDIMM R64B_1, 0; // Loading base address A
LDIMM R64B_2, 52428800; // Loading base address B
LDIMM R64B_3, 104857600; // Loading base address C

LDIMM R64B_4, 0; // For iteration variable
LDIMM R64B_5, 0; // For offset
LDIMM R64B_6, 400; // For number of iterations

loop:
  BREQ R64B_4, R64B_6, 8;
  LDOFF R2048L_1, R64B_1, R64B_5;
  LDOFF R2048L_2, R64B_2, R64B_5;
  COD vecAdd_2048L R2048L_3, R2048L_1, R2048L_2;
  STOFF R2048L_3, R64B_3, R64B_5;
  ADD R64B_4, R64B_4, 1;
  ADD R64B_5, R64B_5, 131072;
  JMPLBL loop;
