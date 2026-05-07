#ifndef VI_INST_H
#define VI_INST_H

#include "backend_utils.h"

struct VI_Move : VI {
    VOper dst;
    VOper src;
    bool neg = false; // mvn

    VI_Move(VOper dst, VOper src) : VI(VI_MOVE), dst(dst), src(src) {};
};

struct VI_Move_Pointer_Cmp {
    bool operator()(const VI_Move *lhs, const VI_Move *rhs) const {
        if (!(lhs->dst == rhs->dst)) return (lhs->dst) < (rhs->dst);
        if (!(lhs->neg == rhs->neg)) return (lhs->neg) < (rhs->neg);
        return (lhs->src) < (rhs->src);
    }
};

struct VI_Binary : VI {
    Binary_Op_Type op;
    VOper dst; 
    VOper lhs, rhs;

	VI_Binary() : VI(VI_BINARY) {}
    VI_Binary(Binary_Op_Type op, VOper dst, VOper lhs, VOper rhs) : 
        VI(VI_BINARY), op(op), dst(dst), lhs(lhs), rhs(rhs) {};
};

struct VI_VBinary : VI {
    Binary_Op_Type op;
    VOper dst;
    VOper lhs, rhs;
    int fneg=0;

    VI_VBinary() : VI(VI_VBINARY) {}
    VI_VBinary(Binary_Op_Type op, VOper dst, VOper lhs, VOper rhs) : VI(VI_VBINARY), op(op), dst(dst), lhs(lhs), rhs(rhs) {};
};

struct VI_Compare : VI {
    VOper lhs, rhs;
    bool neg = false;

    VI_Compare(VOper lhs, VOper rhs) : 
        VI(VI_COMPARE), lhs(lhs), rhs(rhs) {};
};


struct VI_Slt : VI {
    VOper lhs, rhs;
    VOper dst;
    SLT_Tag slt_tag;

    VI_Slt(VOper dst, VOper lhs, VOper rhs, SLT_Tag slt_tag=SLT) : 
        VI(VI_SLT), dst(dst), lhs(lhs), rhs(rhs), slt_tag(slt_tag) {};
};

struct VI_Seqz: VI{
    VOper src;
    VOper dst;
    VI_Seqz(VOper dst, VOper src):VI(VI_SEQZ), dst(dst), src(src) {};
};

struct VI_Snez: VI{
    VOper src;
    VOper dst;
    VI_Snez(VOper dst, VOper src):VI(VI_SNEZ), dst(dst), src(src) {};
};

struct VI_Func_Call : VI {
    const char *func_name;
    int arg_count = 0;
    int arg_stack_size;

    VI_Func_Call(const char *func_name) : VI(VI_FUNC_CALL), func_name(func_name), arg_stack_size(0) {};
};

struct VI_Branch : VI {
    MachineBlock *true_target;
    MachineBlock *false_target;
    Branch_Condition cond = NO_CONDITION;
    VOper lhs, rhs;

    VI_Branch() : VI(VI_BRANCH) {};
    VI_Branch(Branch_Condition cond, MachineBlock *true_target, MachineBlock *false_target=NULL) : 
        VI(VI_BRANCH), true_target(true_target), false_target(false_target), cond(cond) {};
};

struct VI_Return : VI {
    VI_Return() : VI(VI_RETURN) {};
};

struct VI_Fcast : VI{
    int labelNum;
    string funcName;
    VI_Fcast(int labelNum, string funcName): VI(VI_FCAST), labelNum(labelNum), funcName(funcName){};
};

template<enum VI_Tag V>
struct VI_P : public VI{
    vector<VOper> operands;
    VI_P() : VI(V) {}; 
};

using VI_Push = VI_P<VI_PUSH>;
using VI_Pop = VI_P<VI_POP>;
using VI_VPush = VI_P<VI_VPUSH>;
using VI_VPop = VI_P<VI_VPOP>;

struct VI_MemOp : public VI{
    Mem_Tag mem_tag = MEM_UNDEFINED;
    VOper reg;
    VOper base;
	VOper offset;
    VI_MemOp():VI(){};
    VI_MemOp(VI_Tag tag) : VI(tag) {};
};
struct VI_Load : public VI_MemOp{
    VI_Load():VI_MemOp(VI_LOAD) {};
};
struct VI_VLoad : public VI_MemOp{
    VI_VLoad():VI_MemOp(VI_VLOAD) {};
};
struct VI_Store : public VI_MemOp{
    VI_Store():VI_MemOp(VI_STORE) {};
};
struct VI_VStore : public VI_MemOp{
    VI_VStore():VI_MemOp(VI_VSTORE) {};
};

struct VI_FMove : VI {
    VOper dst;
    VOper src;
    bool from_s32;
    bool from_f32;

    VI_FMove() : VI(VI_FMOVE){};
    VI_FMove(VOper dst, VOper src, bool from_s32, bool from_f32=false) : VI(VI_FMOVE), dst(dst), src(src), from_s32(from_s32),from_f32(from_f32){};

};

struct VI_VMove_Pointer_Cmp
{
    bool operator()(const VI_FMove *lhs, const VI_FMove *rhs) const {
        if (!(lhs->dst == rhs->dst)) return (lhs->dst) < (rhs->dst);
        return (lhs->src) < (rhs->src);
    }
};

struct VI_Fcmp_Set : VI {
    VOper lhs, rhs;
    VOper dst;
    Branch_Condition cond = NO_CONDITION;

    VI_Fcmp_Set(Branch_Condition cond, VOper dst, VOper lhs, VOper rhs) : VI(VI_FCMP_SET), dst(dst), lhs(lhs), rhs(rhs), cond(cond) {}
};

struct VI_VCvt : VI {
    Vcvt_Type from_type;
    Vcvt_Type to_type;
    VOper dst;
    VOper src;
    VI_VCvt() : VI(VI_VCVT) {};
    VI_VCvt(Vcvt_Type from, Vcvt_Type to) : VI(VI_VCVT), from_type(from), to_type(to) {};
};

#endif