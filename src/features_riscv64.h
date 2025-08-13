// This file is a part of Julia. License is MIT: https://julialang.org/license

// RISC-V 64 features definition
// Based on RISC-V ISA specifications and LLVM RISC-V target features
// See https://github.com/llvm/llvm-project/blob/main/llvm/lib/Target/RISCV/RISCV.td

// RISC-V 64 features definition
// Basic integer features (RV64I)
// JL_FEATURE_DEF(64bit, 0, 0) // RV64I - implied by 64-bit architecture

// Floating point features
JL_FEATURE_DEF(f, 1, 0) // RV32F/RV64F - single precision floating point
JL_FEATURE_DEF(d, 2, 0) // RV64D - double precision floating point
JL_FEATURE_DEF(zfinx, 3, 0) // Zfinx - floating point in integer registers
JL_FEATURE_DEF(zdinx, 4, 0) // Zdinx - double precision in integer registers
JL_FEATURE_DEF(zhinx, 5, 0) // Zhinx - half precision in integer registers
JL_FEATURE_DEF(zhinxmin, 6, 0) // Zhinxmin - minimal half precision in integer registers

// Vector features
JL_FEATURE_DEF(v, 7, 0) // RVV - vector extension
JL_FEATURE_DEF(zve32x, 8, 0) // Zve32x - vector extension for 32-bit elements
JL_FEATURE_DEF(zve32f, 9, 0) // Zve32f - vector extension for 32-bit float
JL_FEATURE_DEF(zve64x, 10, 0) // Zve64x - vector extension for 64-bit elements
JL_FEATURE_DEF(zve64f, 11, 0) // Zve64f - vector extension for 64-bit float
JL_FEATURE_DEF(zve64d, 12, 0) // Zve64d - vector extension for 64-bit double

// Bit manipulation features
JL_FEATURE_DEF(zba, 13, 0) // Zba - address generation instructions
JL_FEATURE_DEF(zbb, 14, 0) // Zbb - basic bit manipulation
JL_FEATURE_DEF(zbc, 15, 0) // Zbc - carry-less multiplication
JL_FEATURE_DEF(zbkb, 16, 0) // Zbkb - bit manipulation (crypto)
JL_FEATURE_DEF(zbkc, 17, 0) // Zbkc - carry-less multiplication (crypto)
JL_FEATURE_DEF(zbkx, 18, 0) // Zbkx - crossbar permutation (crypto)
JL_FEATURE_DEF(zbs, 19, 0) // Zbs - single-bit instructions

// Scalar crypto features
JL_FEATURE_DEF(zknd, 20, 0) // Zknd - AES decryption
JL_FEATURE_DEF(zkne, 21, 0) // Zkne - AES encryption
JL_FEATURE_DEF(zknh, 22, 0) // Zknh - SHA2 hash functions
JL_FEATURE_DEF(zksed, 23, 0) // Zksed - SM4 encryption/decryption
JL_FEATURE_DEF(zksh, 24, 0) // Zksh - SM3 hash function
JL_FEATURE_DEF(zkr, 25, 0) // Zkr - entropy source
JL_FEATURE_DEF(zk, 26, 0) // Zk - scalar cryptography

// Vector crypto features
JL_FEATURE_DEF(zvknha, 27, 0) // Zvknha - vector AES hash
JL_FEATURE_DEF(zvknhb, 28, 0) // Zvknhb - vector SHA2 hash
JL_FEATURE_DEF(zvksed, 29, 0) // Zvksed - vector SM4
JL_FEATURE_DEF(zvksh, 30, 0) // Zvksh - vector SM3
JL_FEATURE_DEF(zvkb, 31, 0) // Zvkb - vector bit manipulation

// Additional vector features
JL_FEATURE_DEF(zvbb, 32 + 0, 0) // Zvbb - vector basic bit manipulation
JL_FEATURE_DEF(zvbc, 32 + 1, 0) // Zvbc - vector carry-less multiplication
JL_FEATURE_DEF(zvfbfmin, 32 + 2, 0) // Zvfbfmin - vector BF16 conversion
JL_FEATURE_DEF(zvfbfwma, 32 + 3, 0) // Zvfbfwma - vector BF16 widening multiply-add
JL_FEATURE_DEF(zvkg, 32 + 4, 0) // Zvkg - vector AES key generation
JL_FEATURE_DEF(zvkned, 32 + 5, 0) // Zvkned - vector AES encryption/decryption
JL_FEATURE_DEF(zvknha, 32 + 6, 0) // Zvknha - vector AES hash
JL_FEATURE_DEF(zvknhb, 32 + 7, 0) // Zvknhb - vector SHA2 hash
JL_FEATURE_DEF(zvksed, 32 + 8, 0) // Zvksed - vector SM4
JL_FEATURE_DEF(zvksh, 32 + 9, 0) // Zvksh - vector SM3

// Compressed instruction features
JL_FEATURE_DEF(c, 32 * 2 + 0, 0) // RVC - compressed instructions
JL_FEATURE_DEF(zca, 32 * 2 + 1, 0) // Zca - compressed instructions (subset)
JL_FEATURE_DEF(zcb, 32 * 2 + 2, 0) // Zcb - compressed instructions (basic)
JL_FEATURE_DEF(zcd, 32 * 2 + 3, 0) // Zcd - compressed instructions (double)
JL_FEATURE_DEF(zcf, 32 * 2 + 4, 0) // Zcf - compressed instructions (float)
JL_FEATURE_DEF(zcmp, 32 * 2 + 5, 0) // Zcmp - compressed instructions (push/pop)
JL_FEATURE_DEF(zcmt, 32 * 2 + 6, 0) // Zcmt - compressed instructions (table jump)

// Atomic memory operations
JL_FEATURE_DEF(a, 32 * 2 + 7, 0) // RVA - atomic memory operations
JL_FEATURE_DEF(zalrsc, 32 * 2 + 8, 0) // Zalrsc - load-reserved/store-conditional
JL_FEATURE_DEF(zacas, 32 * 2 + 9, 0) // Zacas - compare-and-swap

// Memory management features
JL_FEATURE_DEF(m, 32 * 2 + 10, 0) // RVM - integer multiplication and division
JL_FEATURE_DEF(zmmul, 32 * 2 + 11, 0) // Zmmul - multiplication only
JL_FEATURE_DEF(zicbom, 32 * 2 + 12, 0) // Zicbom - cache block operations
JL_FEATURE_DEF(zicbop, 32 * 2 + 13, 0) // Zicbop - cache block prefetch
JL_FEATURE_DEF(zicboz, 32 * 2 + 14, 0) // Zicboz - cache block zero

// Privileged architecture features
JL_FEATURE_DEF(s, 32 * 2 + 15, 0) // Supervisor mode
JL_FEATURE_DEF(u, 32 * 2 + 16, 0) // User mode
JL_FEATURE_DEF(zicntr, 32 * 2 + 17, 0) // Zicntr - base counters
JL_FEATURE_DEF(zihpm, 32 * 2 + 18, 0) // Zihpm - hardware performance monitors
JL_FEATURE_DEF(zicond, 32 * 2 + 19, 0) // Zicond - conditional operations
JL_FEATURE_DEF(zawrs, 32 * 2 + 20, 0) // Zawrs - wait for reservation set
JL_FEATURE_DEF(zfa, 32 * 2 + 21, 0) // Zfa - additional floating point
JL_FEATURE_DEF(zfh, 32 * 2 + 22, 0) // Zfh - half precision floating point
JL_FEATURE_DEF(zfhmin, 32 * 2 + 23, 0) // Zfhmin - minimal half precision
JL_FEATURE_DEF(zfinx, 32 * 2 + 24, 0) // Zfinx - floating point in integer registers
JL_FEATURE_DEF(zdinx, 32 * 2 + 25, 0) // Zdinx - double precision in integer registers
JL_FEATURE_DEF(zhinx, 32 * 2 + 26, 0) // Zhinx - half precision in integer registers
JL_FEATURE_DEF(zhinxmin, 32 * 2 + 27, 0) // Zhinxmin - minimal half precision in integer registers

// Additional features
JL_FEATURE_DEF(zicclsm, 32 * 3 + 0, 0) // Zicclsm - cache line size management
JL_FEATURE_DEF(zicfilp, 32 * 3 + 1, 0) // Zicfilp - instruction fetch with low power
JL_FEATURE_DEF(zicfiss, 32 * 3 + 2, 0) // Zicfiss - instruction fetch with separate store
JL_FEATURE_DEF(zihintntl, 32 * 3 + 3, 0) // Zihintntl - non-temporal locality hints
JL_FEATURE_DEF(zihintpause, 32 * 3 + 4, 0) // Zihintpause - pause hint
JL_FEATURE_DEF(zihwa, 32 * 3 + 5, 0) // Zihwa - hardware assistance
JL_FEATURE_DEF(zimop, 32 * 3 + 6, 0) // Zimop - memory operations
JL_FEATURE_DEF(ziselect, 32 * 3 + 7, 0) // Ziselect - select operations
JL_FEATURE_DEF(ztso, 32 * 3 + 8, 0) // Ztso - total store ordering 
