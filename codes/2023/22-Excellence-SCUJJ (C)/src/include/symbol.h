#ifndef _SYMBOL_H
#define _SYMBOL_H

#include <stdbool.h>
#include <stdint.h>

#include "m-bitset.h"
#include "type.h"
#include "vector.h"

typedef struct Symbol {
    struct ssaSymbol *ReachingDef;
    // 基本类型
    Type              type;
    uint8_t           isTemp : 1;  // 临时变量信息
    uint8_t           isInit : 1;
    uint8_t           isalloc : 1;
    uint8_t           marked : 1;
    uint8_t           dcemarked : 1;
    uint8_t           arraymarked : 1;
    uint8_t           iszero : 1;     // 数组{}
    uint8_t           isPointer : 1;  // 数组指针

    uint32_t          tok;  // token的字符串信息
    uint32_t          ind;
    int               loc;
    ArrayType        *arrayTy;  // 复合类型
    struct Function  *fnc;
    union {
        uint32_t tmpInd;
        uint32_t count;  // ssa version number

        int      i;  // global init or const init
        float    f;
    };
    struct instruction *def;
    bitset_t            bbset;
    uint8_t             setinit : 1;
    uint8_t             scope;  // 作用域
    struct Symbol      *next;   // stack
    struct Symbol      *prev;   // 上一级定义标识符
} Symbol;

extern Symbol *globalStack;
extern Symbol *localStack;
extern Symbol *tok_starttime;
extern Symbol *tok_stoptime;

extern Symbol *tok_sysy_starttime;
extern Symbol *tok_sysy_stoptime;
/**
 * @brief 符号表压入一个符号
 * @param[in] tok
 */
Symbol        *pushSymbol(uint32_t tok, Type *ty);
/**
 * @brief 退出作用域时，弹出标识符
 * @param[in] *s
 */
void           popSymbol(Symbol *s);
/**
 * @brief 一个标识符是否被定义
 * @param[in] tok
 */
Symbol        *symbolSearch(uint32_t tok);

Symbol        *getGlobalSymbol(uint32_t ind);

#endif