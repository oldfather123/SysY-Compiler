#ifndef _SYSY_H
#define _SYSY_H

#include "symbol.h"
#include "value.h"
#include "vector.h"

void       parser();
/**
 * @brief 编译sysy文件
 * @param[in] *fileName
 */
void       compileFile(char *fileName);
/**
 * @brief 初始化sysy编译器及其标准库
 */
void       sysy_init();

/**
 * @brief 初始化sysy优化器pass
 */
void       initPassManager();

/**
 * @brief 将IR编译为riscv汇编代码
 */
void       riscvCodeGenerator();
void       optRiscvCodeGenerator();

/**
 * @brief 作用域级别
 */
extern int scope;

typedef struct SYContext {
    bool isDebug;
    vector_template(struct optimizePass *, passes);
} SYContext;

Type implictionCast(Value *r, Value *l);
void typeCast(Type ty, Value *v);

void arrayAddDimension(Symbol *sym, int size);

#define SET_IMM_FLOAT(val) ((Value){.imm.f = (val), .ty = IS_IMM | TY_FLOAT})
#define SET_IMM_INT(val)   ((Value){.imm.i = (val), .ty = IS_IMM | TY_INT})

#endif