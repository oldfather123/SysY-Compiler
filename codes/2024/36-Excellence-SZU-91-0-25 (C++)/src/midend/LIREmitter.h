#pragma once

#include "../utility/System.h"
#include "../midend/MIR.h"
#include "../backend/LIR.h"
#include "../backend/VirtualRegister.h"

// class MIRLocalVarTable
// {
// private:
//     unordered_map<int,VirtualRegister> table;

// public:
//     void Add(int mir_localvar_number,VirtualRegister vreg);
//     bool Exist(int mir_localvar_number);
//     VirtualRegister Visit(int mir_localvar_number);

//     void Clear();
// };



class LIREmitter
{
private:
    int in_function_number;
    string in_function_name;
    int in_block_number;

public:
    VRegManager vreg_manager;

    LIRFunction& InFunction();
    LIRBasicBlock& InBasicBlock();

    void AddInsturction(shared_ptr<LIRInstruction> instruction_ptr);
    VirtualRegister Load(shared_ptr<Value>& value);
    void Store(VirtualRegister result_reg,shared_ptr<ValueVariable>& value_variable);

    void EmitLIR();
    void EmitLIRFunction(MIRFunction& mir_function);
    void EmitLIRBasicBlock(MIRBasicBlock& mir_block);
    void EmitLIRInstruction(shared_ptr<MIRInstruction>& mir_instruction_ptr);
    
    //Copy
    void EmitCopy(shared_ptr<Copy>& copy);
    //Control flow
    void EmitBranch(shared_ptr<Branch>& branch);
    void EmitJump(shared_ptr<Jump>& jump);
    void EmitCall(shared_ptr<Calll>& calll);
    void EmitReturn(shared_ptr<Return>& _return);
    //Arithmetic
    void EmitOr(shared_ptr<Or>& _or);
    void EmitAnd(shared_ptr<And>& _and);
    void EmitNot(shared_ptr<Not>& _not);
    void EmitNegative(shared_ptr<Negative>& neg);
    void EmitEqual(shared_ptr<Equal>& equal);
    void EmitNotEqual(shared_ptr<NotEqual>& not_equal);
    void EmitLess(shared_ptr<Less>& less);
    void EmitLessEqual(shared_ptr<LessEqual>& less_equal);
    void EmitGreater(shared_ptr<Greater>& greater);
    void EmitGreaterEqual(shared_ptr<GreaterEqual>& greater_equal);
    void EmitAdd(shared_ptr<Add>& add);
    void EmitSub(shared_ptr<Sub>& sub);
    void EmitMul(shared_ptr<Mul>& mul);
    void EmitDiv(shared_ptr<Div>& div);
    void EmitMod(shared_ptr<Mod>& mod);
    //Convert
    void EmitItoF(shared_ptr<ItoF>& i2f);
    void EmitFtoI(shared_ptr<FtoI>& f2i);
};

extern LIREmitter lir_emitter;