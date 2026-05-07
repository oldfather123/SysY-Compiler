#pragma once

#include "AsmConstantFloat.h"
#include "AsmConstantLong.h"
#include "AsmGlobalVariable.h"
#include "AsmBasicBlock.h"
#include "AsmInstBasic.h"
#include "AsmAddressAllocator.h"
#include "AsmStackAllocator.h"
#include "AsmRegisterAllocator.h"
#include "module.h"

#include <vector>
#include <map>

namespace Backend
{
    class AsmCode;
    class AsmBasicBlock;
    class LifeTimeController;
    class AsmPassController;

    class AsmFunction
    {
    private:
        AsmCode *asmCode;
        IR::Function *irFunction;

        std::string labelName;
        AsmInstBasicList instList;
        AsmOperandRegister *returnReg;
        AsmOperandLabel *returnLabel;

        std::vector<AsmBasicBlock *> bbList;
        std::map<IR::BasicBlock *, AsmBasicBlock *> bbMap;
        std::map<AsmOperandLabel *, AsmBasicBlock *> bbLabelMap;
        std::map<AsmInstLabel *, AsmBasicBlock *> bbLabelInstMap;

        std::vector<AsmOperandBasic *> asmArgList; // contains physical param register or stackvar
        std::map<IR::Value *, AsmOperandBasic *> asmArgMap;
        std::map<IR::Value *, AsmOperandRegister *> asmArgRegMap;
        AsmInstBasicList asmArgInstList;

        AsmStackAllocator *stackAllocator;
        AsmRegisterAllocator *registerAllocator;
        AsmAddressAllocator *addressAllocator;
        LifeTimeController *lifetimeController;
        AsmPassController *passController;

    public:
        AsmFunction(AsmCode *asmCode, IR::Function *irFunction);
        void genFunction();
        std::string emit();

        AsmCode *getAsmCode();
        IR::Function *getIRFunction();
        std::string getLabelName();
        void addInst(AsmInstBasic *inst);
        void addInst(AsmInstBasicList insts);
        void addInst(int index, AsmInstBasic *inst);
        void addInst(int index, AsmInstBasicList insts);
        AsmBasicBlock *getBB(IR::BasicBlock *bb);
        AsmBasicBlock *getBB(AsmOperandLabel *label);
        AsmBasicBlock *getBB(AsmInstLabel *label);
        AsmOperandRegister *getReturnReg();
        AsmOperandLabel *getReturnLabel();
        AsmRegisterAllocator *getRegisterAllocator();
        AsmStackAllocator *getStackAllocator();
        AsmAddressAllocator *getAddressAllocator();
        AsmOperandBasic *getArg(IR::Value *value);
        AsmOperandRegister *getArgReg(IR::Value *value);
        std::vector<AsmOperandRegister *> getArgRegs();
        AsmOperandLabel *getBBLabel(IR::BasicBlock *bb);
        AsmOperandBasic *getAsmOperand(IR::Value *value);
        bool isIRConstant(IR::Value *value);
        AsmOperandBasic *getConstant(IR::Value *value, AsmInstBasicList &appendInstList);
        AsmOperandBasic *getValueByType(IR::Value *value, IR::BasicType *type);
        LifeTimeController *getLifeTimeController();
        std::vector<AsmInstBasic *> getInsts();
        void setInsts(std::vector<AsmInstBasic *> insts);

        void initReturnReg();
        void initFoamalParams();

        void prepareBBs();
        void buildArgs();
        void allocateRegisters();
        void buildStackInsts();

        ExStackVarContent *transformStackVar(AsmOperandStackVariable *src, AsmInstBasicList &instList);
        ExStackVarContent *transformStackVar(AsmOperandStackVariable *src);
        AsmOperandStackVariable *getOperableStackVariable(AsmOperandStackVariable *src, AsmInstBasicList &instList);
        AsmOperandBasic *getFormalParamAsmOperand(IR::Value *irValue, AsmInstBasicList &instList);
        AsmOperandRegisterInt *getOperandToIntRegister(AsmOperandBasic *operand);
        AsmOperandRegisterInt *getAddressToIntRegister(AsmOperandMemoryAddress *address);
        AsmOperandRegisterFloat *getOperandToFloatRegister(AsmOperandBasic *operand);
        AsmOperandMemoryAddress *getOperandToAddress(AsmOperandBasic *operand);
        AsmOperandMemoryAddress *transformStackVarToAddress(AsmOperandStackVariable *src);
        AsmOperandRegister *getValueToRegister(IR::Value *value);


        AsmInstBasicList call(AsmFunction *callee, std::vector<AsmOperandBasic *> args, AsmOperandRegister *returnReg);
    };
}
