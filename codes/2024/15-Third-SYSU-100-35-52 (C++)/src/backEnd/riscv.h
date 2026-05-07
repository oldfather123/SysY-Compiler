#ifndef RISCV_H
#define RISCV_H

#include "Module.h"
#include "general.h"

using namespace std;

union float2int;
struct MOperand;
struct MInst;
struct MBlock;
struct MFunc;
struct MProg;
struct MIUse;
extern MFunc* curFunc;
extern float2int f2i;
extern bool stillOptimize;

union float2int {
    int intValue;
    float floatValue;
};

// 整数寄存器
enum IReg : uint8 {
    x0 = 0,
    x1 = 1,
    x2 = 2,
    x3 = 3,
    x4 = 4,
    x5 = 5,
    x6 = 6,
    x7 = 7,
    x8 = 8,
    x9 = 9,
    x10 = 10,
    x11 = 11,
    x12 = 12,
    x13 = 13,
    x14 = 14,
    x15 = 15,
    x16 = 16,
    x17 = 17,
    x18 = 18,
    x19 = 19,
    x20 = 20,
    x21 = 21,
    x22 = 22,
    x23 = 23,
    x24 = 24,
    x25 = 25,
    x26 = 26,
    x27 = 27,
    x28 = 28,
    x29 = 29,
    x30 = 30,
    x31 = 31,
    REG_COUNT = 32,
    pc = 33,
    // alias
    zero = 0,  // hardwired to 0, ignores writes
    ra = x1,
    sp = x2,
    gp = x3,  // global pointer
    tp = x4,  // thread pointer
    t0 = x5,
    t1 = x6,
    t2 = x7,
    fp = x8,
    s0 = x8,
    s1 = x9,
    a0 = x10,  // return value or functional argument 0
    a1 = x11,  // return value or functional argument 1
    a2 = x12,  // functional argument 2
    a3 = x13,  // functional argument 3
    a4 = x14,  // functional argument 4
    a5 = x15,  // functional argument 5
    a6 = x16,  // functional argument 6
    a7 = x17,  // functional argument 7
    s2 = x18,
    s3 = x19,
    s4 = x20,
    s5 = x21,
    s6 = x22,
    s7 = x23,
    s8 = x24,
    s9 = x25,
    s10 = x26,
    s11 = x27,
    t3 = x28,
    t4 = x29,
    t5 = x30,
    t6 = x31
};

// 浮点数寄存器
enum FReg : uint8 {
    f0 = 0,
    f1 = 1,
    f2 = 2,
    f3 = 3,
    f4 = 4,
    f5 = 5,
    f6 = 6,
    f7 = 7,
    f8 = 8,
    f9 = 9,
    f10 = 10,
    f11 = 11,
    f12 = 12,
    f13 = 13,
    f14 = 14,
    f15 = 15,
    f16 = 16,
    f17 = 17,
    f18 = 18,
    f19 = 19,
    f20 = 20,
    f21 = 21,
    f22 = 22,
    f23 = 23,
    f24 = 24,
    f25 = 25,
    f26 = 26,
    f27 = 27,
    f28 = 28,
    f29 = 29,
    f30 = 30,
    f31 = 31,
    SREG_COUNT = 32,
    // alias
    fa0 = f10,  // return value or functional argument 0
    fa1 = f11,  // return value or functional argument 1
    fa2 = f12,  // functional argument 2
    fa3 = f13,  // functional argument 3
    fa4 = f14,  // functional argument 4
    fa5 = f15,  // functional argument 5
    fa6 = f16,  // functional argument 6
    fa7 = f17,  // functional argument 7
};

// 操作数类型
enum OperandTag {
    ERRORTYPE,
    IREG,    // 整数寄存器
    VIREG,   // 虚拟整数寄存器
    FREG,    // 浮点数寄存器
    VFREG,   // 虚拟浮点数寄存器
    IMM,     // 立即数
    GLO_ADR  // 全局地址(label)
};

// 指令类型
enum MITag {
    MI_BINARY,
    MI_BRANCH,
    MI_SLT,
    MI_LOAD,
    MI_STORE,
    MI_PUSH,
    MI_POP,
    MI_CLZ,
    MI_ZEXT,
    MI_FUNC_CALL,
    MI_RETURN,
    MI_COMPARE,
    MI_MOVE,
    // 浮点数指令
    MI_FBINARY,
    MI_FLOAD,
    MI_FSTORE,
    MI_FPUSH,
    MI_FPOP,
    MI_FCVT,
    MI_FCMP,
    MI_FMOVE
};

// 二元运算类型
enum BinaryOpType {
    BINARY_ADD,      // +
    BINARY_SUB,      // -
    BINARY_MUL,      // *
    BINARY_DIV,      // /
    BINARY_MOD,      // %
    BINARY_SLL2ADD,  // l << 2 + r
    F_NEG,           // -
    BINARY_SLL,      // sll
    BINARY_SRL,      // srl
    BINARY_SRA,      // sra
    BINARY_AND,      // and
    BINARY_OR,       // or
    BINARY_XOR,      // ^
    UNARY_XOR,       // !
    BINARY_LT,       // <
    BINARY_GT,       // >
    BINARY_NE,       // !=
    BINARY_GE,       // >=
    BINARY_LE,       // <=
    BINARY_EQ,       // ==
    BINARY_FT,       // float
};

// 分支类型
enum BranchCondition {
    BR_JU,  // jump
    BR_LT,  // <
    BR_GT,  // >
    BR_NE,  // !=
    BR_GE,  // >=
    BR_LE,  // <=
    BR_EQ,  // ==
    BR_FT   // float
};

// slt类型
enum SLTTag {
    SLT,    // < reg
    SLT_I,  // < imm
    SLT_U,  // < unsigned reg
    SLT_IU  // < unsigned imm
};

// load & store 类型
enum MemTag { MEM_UNDEFINED, MEM_LOAD_ARG, MEM_LOAD_GLOBAL_REF, MEM_LOAD_SPILL, MEM_SAVE_SPILL, MEM_PREP_ARG };

// 整数浮点数转换
enum FcvtType {
    U32,  // 无符号32位整数
    S32,  // 有符号32位整数
    F32   // 浮点数
};

// 用于构建 machine IR
MOperand makeImm(int64 constant);
MOperand makeIReg(uint8 IReg);
MOperand makeVIReg(int32 vIRegIndex);
MOperand makeFReg(uint8 FReg);
MOperand makeVFReg(int32 vFRegIndex);
MOperand makeOperand(Value* irV, MBlock* mb, bool notImm);
MOperand makeLegalImm(int32 constant, MBlock* mb);
BranchCondition biOp2BrCond(BinaryOpType biOpType);
void emitLoadOfLaterArgs(Value* irV, MOperand vReg, MBlock* mb, int curOffset, int argSize, bool isFloat);
void emitLoadOfGlobalRef(Value* irV, MOperand vIReg, MBlock* mb);
void emitMove(MOperand mvDst, MOperand mvSrc, MBlock* mb, MInst* mi = nullptr);
void emitFmove(MOperand mvDst, MOperand mvSrc, MBlock* mb, MInst* mi = nullptr);
void emitBranch(BranchCondition brCond, BrInstruction* brIRI, MBlock* mb, MOperand brLhs, MOperand brRhs, bool isFloat);
void emitInstruction(Instruction* irI, BasicBlock* irB, MBlock* mb, int32& i);
void emitFunction(Function* irF, int idx, vector<VariablePtr>& globalValues);
MProg* emitProg(Module& programModule);
// 用于优化 machine IR
void optimizeMachineIR(MProg* mProg);
// 用于将 machiine IR 翻译成 RISC-V
BranchCondition invertBranchCond(BranchCondition cond);
BranchCondition invertCmpCond(BranchCondition cond);
void buildCvtType(StringBuilder* s, FcvtType t);
const char* buildBranchSuffix(BranchCondition condition);
void buildOperand(StringBuilder* s, MOperand op);
void buildInstruction(StringBuilder* s, MInst* I, MBlock* nextBb, MFunc* func);
void buildFunction(StringBuilder* s, MFunc* func);
void getInitSequence(Variable* gloVal, vector<pair<string, int>>& initInst);
void buildGlobals(StringBuilder* s, vector<VariablePtr>& globalVariables);
void buildProgram(StringBuilder* s, MProg* prog, vector<VariablePtr>& globalVariables, bool shouldAddCacheLookup);
// 辅助函数
bool canBeImm12(int32 x);
bool IIsCalleeSave(uint8 reg);
bool IIsCallerSave(uint8 reg);
bool FIsCalleeSave(uint8 reg);
bool FIsCallerSave(uint8 sreg);
Array<MOperand> getDefs(MInst* I);
Array<MOperand> IGetDefs(MInst* I);
Array<MOperand> FGetDefs(MInst* I);
Array<MOperand> getUses(MInst* I, bool funcHasReturnValue = false);
Array<MOperand> IGetUses(MInst* I, bool funcHasReturnValue = false);
Array<MOperand> FGetUses(MInst* I, bool funcHasReturnValue = false);
Array<MOperand*> getDefsPtr(MInst* I);
Array<MOperand*> IGetDefsPtr(MInst* I);
Array<MOperand*> FGetDefsPtr(MInst* I);
Array<MOperand*> getUsesPtr(MInst* I, bool funcHasReturnValue = false);
Array<MOperand*> IGetUsesPtr(MInst* I, bool funcHasReturnValue = false);
Array<MOperand*> FGetUsesPtr(MInst* I, bool funcHasReturnValue = false);

struct MOperand {
    OperandTag tag = ERRORTYPE;
    int64 value = 0;
    const char* addr = nullptr;
    MOperand() {}
    MOperand(OperandTag tag, const char* addr) : tag(tag), addr(addr) {}
    MOperand(OperandTag tag, int64 value) : tag(tag), value(value) {}
    bool operator<(const MOperand& b) const {
        if(tag != b.tag)
            return tag < b.tag;
        if(tag == IREG || tag == VIREG || tag == FREG || tag == VFREG || tag == IMM)
            return value < b.value;
        if(tag == GLO_ADR)
            return addr < b.addr;
        assert(false && "wrong tag");
        return false;
    }
    bool operator==(const MOperand& b) const {
        if(tag != b.tag)
            return false;
        if(tag == IREG || tag == VIREG || tag == FREG || tag == VFREG || tag == IMM)
            return value == b.value;
        if(tag == GLO_ADR)
            return addr == b.addr;
        assert(false && "wrong tag");
        return false;
    }
    void ISetUseHead(MIUse* use, MFunc* func);
    void FSetUseHead(MIUse* use, MFunc* func);
    void setDefI(MInst* I, MFunc* func);
    MIUse* IGetUseHead(MFunc* func);
    MIUse* FGetUseHead(MFunc* func);
    MInst* getDefI(MFunc* func);
    void IAddUse(MIUse* u, MFunc* func);
    void FAddUse(MIUse* u, MFunc* func);
    bool isImm();
    bool isInt();
    bool isIntReg();
    bool isFloat();
    bool isFloatReg();
    void replaceAllUseWith(MOperand* newOpr, MFunc* func);
};

struct MInst {
    MITag tag;
    BranchCondition cond = BR_JU;
    MInst *prev = nullptr, *next = nullptr;
    MBlock* mb = nullptr;
    bool isRv64 = false;  // 指令是否对 64 位寄存器操作
    MInst(MITag tag, BranchCondition cond = BR_JU) : tag(tag), cond(cond) {}
    virtual ~MInst() = default;
    bool isFcmpCond();
    void insertBefore(MInst* before);
    void insertAfter(MInst* after);
    void moveBefore(MInst* before);
    void moveAfter(MInst* after);
    void IReplaceUses(MOperand* oldOpr, MOperand* newOpr, MFunc* func);
    void FReplaceUses(MOperand* oldOpr, MOperand* newOpr, MFunc* func);
    void removeFromParent();
    void deleteFromParent(MFunc* func);
    void buildUse();
};

struct MI_Binary : MInst {
    BinaryOpType op;
    MOperand dst, lhs, rhs;
    MI_Binary(BinaryOpType op, MOperand dst, MOperand lhs, MOperand rhs)
        : MInst(MI_BINARY), op(op), dst(dst), lhs(lhs), rhs(rhs) {
        buildUse();
    }
};

struct MI_Branch : MInst {
    MOperand lhs, rhs;
    MBlock *TBlock, *FBlock;
    MI_Branch(BranchCondition cond, MOperand lhs, MOperand rhs, MBlock* TBlock, MBlock* FBlock = nullptr)
        : MInst(MI_BRANCH, cond), lhs(lhs), rhs(rhs), TBlock(TBlock), FBlock(FBlock) {
        buildUse();
    }
};

struct MI_Slt : MInst {
    SLTTag sltTag;
    MOperand dst, lhs, rhs;
    MI_Slt(MOperand dst, MOperand lhs, MOperand rhs, SLTTag sltTag = SLT)
        : MInst(MI_SLT), dst(dst), lhs(lhs), rhs(rhs), sltTag(sltTag) {
        buildUse();
    }
};

struct MI_Load : MInst {
    MemTag memTag = MEM_UNDEFINED;
    MOperand reg, base, offset;
    MI_Load(MOperand reg, MOperand base, MOperand offset = makeImm(0)) : MInst(MI_LOAD), reg(reg), base(base), offset(offset) {
        buildUse();
    }
};

struct MI_Store : MInst {
    MemTag memTag = MEM_UNDEFINED;
    MOperand reg, base, offset;
    MI_Store(MOperand reg, MOperand base, MOperand offset) : MInst(MI_STORE), reg(reg), base(base), offset(offset) {
        buildUse();
    }
};

struct MI_Push : MInst {
    Array<MOperand> operands;
    MI_Push(Array<MOperand> operands) : MInst(MI_PUSH), operands(operands) {
        buildUse();
    }
};

struct MI_Pop : MInst {
    Array<MOperand> operands;
    MI_Pop(Array<MOperand> operands) : MInst(MI_POP), operands(operands) {
        buildUse();
    }
};

struct MI_Clz : MInst {
    MOperand dst, operand;
    MI_Clz(MOperand dst, MOperand operand) : MInst(MI_CLZ), dst(dst), operand(operand) {
        buildUse();
    }
};

struct MI_Zext : MInst {
    MOperand dst, src;
    MI_Zext(MOperand dst, MOperand src) : MInst(MI_ZEXT), dst(dst), src(src) {
        buildUse();
    }
};

struct MI_Func_Call : MInst {
    string funcName;
    int argCount = 0;
    int argStackSize = 0;
    MI_Func_Call(string funcName) : MInst(MI_FUNC_CALL), funcName(funcName) {
        buildUse();
    }
};

struct MI_Return : MInst {
    MI_Return() : MInst(MI_RETURN) {
        buildUse();
    }
};

struct MI_Compare : MInst {
    MOperand lhs, rhs;
    MI_Compare(MOperand lhs, MOperand rhs) : MInst(MI_COMPARE), lhs(lhs), rhs(rhs) {
        buildUse();
    }
};

struct MI_Move : MInst {
    MOperand dst, src;
    MI_Move(MOperand dst, MOperand src) : MInst(MI_MOVE), dst(dst), src(src) {
        buildUse();
    }
};

struct MI_Move_Pointer_Cmp {
    bool operator()(const MI_Move* lhs, const MI_Move* rhs) const {
        if(!(lhs->dst == rhs->dst))
            return (lhs->dst) < (rhs->dst);
        return (lhs->src) < (rhs->src);
    }
};

/*------------float------------*/

struct MI_FBinary : MInst {
    BinaryOpType op;
    MOperand dst, lhs, rhs;
    int fneg = 0;
    MI_FBinary(BinaryOpType op, MOperand dst, MOperand lhs, MOperand rhs = makeImm(0))
        : MInst(MI_FBINARY), op(op), dst(dst), lhs(lhs), rhs(rhs) {
        buildUse();
    }
};

struct MI_FLoad : MInst {
    MemTag memTag = MEM_UNDEFINED;
    MOperand reg, base, offset;
    MI_FLoad(MOperand reg, MOperand base, MOperand offset) : MInst(MI_FLOAD), reg(reg), base(base), offset(offset) {
        buildUse();
    }
};

struct MI_FStore : MInst {
    MemTag memTag = MEM_UNDEFINED;
    MOperand reg, base, offset;
    MI_FStore(MOperand reg, MOperand base, MOperand offset) : MInst(MI_FSTORE), reg(reg), base(base), offset(offset) {
        buildUse();
    }
};

struct MI_FPush : MInst {
    Array<MOperand> operands;
    MI_FPush(Array<MOperand> operands) : MInst(MI_FPUSH), operands(operands) {
        buildUse();
    }
};

struct MI_FPop : MInst {
    Array<MOperand> operands;
    MI_FPop(Array<MOperand> operands) : MInst(MI_FPOP), operands(operands) {
        buildUse();
    }
};

struct MI_FCvt : MInst {
    FcvtType toType, fromType;
    MOperand dst, src;
    MI_FCvt(FcvtType toType, FcvtType fromType, MOperand dst, MOperand src)
        : MInst(MI_FCVT), toType(toType), fromType(fromType), dst(dst), src(src) {
        buildUse();
    }
};

struct MI_FCmp : MInst {
    MOperand dst, lhs, rhs;
    MI_FCmp(MOperand dst, MOperand lhs, MOperand rhs, BranchCondition cond) : MInst(MI_FCMP, cond), dst(dst), lhs(lhs), rhs(rhs) {
        buildUse();
    }
};

struct MI_FMove : MInst {
    MOperand dst, src;
    bool fromS32;
    MI_FMove(MOperand dst, MOperand src, bool fromS32) : MInst(MI_FMOVE), dst(dst), src(src), fromS32(fromS32) {
        buildUse();
    }
};

struct MI_FMove_Pointer_Cmp {
    bool operator()(const MI_FMove* lhs, const MI_FMove* rhs) const {
        if(!(lhs->dst == rhs->dst))
            return (lhs->dst) < (rhs->dst);
        return (lhs->src) < (rhs->src);
    }
};

/*------------Block------------*/
struct MBlock {
    int32 id = -1;  // block 编号
    int32 loopDepth = -1;
    MFunc* func = nullptr;
    MInst *firInst = nullptr, *lastInst = nullptr, *terminator = nullptr;
    Array<MBlock*> preds, succs;  // 最多两个后继
    MBlock(int32 id, uint32 loopDepth, MFunc* func) : id(id), loopDepth(loopDepth), func(func) {}
    void push(MInst* mi);
    void cleanEveryButTerm();
};

/*------------Func------------*/
struct MFunc {
    int index;
    int vIRegCount = 0;
    int vFRegCount = 0;
    int stackSize = 0;
    bool hasReturnValue = true;
    int IRegNeedsSave = 0;
    int FRegNeedsSave = 0;
    int IRegSpillSize = 0;
    int FRegSpillSize = 0;
    string name;
    Array<MBlock*> mBlocks;
    map<MOperand, MIUse*> IUseHeadMap;
    map<MOperand, MIUse*> FUseHeadMap;
    map<MOperand, MInst*> defIMap;
    map<MOperand, int> intValMap;
    Array<MInst*> localArrayBases;
    void clearUseDefMap();
};

/*------------Prog------------*/
struct MProg {
    Array<MFunc*> mFuncs;
};

/*------------Use------------*/
struct MIUse {
    MIUse *prev = nullptr, *next = nullptr;
    MOperand *used = nullptr, *user = nullptr;
    MInst* I = nullptr;
    MIUse() {}
    MIUse(MOperand* used, MOperand* user, MInst* I) : used(used), user(user), I(I) {}
    void rmUse(MFunc* func);
};

#endif
