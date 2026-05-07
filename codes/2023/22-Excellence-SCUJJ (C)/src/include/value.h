#ifndef _VALUE_H
#define _VALUE_H

#include <stdbool.h>
#include <stdint.h>

#include "type.h"
#include "vector.h"

typedef enum latticeTy { lattice_TOP = 1, lattice_BOT, lattice_CON } latticeTy;

typedef vector_template(struct instruction *, LFTRedge);
typedef struct OSRLFTR {
    int      op;
    uint32_t v;
} OSRLFTR;
typedef struct ssaSymbol {
    struct ssaSymbol
        *ReachingDef;  // 此字段一定要在第一个,symbol和ssasymbol公用这个一个字段
    Type           ty;
    uint8_t        arrayMark : 1;  // 死代码删除
    uint8_t        isIV : 1;       // ssa值编号
    uint32_t       valueind;       // ssa值编号或者临时变量编号
    uint32_t       ssaind;  // ssa此符号的索引,用于查找符号和位集
    struct Symbol *sym;
    struct instruction *def;
    struct instruction *Header;
    OSRLFTR            LFTR;
    struct ssaSymbol   *_LFTR;
    struct ssaSymbol   *prev;
    int                 loc;

    bool                visited;
    size_t              Num;
    size_t              Low;
    struct ArrayType   *arrayTy;
    union {
        int32_t
            location;  // stack地址,溢出也放这个地址,这个应该在虚拟寄存器结构
        struct BasicBlock *edge;  // 用于PHI函数,该值来源于那个基本块
    };
    latticeTy lattice;
} ssaSymbol;

typedef struct Value {
    Type ty;
    union {
        struct {
            uint32_t          symbol;
            uint32_t          ssaind;
            int8_t            reg;
            struct arrayAddr *arrayAddr;
        };
        uint32_t VNind;
        union {
            int   i;
            float f;
            char *str;
        } imm;
        bool                isTailRec;
        int                 loc;
        struct phiFunction *phi;
    };
} Value;

typedef struct phiParam {
    bool                isVN;
    Value               v;
    struct BasicBlock  *bb;
    int                 copybb;
    struct instruction *join;
} phiParam;
typedef struct phiFunction {
    vector_template(phiParam, param);  // 为NULL则是第一次
    struct Symbol *symbol;
} phiFunction;

typedef struct arrayAddr {
    vector_template(Value, index);
    struct Symbol *symbol;
} arrayAddr;

bool  ValueIsFloat(const Value *v);
bool  ValueIsArray(const Value *v);
bool  ValueIsInt(const Value *v);
bool  ValueIsImm(const Value *v);
int   getImmIntValue(const Value *v);
float getImmFloatValue(const Value *v);

#endif