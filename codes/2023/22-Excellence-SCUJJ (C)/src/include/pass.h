#ifndef _PASS_H
#define _PASS_H

#include <stdbool.h>

#include "function.h"
#include "sysy.h"

typedef bool (*passFnc_t)(Function *);

typedef struct optimizePass {
    const char *name;
    passFnc_t   execute;
} optimizePass;

/**
 * @brief 注册一个pass
 * @param[in] *name
 * @param[in] fnc
 */
void registerFunctionPass(const char *name, passFnc_t fnc);

/**
 * @brief 为一个函数执行所有的pass
 * @param[in] *context
 * @param[in] *fnc
 */
void executeFunctionPass(Function *fnc);

/**
 * @brief 构建入边,为反支配树做准备,其他pass可能破坏inedges,每次需要重新构建
 * @param[in] *fn
 * @param[in] *root
 */
void buildInedges(BasicBlock *parent, BasicBlock *bb);
void initInedges(Function *fnc);

#endif