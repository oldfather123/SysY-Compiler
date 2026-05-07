//
// Created by divin on 23-7-13.
//

#include "IRInstruction.h"
#include "Type.h"
#include "ValueRef.h"
#include "codegen/AsmBuilder.hpp"
#include "codegen/MachineInstruction.hpp"
#include "codegen/MachineOperand.hpp"
#include "math.h"

using namespace std;

int IRInstruction::vRegCount = 0;
int IRInstruction::vfRegCount = 0;
int IRInstruction::labelCount = 0;
std::map<std::string, double> IRInstruction::float_table = {};

IRInstruction::IRInstruction(InstType_Enum instType): Instruction(instType) {
    setVRegisterCount(0);
}

DummyInstruction::DummyInstruction() : IRInstruction(DUMMY){}

AddGlobalInstruction::AddGlobalInstruction(ValueRef *dst, Type *varType, ValueRef *initVal, string& debugInfo) : IRInstruction(AddGlb){
    this->dst = dst;
    this->varType = varType;
    this->initVal = initVal;
    this->debugInfo = debugInfo;

    //TODO:use-def集
}

void AddGlobalInstruction::codegen(AsmBuilder *pBuilder) const {
    auto &ss = pBuilder->getUnit()->global_vars;
//    cout << this->varType->type << endl;
//TODO:other types
//    static_cast<Symbol>(this->dst->get_Ref())
    string name = ((Symbol*)this->dst)->name;
//    pBuilder->getUnit()->global_symbol.insert(name);

    if(Type_Enum(this->varType->type) == INT32TYPE){
        if(this->initVal!= nullptr) {
            ss << ".data" << '\n';
            ss << this->dst->get_Ref().substr(1) << ":\n";
            ss << "\t.word\t" << this->initVal->get_Ref() << '\n';
        } else {
            ss << ".bss" << '\n';
            ss << this->dst->get_Ref().substr(1) << ":\n";
            ss << "\t.skip\t4" << '\n';
        }
    }
    else if(Type_Enum(this->varType->type) == ARRAYTYPE){
        ArrayType * arrayType = (ArrayType *)(this->varType);
        while(arrayType->elementType->type != FLOATTYPE && arrayType->elementType->type != INT32TYPE){
            arrayType = (ArrayType *)arrayType->elementType;
        }
//        cout << arrayType->elementType->type << endl;
        if(this->initVal == nullptr){
            ss << ".data" << '\n';
            ss << this->dst->get_Ref().substr(1) << ":\n";
            ss << "\t.zero\t" << this->varType->size << '\n';
        }else{
            ss << ".data" << '\n';
            ss << this->dst->get_Ref().substr(1) << ":\n";

            auto* init = dynamic_cast<Array_Const *>(this->initVal);

            if(Type_Enum(arrayType->elementType->type) == INT32TYPE){
                arrInitInt(init->values, dynamic_cast<ArrayType *>(this->varType), ss);
            }
            else{
                arrInitFloat(init->values, dynamic_cast<ArrayType *>(this->varType), ss);
            }
        }
    }else if(Type_Enum(this->varType->type) == FLOATTYPE){
        if(this->initVal!= nullptr) {
            ss << ".data" << '\n';
            ss << this->dst->get_Ref().substr(1) << ":\n";
            //TODO: calculate maybe wrong!
            float f = ((Float_Const*)(this->initVal))->value;

            std::bitset<32> bits(*reinterpret_cast<unsigned int*>(&f));
            unsigned long decimal_value = static_cast<int>(bits.to_ulong());
            if (f < 0) {
                decimal_value = ~(decimal_value - 1);
                ss << "\t.word\t" << "-" << decimal_value << '\n';
            } else{
                ss << "\t.word\t" << decimal_value << '\n';
            }
        } else {
            ss << ".bss" << '\n';
            ss << this->dst->get_Ref().substr(1) << ":\n";
            ss << "\t.skip\t4" << '\n';
        }
    }
}

void AddGlobalInstruction::replace(ValueRef *old, ValueRef *now) {
    cerr << "global var can't be replaced" << endl;
    throw exception();
}

AllocaInstruction::AllocaInstruction(ValueRef* dst, Type* varType, string& debugInfo) : IRInstruction(ALLOCA){
    this->dst = dst;
    this->varType = varType;
    this->debugInfo = debugInfo;

    addDef(dst);
}

void AllocaInstruction::replace(ValueRef *old, ValueRef *now) {
    cerr << "allocated var can't be replaced" << endl;
    throw exception();
}

LoadInstruction::LoadInstruction(ValueRef *dst, ValueRef *src) : IRInstruction(LOAD){
    this->dst = dst;
    this->src = src;

    addDef(dst);
    addUse(src);
}

void LoadInstruction::codegen(AsmBuilder *pBuilder, std::map<std::string, int> &offset_table,
                              std::map<std::string, int> &size_table, int frameSize) {
//    cout << this->src->type << endl;
    if(this->src->get_Ref()[0] == '@'){
        // if (pBuilder->mUnit->global_symbol.find(this->src->get_Ref()) != pBuilder->mUnit->global_symbol.end()) {
        auto la = new LoadImmInstruction(pBuilder->getBlock(),
                                         LoadImmInstruction::LA,
                                         new MachineOperand(MachineOperand::VREG, getVRegister()),
                                         new MachineOperand(MachineOperand::LABEL, this->src->get_Ref().substr(1)));
        pBuilder->getBlock()->insertInst(la);
//        cout << ((Symbol*)this->src)->symbolType->type << endl;
        if(((Symbol*)this->src)->symbolType->type == 1){
//            i32
            auto lw = new LoadMInstruction(pBuilder->getBlock(),
                                           LoadMInstruction::LW,
                                           new MachineOperand(MachineOperand::VREG, this->dst->get_Ref()),
                                           new MachineOperand(MachineOperand::IMM, 0),
                                           new MachineOperand(MachineOperand::VREG, getLastVRegister()));
            pBuilder->getBlock()->insertInst(lw);
        }else if(((Symbol*)this->src)->symbolType->type == 2){
//            float
            LoadFloat(pBuilder,this->dst->get_Ref(), getLastVRegister());
        }
    } else{
        if(RefType(this->src->type) == SYMBOL){
//        load var from stack
            auto offsetIter = offset_table.find(this->src->get_Ref());
            if(Type_Enum(((Symbol*)this->src)->symbolType->type)==POINTERTYPE){
                loadFromFP(pBuilder, LoadMInstruction::LD, this->dst->get_Ref(), -16-8-offsetIter->second);
            }
            else if(Type_Enum(((Symbol*)this->src)->symbolType->type)==INT32TYPE){
                loadFromFP(pBuilder, LoadMInstruction::LW, this->dst->get_Ref(), -16-4-offsetIter->second);
            }else if(Type_Enum(((Symbol*)this->src)->symbolType->type)==FLOATTYPE){
                // auto flw = new LoadMInstruction(pBuilder->getBlock(),
                //                                 LoadMInstruction::FLW,
                //                                 new MachineOperand(MachineOperand::FVREG, this->dst->get_Ref()),
                //                                 new MachineOperand(MachineOperand::IMM, -16-4-offsetIter->second),
                //                                 new MachineOperand(MachineOperand::REG, FP));
                // pBuilder->getBlock()->insertInst(flw);
                loadFloatFromFP(pBuilder, LoadMInstruction::FLW, this->dst->get_Ref(), -16 - 4 - offsetIter->second);
            }
        }else if(RefType(this->src->type) == Ptr){
//            cout << "this->src->type" << endl;
            // auto lw1 = new LoadMInstruction(pBuilder->getBlock(),
            //                                 LoadMInstruction::LW,
            //                                 new MachineOperand(MachineOperand::VREG, this->dst->get_Ref()),
            //                                 new MachineOperand(MachineOperand::IMM, 0),
            //                                 new MachineOperand(MachineOperand::VREG, this->src->get_Ref()));
            // pBuilder->getBlock()->insertInst(lw1);
            if (this->dst->type == FloatVar) {
                auto flw = new LoadMInstruction(pBuilder->getBlock(),
                                                LoadMInstruction::FLW,
                                                new MachineOperand(MachineOperand::FVREG, this->dst->get_Ref()),
                                                new MachineOperand(MachineOperand::IMM, 0),
                                                new MachineOperand(MachineOperand::VREG, this->src->get_Ref()));
                pBuilder->getBlock()->insertInst(flw);

            } else {
                auto lw1 = new LoadMInstruction(pBuilder->getBlock(),
                                                LoadMInstruction::LW,
                                                new MachineOperand(MachineOperand::VREG, this->dst->get_Ref()),
                                                new MachineOperand(MachineOperand::IMM, 0),
                                                new MachineOperand(MachineOperand::VREG, this->src->get_Ref()));
                pBuilder->getBlock()->insertInst(lw1);
            }
        }
    }
}

void LoadInstruction::outPut(std::ostream &os) {
    os << dst->get_Ref() + " = load " + dst->get_TypeName() + ", " + dst->get_TypeName() + '*' + ' ' + src->name + ", align 4" << endl;
}

void LoadInstruction::replace(ValueRef *old, ValueRef *now) {
    if(this->src == old) this->src = now;
    if(this->use_list.count(old)) this->use_list.erase(old);
    this->use_list.insert(now);

    now->use.push_back(this);
}

StoreInstruction::StoreInstruction(ValueRef* dst,ValueRef* src) : IRInstruction(STORE){
    this->dst = dst;
    this->src = src;

    addUse(src);
    if(dst->type == SYMBOL){
        addDef(dst); // the symbol is assigned(def), so we can use it in mem2reg
    }
    else{
        addUse(dst); // value is stored in dst, we addUse to avoid be deleted in DCE
    }
}

void StoreInstruction::codegen(AsmBuilder *pBuilder, std::map<std::string, int> &offset_table,
                               std::map<std::string, int> &size_table, int frameSize) {
//    cout << this->dst->get_Ref() << endl;
    if(this->dst->get_Ref()[0] == '@'){
        //            high
        auto liAddress = new LoadImmInstruction(pBuilder->getBlock(),
                                                LoadImmInstruction::LUI,
                                                new MachineOperand(MachineOperand::VREG, getVRegister()),
                                                new MachineOperand(MachineOperand::LABEL, this->dst->get_Ref().substr(1)));
        pBuilder->getBlock()->insertInst(liAddress);
//              low
        auto addAddress = new BinaryMInstruction(pBuilder->getBlock(),
                                                 BinaryMInstruction::ADDI,
                                                 new MachineOperand(MachineOperand::VREG, getLastVRegister()),
                                                 new MachineOperand(MachineOperand::VREG, getLastVRegister()),
                                                 new MachineOperand(MachineOperand::LABEL, "%lo("+this->dst->get_Ref().substr(1)+")"));
        pBuilder->getBlock()->insertInst(addAddress);
//        cout << "storeGlobal" << endl;
        if(RefType(this->src->type) == IntConst){
            loadImmIntoReg(pBuilder,strtol(this->src->get_Ref().c_str(), nullptr, 10));
//            store
            auto store = new StoreMInstruction(pBuilder->getBlock(),
                                               StoreMInstruction::SW,
                                               new MachineOperand(MachineOperand::VREG, getLastVRegister()),
                                               new MachineOperand(MachineOperand::IMM, 0),
                                               new MachineOperand(MachineOperand::VREG, getLastVRegister(2)));
            pBuilder->getBlock()->insertInst(store);
        }else if(RefType(this->src->type) == IntVar){
            auto store = new StoreMInstruction(pBuilder->getBlock(),
                                               StoreMInstruction::SW,
                                               new MachineOperand(MachineOperand::VREG, this->src->get_Ref()),
                                               new MachineOperand(MachineOperand::IMM, 0),
                                               new MachineOperand(MachineOperand::VREG, getLastVRegister()));
            pBuilder->getBlock()->insertInst(store);
        }else if(RefType(this->src->type) == FloatConst){
            isFloatConst(this->src);
            LoadLabel(pBuilder, getVRegister(), getLC());
            LoadFloat(pBuilder,getFVRegister(), getLastVRegister());
            auto fstore = new StoreMInstruction(pBuilder->getBlock(),
                                                StoreMInstruction::FSW,
                                                new MachineOperand(MachineOperand::FVREG, getLastFVRegister()),
                                                new MachineOperand(MachineOperand::IMM, 0),
                                                new MachineOperand(MachineOperand::VREG, getLastVRegister(2)));
            pBuilder->getBlock()->insertInst(fstore);
        }else if(RefType(this->src->type) == FloatVar){
            auto fstore = new StoreMInstruction(pBuilder->getBlock(),
                                                StoreMInstruction::FSW,
                                                new MachineOperand(MachineOperand::FVREG, this->src->get_Ref()),
                                                new MachineOperand(MachineOperand::IMM, 0),
                                                new MachineOperand(MachineOperand::VREG, getLastVRegister()));
            pBuilder->getBlock()->insertInst(fstore);
        }
    } else{
        if(RefType(this->src->type) == IntConst){
//        cout << "intConst" << endl;
            loadImmIntoReg(pBuilder, strtol(this->src->get_Ref().c_str(), nullptr, 10));
            if(this->dst->type == Ptr){
                auto sw = new StoreMInstruction(pBuilder->getBlock(),
                                                StoreMInstruction::SW,
                                                new MachineOperand(MachineOperand::VREG, getLastVRegister()),
                                                new MachineOperand(MachineOperand::IMM, 0),
                                                new MachineOperand(MachineOperand::VREG, this->dst->get_Ref()));
                pBuilder->getBlock()->insertInst(sw);
            }
            else{
                auto offsetIter = offset_table.find(this->dst->get_Ref());
                storeToStack(pBuilder, StoreMInstruction::SW, getLastVRegister(), - 20 - offsetIter->second);
            }
        }else if(RefType(this->src->type) == IntVar){
//        cout << "intVar" << endl;
            if(this->dst->type == Ptr){
                auto sw = new StoreMInstruction(pBuilder->getBlock(),
                                                StoreMInstruction::SW,
                                                new MachineOperand(MachineOperand::VREG, this->src->get_Ref()),
                                                new MachineOperand(MachineOperand::IMM, 0),
                                                new MachineOperand(MachineOperand::VREG, this->dst->get_Ref()));
                pBuilder->getBlock()->insertInst(sw);
            }
            else{
                auto offsetIter = offset_table.find(this->dst->get_Ref());
                storeToStack(pBuilder, StoreMInstruction::SW, this->src->get_Ref(), - 20 - offsetIter->second);
            }
        }else if(RefType(this->src->type) == Ptr){
//            cout << "ptr" << endl; in args
            auto offsetIter = offset_table.find(this->dst->get_Ref());
            storeToStack(pBuilder, StoreMInstruction::SD, this->src->get_Ref(), - 16 - 8 - offsetIter->second);
        }else if(RefType(this->src->type) == FloatConst){
            isFloatConst(this->src);
            LoadLabel(pBuilder, getVRegister(), getLC());
            LoadFloat(pBuilder, getFVRegister(), getLastVRegister());
            if(this->dst->type == Ptr){
                auto fsw = new StoreMInstruction(pBuilder->getBlock(),
                                                 StoreMInstruction::FSW,
                                                 new MachineOperand(MachineOperand::FVREG, getLastFVRegister()),
                                                 new MachineOperand(MachineOperand::IMM, 0),
                                                 new MachineOperand(MachineOperand::VREG,this->dst->get_Ref()));
                pBuilder->getBlock()->insertInst(fsw);
            }
            else{
                auto offsetIter = offset_table.find(this->dst->get_Ref());
                // auto fsw = new StoreMInstruction(pBuilder->getBlock(),
                //                                  StoreMInstruction::FSW,
                //                                  new MachineOperand(MachineOperand::FVREG, getLastFVRegister()),
                //                                  new MachineOperand(MachineOperand::IMM, -20-offsetIter->second),
                //                                  new MachineOperand(MachineOperand::REG,FP));
                // pBuilder->getBlock()->insertInst(fsw);
                storeFloatToStack(pBuilder, StoreMInstruction::FSW, getLastFVRegister(), -20 - offsetIter->second);
            }
        }else if(RefType(this->src->type) == FloatVar){
            if(this->dst->type == Ptr){
                auto fsw = new StoreMInstruction(pBuilder->getBlock(),
                                                 StoreMInstruction::FSW,
                                                 new MachineOperand(MachineOperand::FVREG, this->src->get_Ref()),
                                                 new MachineOperand(MachineOperand::IMM, 0),
                                                 new MachineOperand(MachineOperand::VREG,this->dst->get_Ref()));
                pBuilder->getBlock()->insertInst(fsw);
            }
            else{
                auto offsetIter = offset_table.find(this->dst->get_Ref());
                // auto fsw = new StoreMInstruction(pBuilder->getBlock(),
                //                                  StoreMInstruction::FSW,
                //                                  new MachineOperand(MachineOperand::FVREG, this->src->get_Ref()),
                //                                  new MachineOperand(MachineOperand::IMM, -20-offsetIter->second),
                //                                  new MachineOperand(MachineOperand::REG,FP));
                // pBuilder->getBlock()->insertInst(fsw);
                storeFloatToStack(pBuilder, StoreMInstruction::FSW, this->src->get_Ref(), -20 - offsetIter->second);
            }
        }
    }
}

void StoreInstruction::outPut(std::ostream &os) {
    if(dst->type == SYMBOL){
        Type* eleType = dynamic_cast<Symbol*>(dst)->symbolType;
        os << "store " + eleType->getTypeName() + ' ' + src->get_Ref() + ", " + eleType->getTypeName() + "* " + dst->name + ", align 4" << endl;
    }
    else{ //dst->type == Ptr
        PointerType *p = dynamic_cast<Pointer*>(dst)->pointerType;
        os << "store " + p->elementType->getTypeName() + ' ' + src->get_Ref() + ", " + p->getTypeName() + ' ' + dst->name + ", align 4" << endl;
    }
}

void StoreInstruction::replace(ValueRef *old, ValueRef *now) {
    if(this->src == old) this->src = now;
    if(this->dst == old) this->dst = now;
    if(this->use_list.count(old)) this->use_list.erase(old);
    this->use_list.insert(now);

    now->use.push_back(this);
}

BinaryInstruction::BinaryInstruction(ValueRef* dst,ValueRef* src1, ValueRef* src2, binaryType opTy) : IRInstruction(BINARY){
    this->dst = dst;
    this->src1 = src1;
    this->src2 = src2;
    this->opTy = opTy;

    addDef(dst);
    addUse(src1);
    addUse(src2);
}


void BinaryInstruction::outPut(std::ostream &os) {
    string tb[] = {" add i32 "," sub i32 "," mul i32 "," sdiv i32 "," srem i32 "," and i32 "," fadd float ", " fsub float ", " fmul float ", " fdiv float " ," not exist "," not exist "};
    int idx = opTy;
    if(this->src1->type == FloatVar || this->src1->type == FloatConst) idx += 6;
    os << dst->get_Ref() + " =" + tb[idx] + src1->get_Ref() + ", " + src2->get_Ref() << endl;
}

void BinaryInstruction::codegen(AsmBuilder *pBuilder, std::map<std::string, int> &offset_table,
                                std::map<std::string, int> &size_table, int frameSize) {
    if(this->opTy == ADD){
//        cout << "add" << endl;
        if(RefType(this->src1->type) == IntVar && RefType(this->src2->type) == IntVar){
            BinaryInstr(pBuilder, BinaryMInstruction::ADDW, this->dst->get_Ref(), this->src1->get_Ref(), this->src2->get_Ref());
        }else if(RefType(this->src1->type) == IntVar && RefType(this->src2->type) == IntConst){
            Addiw(pBuilder, BinaryMInstruction::ADDI, this->dst->get_Ref(), this->src1->get_Ref(), strtol(this->src2->get_Ref().c_str(), nullptr, 10));
        }else if(RefType(this->src1->type) == IntConst && RefType(this->src2->type) == IntVar){
            Addiw(pBuilder, BinaryMInstruction::ADDI, this->dst->get_Ref(), this->src2->get_Ref(), strtol(this->src1->get_Ref().c_str(), nullptr, 10));
        }else if(RefType(this->src1->type) == FloatVar && RefType(this->src2->type) == FloatVar){
            fBinaryInstr(pBuilder,BinaryMInstruction::FADD,this->dst->get_Ref(),this->src1->get_Ref(),this->src2->get_Ref());
        }else if(RefType(this->src1->type) == FloatVar && RefType(this->src2->type) == FloatConst){
            isFloatConst(this->src2);
            LoadLabel(pBuilder, getVRegister(), getLC());
            LoadFloat(pBuilder, getFVRegister(), getLastVRegister());
            fBinaryInstr(pBuilder,BinaryMInstruction::FADD,this->dst->get_Ref(),this->src1->get_Ref(),getLastFVRegister());
        }else if(RefType(this->src1->type) == FloatConst && RefType(this->src2->type) == FloatVar){
            isFloatConst(this->src1);
            LoadLabel(pBuilder, getVRegister(), getLC());
            LoadFloat(pBuilder, getFVRegister(), getLastVRegister());
            fBinaryInstr(pBuilder,BinaryMInstruction::FADD,this->dst->get_Ref(),this->src2->get_Ref(),getLastFVRegister());
        }
    }
    else if(this->opTy == SUB){
//        cout << "sub" << endl;
        if(RefType(this->src1->type) == IntVar && RefType(this->src2->type) == IntVar){
            BinaryInstr(pBuilder,BinaryMInstruction::SUBW,this->dst->get_Ref(), this->src1->get_Ref(),this->src2->get_Ref());
        }else if(RefType(this->src1->type) == IntVar && RefType(this->src2->type) == IntConst){
            Addiw(pBuilder, BinaryMInstruction::ADDI, this->dst->get_Ref(), this->src1->get_Ref(), -strtol(this->src2->get_Ref().c_str(), nullptr, 10));
        }else if(RefType(this->src1->type) == IntConst && RefType(this->src2->type) == IntVar){
            loadImmIntoReg(pBuilder, strtol(this->src1->get_Ref().c_str(), nullptr, 10));
            BinaryInstr(pBuilder,BinaryMInstruction::SUBW,this->dst->get_Ref(), getLastVRegister(),this->src2->get_Ref());
        }else if(RefType(this->src1->type) == FloatVar && RefType(this->src2->type) == FloatVar){
            fBinaryInstr(pBuilder,BinaryMInstruction::FSUB,this->dst->get_Ref(),this->src1->get_Ref(),this->src2->get_Ref());
        }else if(RefType(this->src1->type) == FloatVar && RefType(this->src2->type) == FloatConst){
            isFloatConst(this->src2);
            LoadLabel(pBuilder, getVRegister(), getLC());
            LoadFloat(pBuilder, getFVRegister(), getLastVRegister());
            fBinaryInstr(pBuilder,BinaryMInstruction::FSUB,this->dst->get_Ref(),this->src1->get_Ref(),getLastFVRegister());
        }else if(RefType(this->src1->type) == FloatConst && RefType(this->src2->type) == FloatVar){
            isFloatConst(this->src1);
            LoadLabel(pBuilder, getVRegister(), getLC());
            LoadFloat(pBuilder, getFVRegister(), getLastVRegister());
            fBinaryInstr(pBuilder,BinaryMInstruction::FSUB,this->dst->get_Ref(),getLastFVRegister(),this->src2->get_Ref());
        }
    }
    else if(this->opTy == MOD){
//        cout << "mod" << endl;
        if(RefType(this->src1->type) == IntVar && RefType(this->src2->type) == IntVar){
            BinaryInstr(pBuilder, BinaryMInstruction::REMW, this->dst->get_Ref(), this->src1->get_Ref(), this->src2->get_Ref());
        }else if(RefType(this->src1->type) == IntVar && RefType(this->src2->type) == IntConst){
            loadImmIntoReg(pBuilder,strtol(this->src2->get_Ref().c_str(), nullptr, 10));
            BinaryInstr(pBuilder, BinaryMInstruction::REMW, this->dst->get_Ref(), this->src1->get_Ref(), getLastVRegister());
        }else if(RefType(this->src1->type) == IntConst && RefType(this->src2->type) == IntVar){
            loadImmIntoReg(pBuilder,strtol(this->src1->get_Ref().c_str(), nullptr, 10));
            BinaryInstr(pBuilder, BinaryMInstruction::REMW, this->dst->get_Ref(), getLastVRegister(),this->src2->get_Ref());
        }
    }
    else if(this->opTy == DIV) {
        if(RefType(this->src1->type) == IntVar && RefType(this->src2->type) == IntVar){
            BinaryInstr(pBuilder, BinaryMInstruction::DIVW, this->dst->get_Ref(), this->src1->get_Ref(), this->src2->get_Ref());
        }else if(RefType(this->src1->type) == IntVar && RefType(this->src2->type) == IntConst){
            loadImmIntoReg(pBuilder, strtol(this->src2->get_Ref().c_str(), nullptr, 10));
            BinaryInstr(pBuilder, BinaryMInstruction::DIVW, this->dst->get_Ref(), this->src1->get_Ref(), getLastVRegister());
        }else if(RefType(this->src1->type) == IntConst && RefType(this->src2->type) == IntVar){
            loadImmIntoReg(pBuilder, strtol(this->src1->get_Ref().c_str(), nullptr, 10));
            BinaryInstr(pBuilder, BinaryMInstruction::DIVW, this->dst->get_Ref(), getLastVRegister(),this->src2->get_Ref());
        }else if(RefType(this->src1->type) == FloatVar && RefType(this->src2->type) == FloatVar){
            fBinaryInstr(pBuilder,BinaryMInstruction::FDIV,this->dst->get_Ref(),this->src1->get_Ref(),this->src2->get_Ref());
        }else if(RefType(this->src1->type) == FloatVar && RefType(this->src2->type) == FloatConst){
            isFloatConst(this->src2);
            LoadLabel(pBuilder, getVRegister(), getLC());
            LoadFloat(pBuilder, getFVRegister(), getLastVRegister());
            fBinaryInstr(pBuilder,BinaryMInstruction::FDIV,this->dst->get_Ref(),this->src1->get_Ref(),getLastFVRegister());
        }else if(RefType(this->src1->type) == FloatConst && RefType(this->src2->type) == FloatVar){
            isFloatConst(this->src1);
            LoadLabel(pBuilder, getVRegister(), getLC());
            LoadFloat(pBuilder, getFVRegister(), getLastVRegister());
            fBinaryInstr(pBuilder,BinaryMInstruction::FDIV,this->dst->get_Ref(),getLastFVRegister(),this->src2->get_Ref());
        }
    }
    else if(this->opTy == MUL) {
        if(RefType(this->src1->type) == IntVar && RefType(this->src2->type) == IntVar){
            BinaryInstr(pBuilder, BinaryMInstruction::MULW, this->dst->get_Ref(), this->src1->get_Ref(), this->src2->get_Ref());
        }else if(RefType(this->src1->type) == IntVar && RefType(this->src2->type) == IntConst){
            loadImmIntoReg(pBuilder, strtol(this->src2->get_Ref().c_str(), nullptr, 10));
            BinaryInstr(pBuilder, BinaryMInstruction::MULW, this->dst->get_Ref(), this->src1->get_Ref(), getLastVRegister());
        }else if(RefType(this->src1->type) == IntConst && RefType(this->src2->type) == IntVar){
            loadImmIntoReg(pBuilder, strtol(this->src1->get_Ref().c_str(), nullptr, 10));
            BinaryInstr(pBuilder, BinaryMInstruction::MULW, this->dst->get_Ref(), getLastVRegister(),this->src2->get_Ref());
        }else if(RefType(this->src1->type) == FloatVar && RefType(this->src2->type) == FloatVar){
            fBinaryInstr(pBuilder,BinaryMInstruction::FMUL,this->dst->get_Ref(),this->src1->get_Ref(),this->src2->get_Ref());
        }else if(RefType(this->src1->type) == FloatVar && RefType(this->src2->type) == FloatConst){
            isFloatConst(this->src2);
            LoadLabel(pBuilder, getVRegister(), getLC());
            LoadFloat(pBuilder, getFVRegister(), getLastVRegister());
            fBinaryInstr(pBuilder,BinaryMInstruction::FMUL,this->dst->get_Ref(),this->src1->get_Ref(),getLastFVRegister());
        }else if(RefType(this->src1->type) == FloatConst && RefType(this->src2->type) == FloatVar){
            isFloatConst(this->src1);
            LoadLabel(pBuilder, getVRegister(), getLC());
            LoadFloat(pBuilder, getFVRegister(), getLastVRegister());
            fBinaryInstr(pBuilder,BinaryMInstruction::FMUL,this->dst->get_Ref(),getLastFVRegister(),this->src2->get_Ref());
        }
    }
    else if(this->opTy == AND){
        if(RefType(this->src1->type) == IntVar && RefType(this->src2->type) == IntVar){
            BinaryInstr(pBuilder, BinaryMInstruction::AND, this->dst->get_Ref(), this->src1->get_Ref(), this->src2->get_Ref());
        }else if(RefType(this->src1->type) == IntVar && RefType(this->src2->type) == IntConst){
            int num = strtol(this->src2->get_Ref().c_str(), nullptr, 10);
            auto bin = new BinaryMInstruction(pBuilder->getBlock(),
                                              BinaryMInstruction::ANDI,
                                              new MachineOperand(MachineOperand::VREG, this->dst->get_Ref()),
                                              new MachineOperand(MachineOperand::VREG, this->src1->get_Ref()),
                                              new MachineOperand(MachineOperand::IMM, num));
            pBuilder->getBlock()->insertInst(bin);
        }else if(RefType(this->src1->type) == IntConst && RefType(this->src2->type) == IntVar){
            int num = strtol(this->src1->get_Ref().c_str(), nullptr, 10);
            auto bin = new BinaryMInstruction(pBuilder->getBlock(),
                                              BinaryMInstruction::ANDI,
                                              new MachineOperand(MachineOperand::VREG, this->dst->get_Ref()),
                                              new MachineOperand(MachineOperand::VREG, this->src2->get_Ref()),
                                              new MachineOperand(MachineOperand::IMM, num));
            pBuilder->getBlock()->insertInst(bin);
        }
    }
}

void BinaryInstruction::replace(ValueRef *old, ValueRef *now) {
    if(this->src1 == old) this->src1 = now;
    if(this->src2 == old) this->src2 = now;
    if(this->use_list.count(old)) this->use_list.erase(old);
    this->use_list.insert(now);

    now->use.push_back(this);
}

CmpInstruction::CmpInstruction(ValueRef* result, ValueRef* src1, ValueRef* src2, cmpType opTy) : IRInstruction(CMP){
    this->result = result;
    this->src1 = src1;
    this->src2 = src2;
    this->opTy = opTy;

    addDef(result);
    addUse(src1);
    addUse(src2);
}

void CmpInstruction::outPut(std::ostream &os) {
    //EQ, NE, LT, LE, GT, GE
    string tb[] = {"icmp eq i32 ","icmp ne i32 ","icmp slt i32 ","icmp sle i32 ","icmp sgt i32 ","icmp sge i32 ",
                   "fcmp oeq float ", "fcmp une float ", "fcmp olt float ", "fcmp ole float " ,"fcmp ogt float ", "fcmp oge float "};
    int idx = opTy;
    if(this->src1->type == FloatVar || this->src1->type == FloatConst) idx += 6;
    os << result->get_Ref() + " = " + tb[idx] + src1->get_Ref() + ", " + src2->get_Ref() << endl;
}

void
CmpInstruction::codegen(AsmBuilder *pBuilder, map<std::string, int> &offset_table, map<std::string, int> &size_table,
                        int frameSize) {
//    var and var
    if(cmpType(this->opTy) == NE || cmpType(this->opTy) == EQ){
        if(RefType(this->src1->type) == IntVar && RefType(this->src2->type) == IntVar){
            auto xor1 = new BinaryMInstruction(pBuilder->getBlock(),
                                               BinaryMInstruction::XOR,
                                               new MachineOperand(MachineOperand::VREG, getVRegister()),
                                               new MachineOperand(MachineOperand::VREG, this->src1->get_Ref()),
                                               new MachineOperand(MachineOperand::VREG, this->src2->get_Ref()));
            pBuilder->getBlock()->insertInst(xor1);
            auto sltu = new CmpMInstruction(pBuilder->getBlock(),
                                            CmpMInstruction::SLTU,
                                            new MachineOperand(MachineOperand::VREG, this->result->get_Ref()),
                                            new MachineOperand(MachineOperand::REG, ZERO),
                                            new MachineOperand(MachineOperand::VREG, getLastVRegister()));
            pBuilder->getBlock()->insertInst(sltu);
        }else if(RefType(this->src1->type) == IntConst && RefType(this->src2->type) == IntVar){
            auto xor1 = new BinaryMInstruction(pBuilder->getBlock(),
                                               BinaryMInstruction::XORI,
                                               new MachineOperand(MachineOperand::VREG, getVRegister()),
                                               new MachineOperand(MachineOperand::VREG, this->src2->get_Ref()),
                                               new MachineOperand(MachineOperand::IMM, strtol(this->src1->get_Ref().c_str(), nullptr,10)));
            pBuilder->getBlock()->insertInst(xor1);
            auto sltu = new CmpMInstruction(pBuilder->getBlock(),
                                            CmpMInstruction::SLTU,
                                            new MachineOperand(MachineOperand::VREG, this->result->get_Ref()),
                                            new MachineOperand(MachineOperand::REG, ZERO),
                                            new MachineOperand(MachineOperand::VREG, getLastVRegister()));
            pBuilder->getBlock()->insertInst(sltu);
        }else if(RefType(this->src1->type) == IntVar && RefType(this->src2->type) == IntConst){
            auto xor1 = new BinaryMInstruction(pBuilder->getBlock(),
                                               BinaryMInstruction::XORI,
                                               new MachineOperand(MachineOperand::VREG, getVRegister()),
                                               new MachineOperand(MachineOperand::VREG, this->src1->get_Ref()),
                                               new MachineOperand(MachineOperand::IMM, strtol(this->src2->get_Ref().c_str(), nullptr,10)));
            pBuilder->getBlock()->insertInst(xor1);
            auto sltu = new CmpMInstruction(pBuilder->getBlock(),
                                            CmpMInstruction::SLTU,
                                            new MachineOperand(MachineOperand::VREG, this->result->get_Ref()),
                                            new MachineOperand(MachineOperand::REG, ZERO),
                                            new MachineOperand(MachineOperand::VREG, getLastVRegister()));
            pBuilder->getBlock()->insertInst(sltu);
        }else if(RefType(this->src1->type) == FloatVar && RefType(this->src2->type) == FloatVar){
            FCmp(pBuilder, CmpMInstruction::FEQ, this->result->get_Ref(), this->src1->get_Ref(), this->src2->get_Ref());
        }else if(RefType(this->src1->type) == FloatVar && RefType(this->src2->type) == FloatConst){
            isFloatConst(this->src2);
            LoadLabel(pBuilder, getVRegister(), getLC());
            LoadFloat(pBuilder, getFVRegister(), getLastVRegister());
            FCmp(pBuilder, CmpMInstruction::FEQ, this->result->get_Ref(), this->src1->get_Ref(), getLastFVRegister());
        }else if(RefType(this->src1->type) == FloatConst && RefType(this->src2->type) == FloatVar){
            isFloatConst(this->src1);
            LoadLabel(pBuilder, getVRegister(), getLC());
            LoadFloat(pBuilder, getFVRegister(), getLastVRegister());
            FCmp(pBuilder, CmpMInstruction::FEQ, this->result->get_Ref(), getLastFVRegister(), this->src2->get_Ref());
        }
        if(cmpType(this->opTy) == EQ && (RefType(this->src1->type) == IntVar || RefType(this->src1->type) == IntConst)){
            xoriReg(pBuilder,new MachineOperand(MachineOperand::VREG, this->result->get_Ref()));
        }else if(cmpType(this->opTy) == NE && (RefType(this->src1->type) == FloatVar || RefType(this->src1->type) == FloatConst)){
            xoriReg(pBuilder,new MachineOperand(MachineOperand::VREG, this->result->get_Ref()));
        }
    }
    else if(cmpType(this->opTy) == LT || cmpType(this->opTy) == GE){
        if(RefType(this->src1->type) == IntVar && RefType(this->src2->type) == IntVar){
            auto slt = new CmpMInstruction(pBuilder->getBlock(),
                                           CmpMInstruction::SLT,
                                           new MachineOperand(MachineOperand::VREG, this->result->get_Ref()),
                                           new MachineOperand(MachineOperand::VREG, this->src1->get_Ref()),
                                           new MachineOperand(MachineOperand::VREG, this->src2->get_Ref()));
            pBuilder->getBlock()->insertInst(slt);
        }else if(RefType(this->src1->type) == IntVar && RefType(this->src2->type) == IntConst){
            loadImmIntoReg(pBuilder,strtol(this->src2->get_Ref().c_str(), nullptr, 10));
            auto slt = new CmpMInstruction(pBuilder->getBlock(),
                                           CmpMInstruction::SLT,
                                           new MachineOperand(MachineOperand::VREG, this->result->get_Ref()),
                                           new MachineOperand(MachineOperand::VREG, this->src1->get_Ref()),
                                           new MachineOperand(MachineOperand::VREG, getLastVRegister()));
            pBuilder->getBlock()->insertInst(slt);
        }else if(RefType(this->src1->type) == IntConst && RefType(this->src2->type) == IntVar){
            loadImmIntoReg(pBuilder,strtol(this->src1->get_Ref().c_str(), nullptr, 10));
            auto slt = new CmpMInstruction(pBuilder->getBlock(),
                                           CmpMInstruction::SLT,
                                           new MachineOperand(MachineOperand::VREG, this->result->get_Ref()),
                                           new MachineOperand(MachineOperand::VREG, getLastVRegister()),
                                           new MachineOperand(MachineOperand::VREG, this->src2->get_Ref()));
            pBuilder->getBlock()->insertInst(slt);
        }
        else if(RefType(this->src1->type) == FloatVar && RefType(this->src2->type) == FloatVar){
            FCmp(pBuilder, CmpMInstruction::FLT, this->result->get_Ref(), this->src1->get_Ref(), this->src2->get_Ref());
        }else if(RefType(this->src1->type) == FloatVar && RefType(this->src2->type) == FloatConst){
            isFloatConst(this->src2);
            LoadLabel(pBuilder, getVRegister(), getLC());
            LoadFloat(pBuilder, getFVRegister(), getLastVRegister());
            FCmp(pBuilder, CmpMInstruction::FLT, this->result->get_Ref(), this->src1->get_Ref(), getLastFVRegister());
        }else if(RefType(this->src1->type) == FloatConst && RefType(this->src2->type) == FloatVar){
            isFloatConst(this->src1);
            LoadLabel(pBuilder, getVRegister(), getLC());
            LoadFloat(pBuilder, getFVRegister(), getLastVRegister());
            FCmp(pBuilder, CmpMInstruction::FLT, this->result->get_Ref(), getLastFVRegister(), this->src2->get_Ref());
        }
        if(cmpType(this->opTy) == GE)
            xoriReg(pBuilder, new MachineOperand(MachineOperand::VREG, this->result->get_Ref()));
    }
    else if(cmpType(this->opTy) == LE || cmpType(this->opTy) == GT){
        if(RefType(this->src1->type) == IntVar && RefType(this->src2->type) == IntVar){
            auto slt = new CmpMInstruction(pBuilder->getBlock(),
                                           CmpMInstruction::SLT,
                                           new MachineOperand(MachineOperand::VREG, this->result->get_Ref()),
                                           new MachineOperand(MachineOperand::VREG, this->src2->get_Ref()),
                                           new MachineOperand(MachineOperand::VREG, this->src1->get_Ref()));
            pBuilder->getBlock()->insertInst(slt);
        }else if(RefType(this->src1->type) == IntVar && RefType(this->src2->type) == IntConst){
            loadImmIntoReg(pBuilder, strtol(this->src2->get_Ref().c_str(), nullptr, 10));
            auto slt = new CmpMInstruction(pBuilder->getBlock(),
                                           CmpMInstruction::SLT,
                                           new MachineOperand(MachineOperand::VREG, this->result->get_Ref()),
                                           new MachineOperand(MachineOperand::VREG, getLastVRegister()),
                                           new MachineOperand(MachineOperand::VREG, this->src1->get_Ref()));
            pBuilder->getBlock()->insertInst(slt);
        }else if(RefType(this->src1->type) == IntConst && RefType(this->src2->type) == IntVar){
            loadImmIntoReg(pBuilder, strtol(this->src1->get_Ref().c_str(), nullptr, 10));
            auto slt = new CmpMInstruction(pBuilder->getBlock(),
                                           CmpMInstruction::SLT,
                                           new MachineOperand(MachineOperand::VREG, this->result->get_Ref()),
                                           new MachineOperand(MachineOperand::VREG, this->src2->get_Ref()),
                                           new MachineOperand(MachineOperand::VREG, getLastVRegister()));
            pBuilder->getBlock()->insertInst(slt);
        }
        else if(RefType(this->src1->type) == FloatVar && RefType(this->src2->type) == FloatVar){
            FCmp(pBuilder, CmpMInstruction::FGT, this->result->get_Ref(), this->src1->get_Ref(), this->src2->get_Ref());
        }else if(RefType(this->src1->type) == FloatVar && RefType(this->src2->type) == FloatConst){
            isFloatConst(this->src2);
            LoadLabel(pBuilder, getVRegister(), getLC());
            LoadFloat(pBuilder, getFVRegister(), getLastVRegister());
            FCmp(pBuilder, CmpMInstruction::FGT, this->result->get_Ref(), this->src1->get_Ref(), getLastFVRegister());
        }else if(RefType(this->src1->type) == FloatConst && RefType(this->src2->type) == FloatVar){
            isFloatConst(this->src1);
            LoadLabel(pBuilder, getVRegister(), getLC());
            LoadFloat(pBuilder, getFVRegister(), getLastVRegister());
            FCmp(pBuilder, CmpMInstruction::FGT, this->result->get_Ref(), getLastFVRegister(), this->src2->get_Ref());
        }
        if(cmpType(this->opTy) == LE)
            xoriReg(pBuilder, new MachineOperand(MachineOperand::VREG, this->result->get_Ref()));
    }
}

void CmpInstruction::replace(ValueRef *old, ValueRef *now) {
    if(this->src1 == old) this->src1 = now;
    if(this->src2 == old) this->src2 = now;
    if(this->use_list.count(old)) this->use_list.erase(old);
    this->use_list.insert(now);

    now->use.push_back(this);
}

BrInstruction::BrInstruction(BasicBlock* label) : IRInstruction(BR){
    this->label = label;
    label->use.push_back(this);
}

void BrInstruction::codegen(AsmBuilder *pBuilder, std::map<std::string, int> &offset_table,
                            std::map<std::string, int> &size_table, int frameSize) {
    auto br = new JumpMInstruction(pBuilder->getBlock(),
                                   JumpMInstruction::J,
                                   new MachineOperand(MachineOperand::LABEL, this->label->label));
    pBuilder->getBlock()->insertInst(br);
}

void BrInstruction::outPut(std::ostream &os) {
    os << "br label %" + label->label << endl;
}

void BrInstruction::replace(ValueRef *old, ValueRef *now) {
    if(this->label == old) this->label = dynamic_cast<BasicBlock*>(now);
    if(this->use_list.count(old)) this->use_list.erase(old);
    this->use_list.insert(now);

    now->use.push_back(this);
}

CondBrInstruction::CondBrInstruction(ValueRef* condition, BasicBlock* trueLabel, BasicBlock* falseLabel) : IRInstruction(CONDBR){
    this->condition = condition;
    this->trueLabel = trueLabel;
    this->falseLabel = falseLabel;

    addUse(condition);
    trueLabel->use.push_back(this);
    falseLabel->use.push_back(this);
}

void CondBrInstruction::codegen(AsmBuilder *pBuilder, map<std::string, int> &offset_table, map<std::string, int> &size_table,
                                int frameSize) {
    if(this->condition->get_Ref() == "1"){
        JumpLabel(pBuilder, this->trueLabel->label);
    }
    else if(this->condition->get_Ref() == "0"){
        JumpLabel(pBuilder, this->falseLabel->label);
    }
    else{
        auto bne = new BranchMInstruction(pBuilder->getBlock(),
                                          BranchMInstruction::BNE,
                                          new MachineOperand(MachineOperand::VREG,this->condition->get_Ref()),
                                          new MachineOperand(MachineOperand::REG,ZERO),
                                          new MachineOperand(MachineOperand::LABEL,this->trueLabel->label));
        pBuilder->getBlock()->insertInst(bne);
        JumpLabel(pBuilder, this->falseLabel->label);
    }
}

void CondBrInstruction::outPut(std::ostream &os) {
    os << "br i1 " + condition->get_Ref() + ", label %" + trueLabel->label + ", label %" + falseLabel->label << endl;
}

void CondBrInstruction::replace(ValueRef *old, ValueRef *now) {
    if(this->trueLabel == old) this->trueLabel = dynamic_cast<BasicBlock*>(now);
    if(this->falseLabel == old) this->falseLabel = dynamic_cast<BasicBlock*>(now);
    if(this->condition == old) this->condition = now;
    if(this->use_list.count(old)) this->use_list.erase(old);
    this->use_list.insert(now);

    now->use.push_back(this);
}

RetInstruction::RetInstruction(ValueRef* retVal) : IRInstruction(RET){
    this->retVal = retVal;
    if(retVal != nullptr){
        addUse(retVal);
    }
}
void RetInstruction::codegen(AsmBuilder *pBuilder, std::map<std::string, int> &offset_table,
                             std::map<std::string, int> &size_table, int frameSize) {
    if(this->retVal == nullptr){

    }else{
        if(RefType(this->retVal->type) == IntConst){
//        cout << "retIntConst" << endl;
            long value = strtol(this->retVal->get_Ref().c_str(), nullptr, 10);
            if(abs(value) > 2000){
                loadImmIntoReg(pBuilder, value);
                auto addvregToA0 = new BinaryMInstruction(pBuilder->getBlock(),
                                                          BinaryMInstruction::ADD,
                                                          new MachineOperand(MachineOperand::REG, A0),
                                                          new MachineOperand(MachineOperand::REG, ZERO),
                                                          new MachineOperand(MachineOperand::VREG, getLastVRegister()));
                pBuilder->getBlock()->insertInst(addvregToA0);
            }
            else{
                auto addImmToA0 = new BinaryMInstruction(pBuilder->getBlock(),
                                                         BinaryMInstruction::ADDI,
                                                         new MachineOperand(MachineOperand::REG, A0),
                                                         new MachineOperand(MachineOperand::REG, ZERO),
                                                         new MachineOperand(MachineOperand::IMM, (int)value));
                pBuilder->getBlock()->insertInst(addImmToA0);
            }
        }else if(RefType(this->retVal->type) == IntVar){
//        cout << "retIntVar" << endl;
            auto addIntToA0 = new BinaryMInstruction(pBuilder->getBlock(),
                                                     BinaryMInstruction::ADDI,
                                                     new MachineOperand(MachineOperand::REG, A0),
                                                     new MachineOperand(MachineOperand::VREG, this->retVal->get_Ref()),
                                                     new MachineOperand(MachineOperand::IMM, 0));
            pBuilder->getBlock()->insertInst(addIntToA0);
        }else if(RefType(this->retVal->type) == FloatConst){
            isFloatConst(this->retVal);
            LoadLabel(pBuilder, getVRegister(), getLC());
            LoadFloat(pBuilder, getFVRegister(), getLastVRegister());
            auto mv = new MoveMInstruction(pBuilder->getBlock(),
                                           MoveMInstruction::FMV,
                                           new MachineOperand(MachineOperand::REG, static_cast<int>(Fregister::fa0), true),
                                           new MachineOperand(MachineOperand::FVREG, getLastFVRegister()));
            pBuilder->getBlock()->insertInst(mv);
        }else if(RefType(this->retVal->type) == FloatVar){
            auto mv = new MoveMInstruction(pBuilder->getBlock(),
                                           MoveMInstruction::FMV,
                                           new MachineOperand(MachineOperand::REG, Fregister::fa0, true),
                                           new MachineOperand(MachineOperand::FVREG, this->retVal->get_Ref()));
            pBuilder->getBlock()->insertInst(mv);
        }
    }
    retFunc(pBuilder, frameSize-16);
}

void RetInstruction::outPut(std::ostream &os) {
    string code;
    if (retVal == nullptr)
        code = "ret void";
    else {
        code = "ret " + retVal->get_TypeName() + ' ' + retVal->get_Ref();
    }
    os << code << endl;
}

void RetInstruction::replace(ValueRef *old, ValueRef *now) {
    if(this->retVal == old) this->retVal = now;
    if(this->use_list.count(old)) this->use_list.erase(old);
    this->use_list.insert(now);

    now->use.push_back(this);
}

CallInstruction::CallInstruction(ValueRef* retVal, Function* function, vector<ValueRef*>& args) : IRInstruction(CALL){
    this->retVal = retVal;
    this->function = function;
    this->args = args;

    if(retVal != nullptr) // exclude void func
        addDef(retVal);
    for (auto arg : args)
        addUse(arg);
}

static MachineOperand *getMOperandReg(IREGISTER physical_reg);
static MachineOperand *getMOperandImm(int imm);

void CallInstruction::codegen(AsmBuilder *pBuilder, std::map<std::string, int> &offset_table,
                              std::map<std::string, int> &size_table, int frameSize) {
    if(this->args.empty()){
        auto jal = new JalMInstruction(pBuilder->getBlock(),
                                       JalMInstruction::JAL,
                                       new MachineOperand(MachineOperand::REG, RA),
                                       new MachineOperand(MachineOperand::LABEL, this->function->name));
        pBuilder->getBlock()->insertInst(jal);
    } else{
        for(int i=0; i<this->args.size() && i<=7; i++){
            if(RefType(this->args[i]->type) == IntVar){
                auto addArgument = new BinaryMInstruction(pBuilder->getBlock(),
                                                          BinaryMInstruction::ADDI,
                                                          new MachineOperand(MachineOperand::REG, IREGISTER::A0+i),//x2(sp)
                                                          new MachineOperand(MachineOperand::VREG, this->args[i]->get_Ref()),
                                                          new MachineOperand(MachineOperand::IMM, 0));
                pBuilder->getBlock()->insertInst(addArgument);
            }
            else if(RefType(this->args[i]->type) == IntConst){
                int imm = strtol(this->args[i]->get_Ref().c_str(), nullptr, 10);
                if(abs(imm) > 2000){
                    loadImmIntoReg(pBuilder, imm);
                    auto addArgument = new BinaryMInstruction(pBuilder->getBlock(),
                                                              BinaryMInstruction::ADD,
                                                              new MachineOperand(MachineOperand::REG, IREGISTER::A0+i),//x2(sp)
                                                              new MachineOperand(MachineOperand::REG, ZERO),
                                                              new MachineOperand(MachineOperand::VREG,getLastVRegister()));
                    pBuilder->getBlock()->insertInst(addArgument);
                }
                else{
                    auto addArgument = new BinaryMInstruction(pBuilder->getBlock(),
                                                              BinaryMInstruction::ADDI,
                                                              new MachineOperand(MachineOperand::REG, IREGISTER::A0+i),//x2(sp)
                                                              new MachineOperand(MachineOperand::REG, ZERO),
                                                              new MachineOperand(MachineOperand::IMM, strtol(this->args[i]->get_Ref().c_str(), nullptr, 10)));
                    pBuilder->getBlock()->insertInst(addArgument);
                }
            }
            else if(RefType(this->args[i]->type) == Ptr){
//                    cout << "arr" << endl;
                auto addArgument = new BinaryMInstruction(pBuilder->getBlock(),
                                                          BinaryMInstruction::ADDI,
                                                          new MachineOperand(MachineOperand::REG, IREGISTER::A0+i),//x2(sp)
                                                          new MachineOperand(MachineOperand::VREG, this->args[i]->get_Ref()),
                                                          new MachineOperand(MachineOperand::IMM, 0));
                pBuilder->getBlock()->insertInst(addArgument);
            }else if(RefType(this->args[i]->type) == FloatVar){
                auto fmv = new MoveMInstruction(pBuilder->getBlock(),
                                                MoveMInstruction::FMV,
                                                new MachineOperand(MachineOperand::REG, Fregister::fa0+i, true),
                                                new MachineOperand(MachineOperand::FVREG, this->args[i]->get_Ref()));
                pBuilder->getBlock()->insertInst(fmv);
            }else if(RefType(this->args[i]->type) == FloatConst){
                isFloatConst(this->args[i]);
                LoadLabel(pBuilder, getVRegister(), getLC());
                LoadFloat(pBuilder,getFVRegister(), getLastVRegister());
                auto fmv = new MoveMInstruction(pBuilder->getBlock(),
                                                MoveMInstruction::FMV,
                                                new MachineOperand(MachineOperand::REG, Fregister::fa0+i, true),
                                                new MachineOperand(MachineOperand::FVREG, getLastFVRegister()));
                pBuilder->getBlock()->insertInst(fmv);
            }
        }
        int offset = 0;
        if(this->args.size()>8){
            //        more than 8
            if ((args.size() - 8) & 1) offset += 8;
            // auto addi = new BinaryMInstruction(pBuilder->getBlock(), BinaryMInstruction::ADDI,
            //                                             new MachineOperand(MachineOperand::REG, IREGISTER::SP),
            //                                             new MachineOperand(MachineOperand::REG, IREGISTER::SP),
            //                                             new MachineOperand(MachineOperand::IMM, - offset - (args.size()- 8) * 8));
            loadImmIntoReg(pBuilder, - offset - (args.size() - 8) * 8);
            offset = 0;
            auto add = new BinaryMInstruction(pBuilder->getBlock(), BinaryMInstruction::ADD,
                                              getMOperandReg(IREGISTER::SP), getMOperandReg(IREGISTER::SP),
                                              new MachineOperand(MachineOperand::VREG, getLastVRegister()));
            pBuilder->getBlock()->insertInst(add);

            for(int i = 8; i < this->args.size(); i++){
                if(RefType(this->args[i]->type) == IntConst){
                    loadImmIntoReg(pBuilder, strtol(this->args[i]->get_Ref().c_str(), nullptr, 10));
                    if (offset <= 2032 && false) {
                        auto store = new StoreMInstruction(pBuilder->getBlock(),
                                                           StoreMInstruction::SW,
                                                           new MachineOperand(MachineOperand::VREG, getLastVRegister()),
                                                           new MachineOperand(MachineOperand::IMM, offset),
                                                           new MachineOperand(MachineOperand::REG, IREGISTER::SP));
                        pBuilder->getBlock()->insertInst(store);
                    } else {
                        loadImmIntoReg(pBuilder, offset);

                        auto add = new BinaryMInstruction(pBuilder->getBlock(), BinaryMInstruction::ADD,
                                                          new MachineOperand(MachineOperand::VREG, getLastVRegister(1)),
                                                          new MachineOperand(MachineOperand::VREG, getLastVRegister(1)),
                                                          getMOperandReg(IREGISTER::SP));
                        pBuilder->getBlock()->insertInst(add);

                        auto store = new StoreMInstruction(pBuilder->getBlock(), StoreMInstruction::SW,
                                                           new MachineOperand(MachineOperand::VREG, getLastVRegister(2)),
                                                           getMOperandImm(0),
                                                           new MachineOperand(MachineOperand::VREG, getLastVRegister(1)));
                        pBuilder->getBlock()->insertInst(store);
                    }
                    offset += 8;
                }else if(RefType(this->args[i]->type) == IntVar){
                    if (offset <= 2032) {
                        auto store = new StoreMInstruction(pBuilder->getBlock(),
                                                       StoreMInstruction::SW,
                                                       new MachineOperand(MachineOperand::VREG, this->args[i]->get_Ref()),
                                                       new MachineOperand(MachineOperand::IMM, offset),
                                                       new MachineOperand(MachineOperand::REG, IREGISTER::SP));
                        pBuilder->getBlock()->insertInst(store);
                    } else {
                        loadImmIntoReg(pBuilder, offset);
                        auto add = new BinaryMInstruction(pBuilder->getBlock(), BinaryMInstruction::ADD,
                                                                new MachineOperand(MachineOperand::VREG, getLastVRegister()),
                                                                new MachineOperand(MachineOperand::VREG, getLastVRegister()),
                                                                getMOperandReg(IREGISTER::SP));
                        pBuilder->getBlock()->insertInst(add);
                        auto store = new StoreMInstruction(pBuilder->getBlock(), StoreMInstruction::SW,
                                                                new MachineOperand(MachineOperand::VREG, this->args[i]->get_Ref()),
                                                                new MachineOperand(MachineOperand::IMM, 0),
                                                                new MachineOperand(MachineOperand::VREG, getLastVRegister()));
                        pBuilder->getBlock()->insertInst(store);
                    }
                   offset += 8;
                }else if(RefType(this->args[i]->type) == Ptr){
                    if (offset <= 2032) {
                        auto store = new StoreMInstruction(pBuilder->getBlock(),
                                                           StoreMInstruction::SD,
                                                           new MachineOperand(MachineOperand::VREG, this->args[i]->get_Ref()),
                                                           new MachineOperand(MachineOperand::IMM, offset),
                                                           new MachineOperand(MachineOperand::REG, IREGISTER::SP));
                        pBuilder->getBlock()->insertInst(store);
                    } else {
                        loadImmIntoReg(pBuilder, offset);

                        auto add = new BinaryMInstruction(pBuilder->getBlock(), BinaryMInstruction::ADD,
                                                          new MachineOperand(MachineOperand::VREG, getLastVRegister()),
                                                          new MachineOperand(MachineOperand::VREG, getLastVRegister()),
                                                          getMOperandReg(IREGISTER::SP));
                        pBuilder->getBlock()->insertInst(add);

                        auto store = new StoreMInstruction(pBuilder->getBlock(),
                                                           StoreMInstruction::SD,
                                                           new MachineOperand(MachineOperand::VREG, this->args[i]->get_Ref()),
                                                           getMOperandImm(0),
                                                           new MachineOperand(MachineOperand::VREG, getLastVRegister()));
                        pBuilder->getBlock()->insertInst(store);
                    }
                    offset += 8;
                }
                else if(RefType(this->args[i]->type) == FloatVar){
                    if (offset <= 2032) {
                        auto store = new StoreMInstruction(pBuilder->getBlock(),
                                                           StoreMInstruction::FSW,
                                                           new MachineOperand(MachineOperand::FVREG, this->args[i]->get_Ref()),
                                                           new MachineOperand(MachineOperand::IMM, offset),
                                                           new MachineOperand(MachineOperand::REG, IREGISTER::SP));
                        pBuilder->getBlock()->insertInst(store);
                    } else {
                        loadImmIntoReg(pBuilder, offset);
                        auto add = new BinaryMInstruction(pBuilder->getBlock(), BinaryMInstruction::ADD,
                                                          new MachineOperand(MachineOperand::VREG, getLastVRegister()),
                                                          new MachineOperand(MachineOperand::VREG, getLastVRegister()),
                                                          getMOperandReg(IREGISTER::SP));
                        pBuilder->getBlock()->insertInst(add);
                        auto store = new StoreMInstruction(pBuilder->getBlock(),
                                                           StoreMInstruction::FSW,
                                                           new MachineOperand(MachineOperand::FVREG, this->args[i]->get_Ref()),
                                                           new MachineOperand(MachineOperand::IMM, 0),
                                                           new MachineOperand(MachineOperand::VREG, getLastVRegister()));
                        pBuilder->getBlock()->insertInst(store);
                    }
                    offset += 8;
                }else if(RefType(this->args[i]->type) == FloatConst){
                    if (offset <= 2032) {
                        isFloatConst(this->args[i]);
                        LoadLabel(pBuilder, getVRegister(), getLC());
                        LoadFloat(pBuilder,getFVRegister(), getLastVRegister());
                        auto store = new StoreMInstruction(pBuilder->getBlock(),
                                                           StoreMInstruction::FSW,
                                                           new MachineOperand(MachineOperand::FVREG, getLastFVRegister()),
                                                           new MachineOperand(MachineOperand::IMM, offset),
                                                           new MachineOperand(MachineOperand::REG, IREGISTER::SP));
                        pBuilder->getBlock()->insertInst(store);
                    } else {
                        isFloatConst(this->args[i]);
                        LoadLabel(pBuilder, getVRegister(), getLC());
                        LoadFloat(pBuilder,getFVRegister(), getLastVRegister());
                        loadImmIntoReg(pBuilder, offset);
                        auto add = new BinaryMInstruction(pBuilder->getBlock(), BinaryMInstruction::ADD,
                                                          new MachineOperand(MachineOperand::VREG, getLastVRegister()),
                                                          new MachineOperand(MachineOperand::VREG, getLastVRegister()),
                                                          getMOperandReg(IREGISTER::SP));
                        pBuilder->getBlock()->insertInst(add);
                        auto store = new StoreMInstruction(pBuilder->getBlock(),
                                                           StoreMInstruction::FSW,
                                                           new MachineOperand(MachineOperand::FVREG, getLastFVRegister()),
                                                           new MachineOperand(MachineOperand::IMM, 0),
                                                           new MachineOperand(MachineOperand::VREG, getLastVRegister()));
                        pBuilder->getBlock()->insertInst(store);
                    }
                    offset += 8;
                }
            }
        }
        auto jal = new JalMInstruction(pBuilder->getBlock(),
                                       JalMInstruction::JAL,
                                       new MachineOperand(MachineOperand::REG, RA),
                                       new MachineOperand(MachineOperand::LABEL, this->function->name));
        pBuilder->getBlock()->insertInst(jal);
        if (this->args.size() > 8) {
            // auto addi = new BinaryMInstruction(pBuilder->getBlock(), BinaryMInstruction::ADDI,
            //                                             new MachineOperand(MachineOperand::REG, IREGISTER::SP),
            //                                             new MachineOperand(MachineOperand::REG, IREGISTER::SP),
            //                                             new MachineOperand(MachineOperand::IMM, offset));
            // pBuilder->getBlock()->insertInst(addi);
            loadImmIntoReg(pBuilder, (args.size() - 8) & 1 ? offset + 8 : offset);
            auto add = new BinaryMInstruction(pBuilder->getBlock(), BinaryMInstruction::ADD,
                                              getMOperandReg(IREGISTER::SP), getMOperandReg(IREGISTER::SP),
                                              new MachineOperand(MachineOperand::VREG, getLastVRegister()));
            pBuilder->getBlock()->insertInst(add);
        }
    }
//    return val
    if(Type_Enum(this->function->funcType->retType->type) == INT32TYPE){
        auto addi = new BinaryMInstruction(pBuilder->getBlock(),
                                           BinaryMInstruction::ADDI,
                                           new MachineOperand(MachineOperand::VREG, this->retVal->get_Ref()),
                                           new MachineOperand(MachineOperand::REG, A0),
                                           new MachineOperand(MachineOperand::IMM, 0));
        pBuilder->getBlock()->insertInst(addi);
    }else if(Type_Enum(this->function->funcType->retType->type) == FLOATTYPE){
        auto fmv = new MoveMInstruction(pBuilder->getBlock(),
                                        MoveMInstruction::FMV,
                                        new MachineOperand(MachineOperand::FVREG, this->retVal->get_Ref()),
                                        new MachineOperand(MachineOperand::REG, Fregister::fa0, true));
        pBuilder->getBlock()->insertInst(fmv);
    }
}

void CallInstruction::outPut(ostream &os) {
    //e.g %0 = call i32 @func(i32 %1) -> normal call
    //    call void @foo(i32 %2)      -> void call
    string command;
    if(retVal != nullptr) {
        os << retVal->get_Ref();
    }
    switch(function->funcType->retType->type){
        case VOIDTYPE:{
            os << "call void @";
            break;
        }
        case INT32TYPE:{
            os << " = call i32 @";
            break;
        }
        case FLOATTYPE:{
            os << " = call float @";
            break;
        }
        default: {
            cerr << "Return type is not basic type!!!" << endl;
            throw exception();
        }
    }

    os << function->name << '(';
    // add params
    bool not_first = false;
    for(ValueRef * ref: args){
        if(not_first) os << ",";
        not_first = true;
        os << ref->get_TypeName() + ' ' + ref->get_Ref();
    }
    os << ')' << endl;
}

void CallInstruction::replace(ValueRef *old, ValueRef *now) {
    for(int i=0;i<args.size();++i){
        if(args[i] == old) args[i] = now;
    }
    if(this->use_list.count(old)) this->use_list.erase(old);
    this->use_list.insert(now);

    now->use.push_back(this);
}

ZExtInstruction::ZExtInstruction(ValueRef* dst,ValueRef* src) : IRInstruction(ZEXT){
    this->dst = dst;
    this->src = src;

    addDef(dst);
    addUse(src);
}

void
ZExtInstruction::codegen(AsmBuilder *pBuilder, map<std::string, int> &offset_table, map<std::string, int> &size_table,
                         int frameSize) {
    auto and1 = new BinaryMInstruction(pBuilder->getBlock(),
                                       BinaryMInstruction::ANDI,
                                       new MachineOperand(MachineOperand::VREG, this->dst->get_Ref()),
                                       new MachineOperand(MachineOperand::VREG, this->src->get_Ref()),
                                       new MachineOperand(MachineOperand::IMM, 1));
    pBuilder->getBlock()->insertInst(and1);
}

void ZExtInstruction::outPut(std::ostream &os) {
    os << dst->name + " = zext i1 " + src->get_Ref() + " to i32" << endl;
}

void ZExtInstruction::replace(ValueRef *old, ValueRef *now) {
    if(this->src == old) this->src = now;
    if(this->use_list.count(old)) this->use_list.erase(old);
    this->use_list.insert(now);

    now->use.push_back(this);
}

XorInstruction::XorInstruction(ValueRef* dst, ValueRef* src) : IRInstruction(XOR){
    this->dst = dst;
    this->src = src;

    addDef(dst);
    addUse(src);
}

void
XorInstruction::codegen(AsmBuilder *pBuilder, map<std::string, int> &offset_table, map<std::string, int> &size_table,
                        int frameSize) {
//    if i32 then false
    auto xor1 = new BinaryMInstruction(pBuilder->getBlock(),
                                       BinaryMInstruction::XORI,
                                       new MachineOperand(MachineOperand::VREG, this->dst->get_Ref()),
                                       new MachineOperand(MachineOperand::VREG, this->src->get_Ref()),
                                       new MachineOperand(MachineOperand::IMM, 1));
    pBuilder->getBlock()->insertInst(xor1);
}

void XorInstruction::outPut(std::ostream &os) {
    os << dst->name + " = xor i1 " + src->get_Ref() + ", true" << endl;
}

void XorInstruction::replace(ValueRef *old, ValueRef *now) {
    if(this->src == old) this->src = now;
    if(this->use_list.count(old)) this->use_list.erase(old);
    this->use_list.insert(now);

    now->use.push_back(this);
}

GEPInstruction::GEPInstruction(ValueRef* dst, ValueRef* src, ValueRef* index) : IRInstruction(GEP){
    this->dst = dst;
    this->src = src;
    this->index = index;

    addDef(dst);
    addUse(src);
    addUse(index);
}

void
GEPInstruction::codegen(AsmBuilder *pBuilder, map<std::string, int> &offset_table, map<std::string, int> &size_table,
                        int frameSize) {
    auto offsetIter = offset_table.find(this->src->get_Ref());

    if(offsetIter != offset_table.end()){
//        cout << "basic gep" << this->src->get_Ref() << endl;
//        从符号表种get，也就是说只有一种情况：从符号表种get，且一定是多维数组的第一次get
//        在这里，是对地址（ptr）做操作，int和float是一样的
//        第一步：找到首地址 第二步：load
        int start_offset = -16-size_table[this->src->get_Ref()] - offsetIter->second;
        if (abs(start_offset) < 2040) {
            auto bin = new BinaryMInstruction(pBuilder->getBlock(),
                                              BinaryMInstruction::ADDI,
                                              new MachineOperand(MachineOperand::VREG, this->dst->get_Ref()),
                                              new MachineOperand(MachineOperand::REG, IREGISTER::FP),
                                              new MachineOperand(MachineOperand::IMM,
                                                      // previously `fp - 16 - 4 - offset`, now `fp - 16 - size of array - offset`
                                                                 -16 - size_table[this->src->get_Ref()] -offsetIter->second));
            pBuilder->getBlock()->insertInst(bin);
        } else {
            loadImmIntoReg(pBuilder, start_offset);
            auto bin = new BinaryMInstruction(pBuilder->getBlock(), BinaryMInstruction::ADD,
                                              new MachineOperand(MachineOperand::VREG, this->dst->get_Ref()),
                                              new MachineOperand(MachineOperand::REG, IREGISTER::FP),
                                              new MachineOperand(MachineOperand::VREG, getLastVRegister()));
            pBuilder->getBlock()->insertInst(bin);
        }
//        cout << this->index->get_Ref() << endl;
        if(RefType(this->index->type) == IntConst) {
            int offset = strtol(this->index->get_Ref().c_str(), nullptr, 10);
            int len = this->src->getElementSize();
            int offsetFromStart = offset * len;
            Addiw(pBuilder, BinaryMInstruction::ADDI, this->dst->get_Ref(), this->dst->get_Ref(), offsetFromStart);
        }
        else if(RefType(this->index->type) == IntVar){
            int elementsize = this->src->getElementSize();
            if((elementsize & (elementsize - 1)) == 0){
                int bits = log2(elementsize);
                auto sli = new BinaryMInstruction(pBuilder->getBlock(),BinaryMInstruction::slliw,
                                              new MachineOperand(MachineOperand::VREG, getVRegister()),
                                              new MachineOperand(MachineOperand::VREG, index->get_Ref()),
                                              new MachineOperand(MachineOperand::IMM, bits));
                pBuilder->getBlock()->insertInst(sli);
            }
            else{
                loadImmIntoReg(pBuilder, this->src->getElementSize());
                std::string lastVRegister = getLastVRegister();
                BinaryInstr(pBuilder, BinaryMInstruction::MULW, getVRegister(), index->get_Ref(), lastVRegister);
            }
            BinaryInstr(pBuilder, BinaryMInstruction::ADD, this->dst->get_Ref(), this->dst->get_Ref(), getLastVRegister());
        }

    }else{
        //        不是从符号表种get，也就是说可能有两种情况：一种是从get到的数组再get，还有一种是从全局get
//        同样的，这里也是对地址（ptr）做操作，int和float是一样的
        if(this->src->get_Ref()[0] == '@'){
            auto la = new LoadImmInstruction(pBuilder->getBlock(),
                                             LoadImmInstruction::LA,
                                             new MachineOperand(MachineOperand::VREG, getVRegister()),
                                             new MachineOperand(MachineOperand::LABEL, this->src->get_Ref().substr(1)));
            pBuilder->getBlock()->insertInst(la);
            if(RefType(this->index->type) == IntConst) {
                int offset = strtol(this->index->get_Ref().c_str(), nullptr, 10);
                int len = this->src->getElementSize();
                int offsetFromStart = offset * len;
                Addiw(pBuilder, BinaryMInstruction::ADDI, this->dst->get_Ref(), getLastVRegister(), offsetFromStart);
            }
            else if(RefType(this->index->type) == IntVar){
                int elementsize = this->src->getElementSize();
                if((elementsize & (elementsize - 1)) == 0){
                    int bits = log2(elementsize);
                    auto sli = new BinaryMInstruction(pBuilder->getBlock(),BinaryMInstruction::slliw,
                                                  new MachineOperand(MachineOperand::VREG, getVRegister()),
                                                  new MachineOperand(MachineOperand::VREG, index->get_Ref()),
                                                  new MachineOperand(MachineOperand::IMM, bits));
                    pBuilder->getBlock()->insertInst(sli);
                    BinaryInstr(pBuilder, BinaryMInstruction::ADD, this->dst->get_Ref(), getLastVRegister(2), getLastVRegister());
                }
                else{
                    loadImmIntoReg(pBuilder, this->src->getElementSize());
                    std::string lastVRegister = getLastVRegister();
                    BinaryInstr(pBuilder, BinaryMInstruction::MULW, getVRegister(), index->get_Ref(), lastVRegister);
                    BinaryInstr(pBuilder, BinaryMInstruction::ADD, this->dst->get_Ref(), getLastVRegister(3), getLastVRegister());
                }
//                BinaryInstr(pBuilder, BinaryMInstruction::ADD, this->dst->get_Ref(), getLastVRegister(3), getLastVRegister());
            }
        }
//        multi dimensions
        else{
            if(RefType(this->index->type) == IntConst) {
                int offset = strtol(this->index->get_Ref().c_str(), nullptr, 10);
                int len = this->src->getElementSize();
                int offsetFromStart = offset * len;
                Addiw(pBuilder, BinaryMInstruction::ADDI, this->dst->get_Ref(), this->src->get_Ref(), offsetFromStart);
            }
            else if(RefType(this->index->type) == IntVar){
                int elementsize = this->src->getElementSize();
                if((elementsize & (elementsize - 1)) == 0){
                    int bits = log2(elementsize);
                    auto sli = new BinaryMInstruction(pBuilder->getBlock(),BinaryMInstruction::slliw,
                                                      new MachineOperand(MachineOperand::VREG, getVRegister()),
                                                      new MachineOperand(MachineOperand::VREG, index->get_Ref()),
                                                      new MachineOperand(MachineOperand::IMM, bits));
                    pBuilder->getBlock()->insertInst(sli);
                }
                else{
                    loadImmIntoReg(pBuilder, elementsize);
                    std::string lastVRegister = getLastVRegister();
                    BinaryInstr(pBuilder, BinaryMInstruction::MULW, getVRegister(), index->get_Ref(), lastVRegister);
                }
                BinaryInstr(pBuilder, BinaryMInstruction::ADD, this->dst->get_Ref(), this->src->get_Ref(),getLastVRegister());
            }
        }
    }
}

void GEPInstruction::outPut(ostream &os) {
    Type *ptrType; // 只有可能是指针类型或数组类型
    if(src->type == Arr){
        ptrType = ((Array*)src)->arrayType;
    }
    else if(src->type == Ptr){
        ptrType = ((Pointer*)src)->pointerType;
    }
    else{ // ref->type == Symbol
        ptrType = ((Symbol*)src)->symbolType;
    }

    if(ptrType->type == ARRAYTYPE){
        //数组的GEP: 数组类型* -> 数组类型
        os << dst->get_Ref() + " = getelementptr " + src->get_TypeName() + ", " + src->get_TypeName() + "* " + src->get_Ref() + ", i32 0, i32 " + index->get_Ref() << endl;
        //e.g %4 = getelementptr inbounds [2 x [2 x i32]], [2 x [2 x i32]]* %2, i64 0, i64 1
    }
    else if(ptrType->type == POINTERTYPE){
        //指针的GEP: 元素类型* -> 元素类型
        string eleName = ((PointerType*)ptrType)->elementType->getTypeName();
        os << dst->get_Ref() + " = getelementptr " + eleName + ", " + eleName + "* " + src->get_Ref() + ", i32 " + index->get_Ref() << endl;
        //e.g %4 = getelementptr inbounds [2 x [2 x i32]], [2 x [2 x i32]]* %2, i64 0
    }
    else{
        cerr << "Not a pointer/arr type!" << endl;
        throw exception();
    }
}

void GEPInstruction::replace(ValueRef *old, ValueRef *now) {
    if(this->src == old) this->src = now;
    if(this->index == old) this->index = now;
    if(this->use_list.count(old)) this->use_list.erase(old);
    this->use_list.insert(now);

    now->use.push_back(this);
}

PhiInstruction::PhiInstruction(ValueRef* symbol,ValueRef* result,int cnt) : IRInstruction(PHI){
    this->symbol = symbol;
    this->result = result;
    this->branch_cnt = cnt;

    addDef(result);
}

void PhiInstruction::outPut(std::ostream &os) {
    // %3 = phi i32 [ 0, %0 ], [ %10, %6 ]
    os << result->get_Ref() + " = phi ";
    os << ((result->type == IntVar)?"i32 " : "float ");
    bool not_first = false;
    for(auto[block,ref] : this->mp){
        if(not_first){
            os << ", ";
        }
        os << "[ " << ref->get_Ref() << ", %"<< block->label << " ]";
        not_first = true;
    }
    os << endl;
}

void PhiInstruction::replace(ValueRef *old, ValueRef *now) {
    if(old->type == Block){
        BasicBlock * bb = dynamic_cast<BasicBlock*>(old);
        if(mp.count(bb)){
            ValueRef * val = mp[bb];
            mp.erase(bb);
            mp[dynamic_cast<BasicBlock*>(now)] = val;
        }
    }
    else{
        for(auto &[block,ref] : mp){
            if(ref == old){
                mp[block] = now;
            }
        }
        if(this->use_list.count(old)) this->use_list.erase(old);
        this->use_list.insert(now);
    }

    now->use.push_back(this);
}

FPTIInstruction::FPTIInstruction(ValueRef* dst, ValueRef* src) : IRInstruction(FPTI){
    this->dst = dst;
    this->src = src;

    addDef(dst);
    addUse(src);
}

void FPTIInstruction::outPut(std::ostream &os) {
    os << dst->get_Ref() + " = fptosi float "+ src->get_Ref() + " to i32" << endl;
}

void FPTIInstruction::replace(ValueRef *old, ValueRef *now) {
    if(this->src == old) this->src = now;
    if(this->use_list.count(old)) this->use_list.erase(old);
    this->use_list.insert(now);

    now->use.push_back(this);
}
void
FPTIInstruction::codegen(AsmBuilder *pBuilder, map<std::string, int> &offset_table, map<std::string, int> &size_table,
                         int frameSize) {
    auto f2i = new ConvertMInstruction(pBuilder->getBlock(),
                                       ConvertMInstruction::F2I,
                                       new MachineOperand(MachineOperand::VREG, this->dst->get_Ref()),
                                       new MachineOperand(MachineOperand::FVREG, this->src->get_Ref()));
    pBuilder->getBlock()->insertInst(f2i);
}

ITFPInstruction::ITFPInstruction(ValueRef *dst, ValueRef *src) : IRInstruction(ITFP){
    this->dst = dst;
    this->src = src;

    addDef(dst);
    addUse(src);
}

void ITFPInstruction::outPut(std::ostream &os) {
    os << dst->get_Ref() + " = sitofp i32 "+ src->get_Ref() + " to float" << endl;
}

void ITFPInstruction::replace(ValueRef *old, ValueRef *now) {
    if(this->src == old) this->src = now;
    if(this->use_list.count(old)) this->use_list.erase(old);
    this->use_list.insert(now);

    now->use.push_back(this);
}
void
ITFPInstruction::codegen(AsmBuilder *pBuilder, map<std::string, int> &offset_table, map<std::string, int> &size_table,
                         int frameSize) {
    auto i2f = new ConvertMInstruction(pBuilder->getBlock(),
                                       ConvertMInstruction::I2F,
                                       new MachineOperand(MachineOperand::FVREG, this->dst->get_Ref()),
                                       new MachineOperand(MachineOperand::VREG, this->src->get_Ref()));
    pBuilder->getBlock()->insertInst(i2f);
}


FNegInstruction::FNegInstruction(ValueRef *dst, ValueRef *src) : IRInstruction(FNEG){
    this->dst = dst;
    this->src = src;

    addDef(dst);
    addUse(src);
}

void FNegInstruction::outPut(std::ostream &os) {
    os << dst->name + " = fneg float " + src->get_Ref() << endl;
}

void FNegInstruction::replace(ValueRef *old, ValueRef *now) {
    if(this->src == old) this->src = now;
    if(this->use_list.count(old)) this->use_list.erase(old);
    this->use_list.insert(now);

    now->use.push_back(this);
}

void
FNegInstruction::codegen(AsmBuilder *pBuilder, map<std::string, int> &offset_table, map<std::string, int> &size_table,
                         int frameSize) {
    auto fneg = new FnegMInstruction(pBuilder->getBlock(),
                                     FnegMInstruction::FNEG,
                                     new MachineOperand(MachineOperand::FVREG, this->dst->get_Ref()),
                                     new MachineOperand(MachineOperand::FVREG, this->src->get_Ref()));
    pBuilder->getBlock()->insertInst(fneg);
}


//BitCastInstruction::BitCastInstruction(BasicBlock *block) : IRInstruction(BITCAST){}
//
//ShlInstruction::ShlInstruction(BasicBlock *block) : IRInstruction(SHL){}
//
//AShrInstruction::AShrInstruction(BasicBlock *block) : IRInstruction(ASHR){}

static MachineOperand *getMOperandReg(IREGISTER physical_reg) {
    return new MachineOperand(MachineOperand::REG, physical_reg);
}
static MachineOperand *getMOperandImm(int imm) {
    return new MachineOperand(MachineOperand::IMM, imm);
}

MemsetInstruction::MemsetInstruction(ValueRef *start, int len, int byte): IRInstruction(MEMSET) {
    this->start = start;
    this->len = len;
    this->byte = byte;
}

void MemsetInstruction::outPut(ostream &os) {
    //  %24 = bitcast [2 x [2 x [2 x [2 x [2 x [2 x [2 x [2 x [2 x [2 x [2 x [2 x [2 x [2 x [2 x [2 x [2 x [2 x [2 x i32]]]]]]]]]]]]]]]]]]]* %2 to i8*
    //  call void @llvm.memset.p0i8.i64(i8* align 16 %24, i8 0, i64 2097152, i1 false)

    os << start->get_Ref() + "_memset = bitcast " + start->get_TypeName() + "* " + start->get_Ref() + " to i8*" << endl;
    os << "  call void @llvm.memset.p0i8.i64(i8* align 16 " + start->get_Ref() + "_memset, i8 " + to_string(byte) + ", i64 " + to_string(len) + ", i1 false)" << endl;
}

void MemsetInstruction::replace(ValueRef *old, ValueRef *now) {
    cerr << "this instr should not be replaced!";
    throw exception();
}

MoveInstruction::MoveInstruction(ValueRef *dst, ValueRef *src): IRInstruction(MOVE) {
    this->dst = dst;
    this->src = src;
}

void MoveInstruction::outPut(ostream &os) {
    cout << dst->get_Ref() << " <- " << src->get_Ref() << endl;
}

void MoveInstruction::replace(ValueRef *old, ValueRef *now) {
    cerr << "It should never be replaced" << endl;
    throw exception();
}

void
MoveInstruction::codegen(AsmBuilder *pBuilder, map<std::string, int> &offset_table, map<std::string, int> &size_table,
                         int frameSize) {
    if (this->dst->type == IntVar) {
        if (this->src->type == IntConst) {
            loadImmIntoReg(pBuilder, strtol(this->src->get_Ref().c_str(), nullptr, 10));
            auto mv = new MoveMInstruction(pBuilder->getBlock(), MoveMInstruction::MV,
                                           new MachineOperand(MachineOperand::VREG, this->dst->get_Ref()),
                                           new MachineOperand(MachineOperand::VREG, getLastVRegister()));
            pBuilder->getBlock()->insertInst(mv);
        } else {
            auto mv = new MoveMInstruction(pBuilder->getBlock(), MoveMInstruction::MV,
                                           new MachineOperand(MachineOperand::VREG, this->dst->get_Ref()),
                                           new MachineOperand(MachineOperand::VREG, this->src->get_Ref()));
            pBuilder->getBlock()->insertInst(mv);
        }
    } else if (this->dst->type == FloatVar) {
        if (this->src->type == FloatConst) {
            isFloatConst(this->src);
            LoadLabel(pBuilder, getVRegister(), getLC());
            LoadFloat(pBuilder, getFVRegister(), getLastVRegister());
//                auto la = new LoadImmInstruction(pBuilder->getBlock(), LoadImmInstruction::LA,
//                                                 new MachineOperand(MachineOperand::VREG, getVRegister()),
//                                                 new MachineOperand(MachineOperand::LABEL, getLC()));
//                pBuilder->getBlock()->insertInst(la);
//                auto flw = new LoadMInstruction(pBuilder->getBlock(),
//                                                LoadMInstruction::FLW,
//                                                new MachineOperand(MachineOperand::FVREG, getFVRegister()),
//                                                new MachineOperand(MachineOperand::IMM, 0),
//                                                new MachineOperand(MachineOperand::VREG, getLastVRegister()));
//                pBuilder->getBlock()->insertInst(flw);
            auto fmv = new MoveMInstruction(pBuilder->getBlock(), MoveMInstruction::FMV,
                                            new MachineOperand(MachineOperand::FVREG, this->dst->get_Ref()),
                                            new MachineOperand(MachineOperand::FVREG, getLastFVRegister()));
            pBuilder->getBlock()->insertInst(fmv);
        } else {
            auto fmv = new MoveMInstruction(pBuilder->getBlock(), MoveMInstruction::FMV,
                                            new MachineOperand(MachineOperand::FVREG, this->dst->get_Ref()),
                                            new MachineOperand(MachineOperand::FVREG, this->src->get_Ref()));
            pBuilder->getBlock()->insertInst(fmv);
        }
    }
}
void MemsetInstruction::codegen(AsmBuilder *pBuilder, map<std::string, int> &offset_table, map<std::string, int> &size_table,
                                int frameSize) {
    auto offsetIter = offset_table.find(this->start->get_Ref());
    int start_offset = -16-size_table[this->start->get_Ref()] - offsetIter->second;
    if (abs(start_offset) < 2040) {
        auto bin = new BinaryMInstruction(pBuilder->getBlock(),
                                          BinaryMInstruction::ADDI,
                                          new MachineOperand(MachineOperand::VREG, this->start->get_Ref()),
                                          new MachineOperand(MachineOperand::REG, IREGISTER::FP),
                                          new MachineOperand(MachineOperand::IMM,
                                                  // previously `fp - 16 - 4 - offset`, now `fp - 16 - size of array - offset`
                                                             -16 - size_table[this->start->get_Ref()] -offsetIter->second));
        pBuilder->getBlock()->insertInst(bin);
    } else {
        loadImmIntoReg(pBuilder, start_offset);
        auto bin = new BinaryMInstruction(pBuilder->getBlock(), BinaryMInstruction::ADD,
                                          new MachineOperand(MachineOperand::VREG, this->start->get_Ref()),
                                          new MachineOperand(MachineOperand::REG, IREGISTER::FP),
                                          new MachineOperand(MachineOperand::VREG, getLastVRegister()));
        pBuilder->getBlock()->insertInst(bin);
    }

    auto adda0 = new BinaryMInstruction(pBuilder->getBlock(),BinaryMInstruction::ADD,
                                        new MachineOperand(MachineOperand::REG, A0),
                                        new MachineOperand(MachineOperand::REG, ZERO),
                                        new MachineOperand(MachineOperand::VREG, this->start->get_Ref()));
    pBuilder->getBlock()->insertInst(adda0);
    auto lia1 = new LoadImmInstruction(pBuilder->getBlock(),LoadImmInstruction::LI,
                                       new MachineOperand(MachineOperand::REG, A1),
                                       new MachineOperand(MachineOperand::IMM, this->byte));
    pBuilder->getBlock()->insertInst(lia1);
    auto lia2 = new LoadImmInstruction(pBuilder->getBlock(), LoadImmInstruction::LI,
                                       new MachineOperand(MachineOperand::REG, A2),
                                       new MachineOperand(MachineOperand::IMM, this->len));
    pBuilder->getBlock()->insertInst(lia2);
    auto jal = new JalMInstruction(pBuilder->getBlock(),
                                   JalMInstruction::JAL,
                                   new MachineOperand(MachineOperand::REG, RA),
                                   new MachineOperand(MachineOperand::LABEL, "memset"));
    pBuilder->getBlock()->insertInst(jal);
    // auto call = new CallFuncMInstruction(pBuilder->getBlock(),
    //                                      CallFuncMInstruction::CALL,
    //                                new MachineOperand(MachineOperand::LABEL, "memset"));
    // pBuilder->getBlock()->insertInst(call);
}

SllInstruction::SllInstruction(ValueRef *dst, ValueRef *src1, ValueRef *bits): IRInstruction(SLL) {
    this->dst = dst; this->src1 = src1; this->bits = bits;
    addDef(dst);
    addUse(src1);
    addUse(bits);
}

void SllInstruction::outPut(ostream &os) {
    //%5 = shl i32 %4, 1
    os << dst->get_Ref() << " = shl i32 " << src1->get_Ref() << ", " << bits->get_Ref() << endl;
}

void SllInstruction::replace(ValueRef *old, ValueRef *now) {
    if(this->src1 == old) this->src1 = now;
    if(this->bits == old) this->bits = now;
    if(this->use_list.count(old)) this->use_list.erase(old);
    this->use_list.insert(now);

    now->use.push_back(this);
}

void
SllInstruction::codegen(AsmBuilder *pBuilder, map<std::string, int> &offset_table, map<std::string, int> &size_table,
                        int frameSize) {
    auto sli = new BinaryMInstruction(pBuilder->getBlock(),BinaryMInstruction::slliw,
                                          new MachineOperand(MachineOperand::VREG, this->dst->get_Ref()),
                                          new MachineOperand(MachineOperand::VREG, this->src1->get_Ref()),
                                          new MachineOperand(MachineOperand::IMM, strtol(this->bits->get_Ref().c_str(), nullptr, 10)));
    pBuilder->getBlock()->insertInst(sli);
}

SraInstruction::SraInstruction(ValueRef *dst, ValueRef *src1, ValueRef *bits): IRInstruction(SRA) {
    this->dst = dst; this->src1 = src1; this->bits = bits;
    addDef(dst);
    addUse(src1);
    addUse(bits);
}

void SraInstruction::outPut(ostream &os) {
    os << dst->get_Ref() << " = ashr i32 " << src1->get_Ref() << ", " << bits->get_Ref() << endl;
}

void SraInstruction::replace(ValueRef *old, ValueRef *now) {
    if(this->src1 == old) this->src1 = now;
    if(this->bits == old) this->bits = now;
    if(this->use_list.count(old)) this->use_list.erase(old);
    this->use_list.insert(now);

    now->use.push_back(this);
}

void SraInstruction::codegen(AsmBuilder *pBuilder, map<std::string, int> &offset_table, map<std::string, int> &size_table,
                        int frameSize) {
        auto sai = new BinaryMInstruction(pBuilder->getBlock(),BinaryMInstruction::srliw,
                                          new MachineOperand(MachineOperand::VREG, this->dst->get_Ref()),
                                          new MachineOperand(MachineOperand::VREG, this->src1->get_Ref()),
                                          new MachineOperand(MachineOperand::IMM, strtol(this->bits->get_Ref().c_str(), nullptr, 10)));
        pBuilder->getBlock()->insertInst(sai);
}
