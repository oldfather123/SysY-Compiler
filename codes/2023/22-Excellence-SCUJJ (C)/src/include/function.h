#ifndef _FUNCTION_H
#define _FUNCTION_H

#include <stdbool.h>
#include <stdio.h>

#include "bitset.h"
#include "symbol.h"
#include "sysy.h"
#include "utlist.h"
#include "vector.h"

typedef struct BasicBlock {
    struct instruction *instlist;
    vector_template(struct instruction *, tmplist);
    vector_template(struct instruction *, inst);
    vector_template(struct BasicBlock *, df);      // 支配边界
    vector_template(struct BasicBlock *, postdf);  // 反向支配边界
    vector_template(struct BasicBlock *, inedges);

    uint32_t inNum;

    vector_template(struct BasicBlock *, doms);
    struct BasicBlock *idom;
    struct BasicBlock *dom;
    int                domind;

    vector_template(struct BasicBlock *, postdoms);
    struct BasicBlock *postidom;
    struct BasicBlock *postdom;
    int                postdomind;

    uint8_t            marked : 1;   // 遍历数据结构访问信息
    uint8_t            visited : 1;  // 遍历数据结构访问信息
    uint8_t            isLoop : 1;
    struct BasicBlock *semi;
    struct BasicBlock *ancestor;
    int                dfnum;
    uint32_t           bbName;
    struct BasicBlock *parent;
    struct BasicBlock *nextblock;
    struct BasicBlock *branchblock;

    bool               isloop;
    struct BasicBlock *backblock;

    // regitser alloc
    bitset_t           live;

    bool               bb1unexecuted;
    bool               bb2unexecuted;

    bitset_t           liveOut;
    bitset_t           liveIn;
    bitset_t           LiveUse;
    bitset_t           LiveDef;
} BasicBlock;
typedef struct FunctionType {
    Type ret;                           // sysy函数只会返回基本类型
    vector_template(Symbol *, params);  // tok字段可能为0,函数声明
} FunctionType;

/**
 * @brief IR中值的状态
 */
typedef enum ValueState { VAL_SSA, VAL_GVN } ValueState;

__attribute((constructor)) void before_main();
__attribute((destructor)) void  after_main();

typedef enum FncAttribute { ATTR_constructor, ATTR_destructor } FncAttribute;

typedef struct Function {
    uint8_t      isExtern : 1;
    uint8_t      isCopy : 1;
    uint8_t      insert : 1;

    uint8_t      overParam : 1;

    FncAttribute attribute;
    ValueState   vState;
    FunctionType fncTy;
    Symbol      *symbol;
    char        *name;
    BasicBlock  *firstBB;
    BasicBlock  *finalBB;
    vector_template(Symbol *, params);

    vector_template(BasicBlock *, BBs);
    BasicBlock *currentBB;

    vector_template(Symbol *, symPool);
    vector_template(ssaSymbol *, SSAValuePool);

    vector_template(BasicBlock *, reverse_bb_list);
    bool   instNumbering;
    bool   isReg;
    size_t tmpVariableNum;
    int    loc;
    int    loc1;
    void  *asmlist;
    int   *stackSize;
    int   *raAddr;
} Function;

/**
 * @brief 创建一个函数
 * @param[in] *context
 * @param[in] *sym
 */
Function         *createFunction(SYContext *context, Symbol *sym);

/**
 * @brief 对函数进行优化
 * @param[in] *fn
 */
void              optimize(Function *fnc);

/**
 * @brief 打印基本块
 * @param[in] *fp
 * @param[in] *fnc
 * @param[in] *bb
 */
void              printBasicBlock(FILE *fp, Function *fnc, BasicBlock *bb);
void              printBasicAllBlock(FILE *fp, Function *fnc);
Symbol           *getLocalVariable(Function *fnc, size_t i);
struct ssaSymbol *getSSAlVariable(Function *fnc, size_t i);
/**
 * @brief 打印出此函数的Markdown格式的控制流图
 * @param[in] *fnc
 */
void              printCFG(Function *fnc);
#endif