#include "../include/pass.h"

#include <stdbool.h>

#include "../include/bitset.h"
#include "../include/function.h"
#include "../include/ir.h"
#include "../include/lexer.h"
#include "../include/symbol.h"
#include "../include/sysy.h"
#include "../include/util.h"
#include "../include/value.h"
#include "../include/vector.h"

bool ssaPass(Function *fnc);
/**
 * @brief 值编号pass
 * @param[in] *fnc
 */
bool DVNTPass(Function *fnc);
/**
 * @brief 强度削弱pass
 * @param[in] *fnc
 */
bool StrengthReductionPass(Function *fnc);
/**
 * @brief 死代码消除pass
 * @param[in] *fnc
 */
bool DeadCodeEliminationPass(Function *fnc);
/**
 * @brief 函数常量传播pass
 * @param[in] *fnc
 */
bool functionCopy(Function *fnc);

vector_template(optimizePass *, sysy_passes);

/**
 * @brief 向pass管理器注册一个pass
 * @param[in] *name
 * @param[in] fnc
 */
void registerFunctionPass(const char *name, passFnc_t fnc) {
    optimizePass *ptr = (optimizePass *)sy_malloc(sizeof(optimizePass));
    ptr->name = name;
    ptr->execute = fnc;
    vector_push_back(&sysy_passes, ptr);
}
/**
 * @brief 为函数按顺序运行pass
 * @param[in] *name
 * @param[in] fnc
 */
void executeFunctionPass(Function *fnc) {
    size_t        i;
    optimizePass *pass = NULL;
    vector_each(&sysy_passes, i, pass) {
        fprintf(stderr, "pass %d\n", i);

        pass->execute(fnc);
    }
}

extern bool isOptimimze;

/**
 * @brief 初始化pass管理器
 */
void        initPassManager() {
    // 执行优化顺序如下
    if (isOptimimze) {
        registerFunctionPass("ssa", ssaPass);
        registerFunctionPass("dce", DeadCodeEliminationPass);
        registerFunctionPass("dvnt", DVNTPass);
        registerFunctionPass("dce", DeadCodeEliminationPass);
        registerFunctionPass("functioncopy", functionCopy);

        // 需要先将数组寻址指令转为低级指令
        // registerFunctionPass("OSR", StrengthReductionPass);
    }
}