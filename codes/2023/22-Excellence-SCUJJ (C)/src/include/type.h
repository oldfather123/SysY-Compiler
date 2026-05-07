#ifndef _TYPE_H
#define _TYPE_H

#include <stdio.h>

typedef enum sy_type {
    TY_INT = 0x1,
    TY_FLOAT = 0x2,
    TY_VOID = 0x4,
} sy_type;

#define BTYPE_MASK 0x7

typedef enum Type {
    IS_IMM = 0x10,
    IS_VAR = 0x20,
    IS_FNC = 0x40,
    IS_ARRAY = 0x80,
    IS_COSNT = 0x100,
    IS_SSA = 0x200,
    IS_GLOBAL = 0x400,
    IS_POINTER = 0x800,
    IS_REG = 0x1000,
} Type;

typedef struct ArrayType {
    int               size;
    struct ArrayType *next;
} ArrayType;

#endif