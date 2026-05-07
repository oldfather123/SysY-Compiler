
#ifndef COMPILER_IRInstruction_H
#define COMPILER_IRInstruction_H

#include <exception>
#include "BasicBlock.h"
#include "ValueRef.h"
#include "Function.h"
#include "Instruction.h"
#include "codegen/AsmBuilder.hpp"
#include "codegen/MachineInstruction.hpp"
#include "codegen/MachineOperand.hpp"

using namespace std;

enum binaryType {ADD, SUB, MUL, DIV, MOD, AND, OR};
enum cmpType {EQ, NE, LT, LE, GT, GE};


class IRInstruction : public Instruction{
public:

    BasicBlock* block; // nullptr表示全局指令
    static int vRegCount;
    static int vfRegCount;
    static int labelCount; // floatconst
    static std::map<std::string, double> float_table; //name

    explicit IRInstruction(InstType_Enum instType);

    void codegen(AsmBuilder *pBuilder,
                 std::map<std::string, int> &offset_table,
                 std::map<std::string, int> &size_table,
                 int frameSize) override {}

    static void setVRegisterCount(int n){
        vRegCount = n;
    }

    static std::string getLastVRegister(int pre = 1){
        return "VREG" + std::to_string(vRegCount - pre);
    }

    static std::string getVRegister(){
        return "VREG" + std::to_string(vRegCount++);
    }

    static std::string getLastFVRegister(int pre = 1){
        return "FVREG" + std::to_string(vfRegCount - pre);
    }

    static std::string getFVRegister(){
        return "FVREG" + std::to_string(vfRegCount++);
    }

    static std::string getLC(int pre = 1){
        return ".LC" + std::to_string( labelCount - pre);
    }

    static std::string generateLC(){
        return ".LC" + std::to_string( labelCount++);
    }

    static void retFunc(AsmBuilder *pBuilder, int offset){
        auto cur_block = pBuilder->getBlock();
        int framesize = offset + 16;
        auto addi = new BinaryMInstruction(cur_block,
                                           BinaryMInstruction::ADDI,
                                           new MachineOperand(MachineOperand::REG, IREGISTER::SP),
                                           new MachineOperand(MachineOperand::REG, IREGISTER::SP),
                                           new MachineOperand(MachineOperand::IMM, offset+16));
        cur_block->insertInst(addi);
        auto ld1 = new LoadMInstruction(cur_block,
                                        LoadMInstruction::LD,
                                        new MachineOperand(MachineOperand::REG, IREGISTER::RA),
                                        new MachineOperand(MachineOperand::IMM, -8), // framesize - 8
                                        new MachineOperand(MachineOperand::REG, IREGISTER::SP));
        cur_block->insertInst(ld1);
        auto ld2 = new LoadMInstruction(cur_block,
                                        LoadMInstruction::LD,
                                        new MachineOperand(MachineOperand::REG, IREGISTER::FP),
                                        new MachineOperand(MachineOperand::IMM, -16), // framesize
                                        new MachineOperand(MachineOperand::REG, IREGISTER::SP));
        cur_block->insertInst(ld2);
        auto jr = new JrMInstruction(cur_block,
                                     JrMInstruction::JR,
                                     new MachineOperand(MachineOperand::REG, IREGISTER::RA));
        cur_block->insertInst(jr);
    }

    static void loadImmIntoReg(AsmBuilder *pBuilder, long value){
        auto cur_block = pBuilder->getBlock();
        auto liImm = new LoadImmInstruction(cur_block,
                                            LoadImmInstruction::LI,
                                            new MachineOperand(MachineOperand::VREG, getVRegister()),
                                            new MachineOperand(MachineOperand::IMM, value));
        pBuilder->getBlock()->insertInst(liImm);
    }

//    将布尔值取反
    static void xoriReg(AsmBuilder *pBuilder, MachineOperand* t){
        auto cur_block = pBuilder->getBlock();
        auto xori = new BinaryMInstruction(cur_block,
                                           BinaryMInstruction::XORI,
                                           t,
                                           new MachineOperand(*t),
                                           new MachineOperand(MachineOperand::IMM, 1));
        pBuilder->getBlock()->insertInst(xori);
    }

//    binary
    static void BinaryInstr(AsmBuilder *pBuilder, BinaryMInstruction::OpType op, string dst, string src1, string src2, bool isfloat = false){
        auto cur_block = pBuilder->getBlock();
        auto bin = new BinaryMInstruction(cur_block,
                                          op,
                                          new MachineOperand(MachineOperand::VREG, dst),
                                          new MachineOperand(MachineOperand::VREG, src1),
                                          new MachineOperand(MachineOperand::VREG, src2));
        pBuilder->getBlock()->insertInst(bin);
    }

//    add register and imm
    static void Addiw(AsmBuilder *pBuilder, BinaryMInstruction::OpType op, string dst, string src1, int imm){
        auto cur_block = pBuilder->getBlock();
        if(op != BinaryMInstruction::ADDI){
            throw new NotImplemented("unexpected optype in function Addiw");
        }
        else{
            if(abs(imm) > 2000){
                loadImmIntoReg(pBuilder, imm);
                auto bin = new BinaryMInstruction(cur_block,
                                                  BinaryMInstruction::ADD,
                                                  new MachineOperand(MachineOperand::VREG, dst),
                                                  new MachineOperand(MachineOperand::VREG, src1),
                                                  new MachineOperand(MachineOperand::VREG, getLastVRegister()));
                pBuilder->getBlock()->insertInst(bin);
            }else{
                auto bin = new BinaryMInstruction(cur_block,
                                                  op,
                                                  new MachineOperand(MachineOperand::VREG, dst),
                                                  new MachineOperand(MachineOperand::VREG, src1),
                                                  new MachineOperand(MachineOperand::IMM, imm));
                pBuilder->getBlock()->insertInst(bin);
            }
        }
    }

//    f bin
    static void fBinaryInstr(AsmBuilder *pBuilder, BinaryMInstruction::OpType op, string dst, string src1, string src2){
        auto cur_block = pBuilder->getBlock();
        auto fbin = new BinaryMInstruction(cur_block,
                                          op,
                                          new MachineOperand(MachineOperand::FVREG, dst),
                                          new MachineOperand(MachineOperand::FVREG, src1),
                                          new MachineOperand(MachineOperand::FVREG, src2));
        pBuilder->getBlock()->insertInst(fbin);
    }

//    jump label
    static void JumpLabel(AsmBuilder *pBuilder, const string& label){
        auto br = new JumpMInstruction(pBuilder->getBlock(),
                                       JumpMInstruction::J,
                                       new MachineOperand(MachineOperand::LABEL, label));
        pBuilder->getBlock()->insertInst(br);
    }

//    load label
    static void LoadLabel(AsmBuilder *pBuilder, const string &vreg, const string& label){
        auto la = new LoadImmInstruction(pBuilder->getBlock(),
                                         LoadImmInstruction::LA,
                                         new MachineOperand(MachineOperand::VREG, vreg),
                                         new MachineOperand(MachineOperand::LABEL, label));
        pBuilder->getBlock()->insertInst(la);
    }

//    load float
    static void LoadFloat(AsmBuilder *pBuilder, const string &fvreg, const string& vreg){
        auto flw = new LoadMInstruction(pBuilder->getBlock(),
                                        LoadMInstruction::FLW,
                                        new MachineOperand(MachineOperand::FVREG, fvreg),
                                        new MachineOperand(MachineOperand::IMM, 0),
                                        new MachineOperand(MachineOperand::VREG, vreg));
        pBuilder->getBlock()->insertInst(flw);
    }

//    static float cmp
    static void FCmp(AsmBuilder *pBuilder, CmpMInstruction::OpType op, const string &result, const string& fvreg1, const string& fvreg2){
        auto fcmp = new CmpMInstruction(pBuilder->getBlock(),
                                       op,
                                       new MachineOperand(MachineOperand::VREG, result),
                                       new MachineOperand(MachineOperand::FVREG, fvreg1),
                                       new MachineOperand(MachineOperand::FVREG, fvreg2));
        pBuilder->getBlock()->insertInst(fcmp);
    }

    static void loadFromFP(AsmBuilder *pBuilder,LoadMInstruction::OpType optype, string dst, int imm){
        if (abs(imm) < 2000) {
            auto lw1 = new LoadMInstruction(pBuilder->getBlock(),
                                        optype,
                                        new MachineOperand(MachineOperand::VREG, dst),
                                        new MachineOperand(MachineOperand::IMM, imm),
                                        new MachineOperand(MachineOperand::REG, IREGISTER::FP));
            pBuilder->getBlock()->insertInst(lw1);
        } else {
            loadImmIntoReg(pBuilder, imm);
            auto add = new BinaryMInstruction(pBuilder->getBlock(), BinaryMInstruction::ADD,
                                                new MachineOperand(MachineOperand::VREG, getLastVRegister()),
                                                new MachineOperand(MachineOperand::VREG, getLastVRegister()),
                                                new MachineOperand(MachineOperand::REG, IREGISTER::FP));
            pBuilder->getBlock()->insertInst(add);
            auto lw = new LoadMInstruction(pBuilder->getBlock(), optype,
                                                new MachineOperand(MachineOperand::VREG, dst),
                                                new MachineOperand(MachineOperand::IMM, 0),
                                                new MachineOperand(MachineOperand::VREG, getLastVRegister()));
            pBuilder->getBlock()->insertInst(lw);
        }
    }
    static void loadFloatFromFP(AsmBuilder *pBuilder,LoadMInstruction::OpType optype, string dst, int imm){
        if (abs(imm) < 2000) {
            auto lw1 = new LoadMInstruction(pBuilder->getBlock(),
                                        optype,
                                        new MachineOperand(MachineOperand::FVREG, dst),
                                        new MachineOperand(MachineOperand::IMM, imm),
                                        new MachineOperand(MachineOperand::REG, IREGISTER::FP));
            pBuilder->getBlock()->insertInst(lw1);
        } else {
            loadImmIntoReg(pBuilder, imm);
            auto add = new BinaryMInstruction(pBuilder->getBlock(), BinaryMInstruction::ADD,
                                                new MachineOperand(MachineOperand::VREG, getLastVRegister()),
                                                new MachineOperand(MachineOperand::VREG, getLastVRegister()),
                                                new MachineOperand(MachineOperand::REG, IREGISTER::FP));
            pBuilder->getBlock()->insertInst(add);
            auto lw = new LoadMInstruction(pBuilder->getBlock(), optype,
                                                new MachineOperand(MachineOperand::FVREG, dst),
                                                new MachineOperand(MachineOperand::IMM, 0),
                                                new MachineOperand(MachineOperand::VREG, getLastVRegister()));
            pBuilder->getBlock()->insertInst(lw);
        }
    }

    static void storeToStack(AsmBuilder *pBuilder, StoreMInstruction::OpType optype, std::string src, int imm) {
        if (abs(imm) < 2032) {
            auto store = new StoreMInstruction(pBuilder->getBlock(), optype,
                                                new MachineOperand(MachineOperand::VREG, src),
                                                new MachineOperand(MachineOperand::IMM, imm),
                                                new MachineOperand(MachineOperand::REG, IREGISTER::FP));
            pBuilder->getBlock()->insertInst(store);
        } else {
            loadImmIntoReg(pBuilder, imm);
            auto add = new BinaryMInstruction(pBuilder->getBlock(), BinaryMInstruction::ADD,
                                                new MachineOperand(MachineOperand::VREG, getLastVRegister()),
                                                new MachineOperand(MachineOperand::VREG, getLastVRegister()),
                                                new MachineOperand(MachineOperand::REG, IREGISTER::FP));
            pBuilder->getBlock()->insertInst(add);
            auto store = new StoreMInstruction(pBuilder->getBlock(), optype,
                                                new MachineOperand(MachineOperand::VREG, src),
                                                new MachineOperand(MachineOperand::IMM, 0),
                                                new MachineOperand(MachineOperand::VREG, getLastVRegister()));
            pBuilder->getBlock()->insertInst(store);
        }
    }
    
    static void storeFloatToStack(AsmBuilder *pBuilder, StoreMInstruction::OpType optype, std::string src, int imm) {
        if (abs(imm) < 2032) {
            auto store = new StoreMInstruction(pBuilder->getBlock(), optype,
                                                new MachineOperand(MachineOperand::FVREG, src),
                                                new MachineOperand(MachineOperand::IMM, imm),
                                                new MachineOperand(MachineOperand::REG, IREGISTER::FP));
            pBuilder->getBlock()->insertInst(store);
        } else {
            loadImmIntoReg(pBuilder, imm);
            auto add = new BinaryMInstruction(pBuilder->getBlock(), BinaryMInstruction::ADD,
                                                new MachineOperand(MachineOperand::VREG, getLastVRegister()),
                                                new MachineOperand(MachineOperand::VREG, getLastVRegister()),
                                                new MachineOperand(MachineOperand::REG, IREGISTER::FP));
            pBuilder->getBlock()->insertInst(add);
            auto store = new StoreMInstruction(pBuilder->getBlock(), optype,
                                                new MachineOperand(MachineOperand::FVREG, src),
                                                new MachineOperand(MachineOperand::IMM, 0),
                                                new MachineOperand(MachineOperand::VREG, getLastVRegister()));
            pBuilder->getBlock()->insertInst(store);
        }
    }
//    arr
    static void arrInitInt(std::vector<std::any> values, ArrayType* arrayType, std::stringstream &ss){
        int sizeTotal = arrayType->size;
        if(values.empty()){
            ss << "\t.zero\t" << sizeTotal << '\n';
            return;
        }
        if(values.at(0).type() != typeid(vector<any>)){
// one dimension
            for(auto & value : values){
                ss << "\t.word\t" << any_cast<ValueRef*>(value)->get_Ref() << endl;
                sizeTotal -= 4;
            }

            if(sizeTotal > 0)
                ss << "\t.zero\t" << sizeTotal << '\n';
        }
        else{
            int sum = 0;
            for(auto & value : values){
                sum += (dynamic_cast<ArrayType*>(arrayType->elementType))->size;
                arrInitInt(any_cast<vector<any>>(value), dynamic_cast<ArrayType*>(arrayType->elementType) ,ss);
            }
            if(sum < sizeTotal){
                ss << "\t.zero\t" << sizeTotal-sum << '\n';
            }
        }
    }

    static void arrInitFloat(std::vector<std::any> values, ArrayType* arrayType, std::stringstream &ss){
        int sizeTotal = arrayType->size;
        if(values.empty()){
            ss << "\t.zero\t" << sizeTotal << '\n';
            return;
        }
        if(values.at(0).type() != typeid(vector<any>)){
// one dimension
            for(auto & value : values){
//                TODO: warning : cast type from double to float
                float val = ((Float_Const*)any_cast<ValueRef*>(value))->value;
                unsigned long dec = float2dec(val);
                if(val < 0){
                    ss << "\t.word\t-" << dec << endl;
                }else{
                    ss << "\t.word\t" << dec << endl;
                }
                sizeTotal -= 4;
            }

            if(sizeTotal > 0)
                ss << "\t.zero\t" << sizeTotal << '\n';
        }
        else{
            for(auto & value : values){
                arrInitFloat(any_cast<vector<any>>(value), dynamic_cast<ArrayType*>(arrayType->elementType) ,ss);
            }
        }
    }

    //float
    static unsigned long float2dec(float f){
//        float f = stof(value);
        std::bitset<32> bits(*reinterpret_cast<unsigned int*>(&f));
        unsigned long decimal_value = static_cast<int>(bits.to_ulong());
        if (f < 0) {
            decimal_value = ~(decimal_value - 1);
        }
        return decimal_value;
    }

    //float
    static void isFloatConst(ValueRef* value){
        float_table[generateLC()] = ((Float_Const*)value)->value;
    }
};

class DummyInstruction : public IRInstruction {//NOP指令
public:

    DummyInstruction();

    //void outPut(std::ostream &os) override;
};

class AddGlobalInstruction : public IRInstruction {
public:
    ValueRef* dst;
    Type* varType;
    ValueRef* initVal;
    string debugInfo;

    AddGlobalInstruction(ValueRef* dst, Type* varType, ValueRef* initVal, string& debugInfo);

    void codegen(AsmBuilder *pBuilder) const;

    void outPut(std::ostream &os) override { os << debugInfo << endl; }

    void replace(ValueRef *old, ValueRef *now) override;
};

class AllocaInstruction : public IRInstruction {
public:
    ValueRef* dst;
    Type* varType;
    string debugInfo;

    AllocaInstruction(ValueRef* dst, Type* varType, string& debugInfo);

    void outPut(std::ostream &os) override { os << debugInfo << endl; }

    void replace(ValueRef *old, ValueRef *now) override;
};

class LoadInstruction : public IRInstruction {
public:
    ValueRef* dst;
    ValueRef* src;

    LoadInstruction(ValueRef* dst,ValueRef* src);//(ValueRef* dst, Pointer* src)?

    void codegen(AsmBuilder *pBuilder,
                 std::map<std::string, int> &offset_table,
                 std::map<std::string, int> &size_table,
                 int frameSize) override;
    void outPut(std::ostream &os) override;

    void replace(ValueRef *old, ValueRef *now) override;
};

class StoreInstruction : public IRInstruction {
public:
    ValueRef* dst;
    ValueRef* src;

    StoreInstruction(ValueRef* dst,ValueRef* src);

    void codegen(AsmBuilder *pBuilder,
                 std::map<std::string, int> &offset_table,
                 std::map<std::string, int> &size_table,
                 int frameSize) override;

    void outPut(std::ostream &os) override;

    void replace(ValueRef *old, ValueRef *now) override;
};
class BinaryInstruction : public IRInstruction {
public:

    ValueRef* dst;
    ValueRef* src1;
    ValueRef* src2;
    binaryType opTy;

    BinaryInstruction(ValueRef* dst,ValueRef* src1, ValueRef* src2, binaryType opTy);

    void codegen(AsmBuilder *pBuilder,
                 std::map<std::string, int> &offset_table,
                 std::map<std::string, int> &size_table,
                 int frameSize) override;

    void outPut(std::ostream &os) override;

    void replace(ValueRef *old, ValueRef *now) override;
};

class CmpInstruction : public IRInstruction {
public:
    ValueRef* result;
    ValueRef* src1;
    ValueRef* src2;
    cmpType opTy;

    CmpInstruction(ValueRef* result, ValueRef* src1, ValueRef* src2, cmpType opTy);

    void codegen(AsmBuilder *pBuilder,
                 std::map<std::string, int> &offset_table,
                 std::map<std::string, int> &size_table,
                 int frameSize) override;

    void outPut(std::ostream &os) override;

    void replace(ValueRef *old, ValueRef *now) override;
};

class BrInstruction : public IRInstruction {
public:
    BasicBlock* label;

    BrInstruction(BasicBlock* label);

    void codegen(AsmBuilder *pBuilder,
                 std::map<std::string, int> &offset_table,
                 std::map<std::string, int> &size_table,
                 int frameSize) override;

    void outPut(std::ostream &os) override;

    void replace(ValueRef *old, ValueRef *now) override;
};

class CondBrInstruction : public IRInstruction {
public:
    ValueRef* condition;
    BasicBlock* trueLabel;
    BasicBlock* falseLabel;

    CondBrInstruction(ValueRef* condition, BasicBlock* trueLabel, BasicBlock* falseLabel);

    void codegen(AsmBuilder *pBuilder,
                 std::map<std::string, int> &offset_table,
                 std::map<std::string, int> &size_table,
                 int frameSize) override;

    void outPut(std::ostream &os) override;

    void replace(ValueRef *old, ValueRef *now) override;
};

class RetInstruction : public IRInstruction {
public:
    ValueRef* retVal; // nullptr 表示 ret void;

    RetInstruction(ValueRef* retVal);

    void codegen(AsmBuilder *pBuilder,
                 std::map<std::string, int> &offset_table,
                 std::map<std::string, int> &size_table,
                 int frameSize) override;
    void outPut(std::ostream &os) override;

    void replace(ValueRef *old, ValueRef *now) override;
};

class CallInstruction : public IRInstruction {
public:
    ValueRef* retVal;
    Function* function;
    vector<ValueRef*> args;

    CallInstruction(ValueRef* retVal, Function* function, vector<ValueRef*>& args);

    void codegen(AsmBuilder *pBuilder,
                 std::map<std::string, int> &offset_table,
                 std::map<std::string, int> &size_table,
                 int frameSize) override;

    void outPut(std::ostream &os) override;

    void replace(ValueRef *old, ValueRef *now) override;
};

class ZExtInstruction : public IRInstruction {
public:
    ValueRef* dst;
    ValueRef* src;

    ZExtInstruction(ValueRef* dst,ValueRef* src);

    void codegen(AsmBuilder *pBuilder,
                 std::map<std::string, int> &offset_table,
                 std::map<std::string, int> &size_table,
                 int frameSize) override;

    void outPut(std::ostream &os) override;

    void replace(ValueRef *old, ValueRef *now) override;
};

class XorInstruction : public IRInstruction {
public:
    // 理论上只有Xor true的情况，所以另一个参数永远为1(i1/i32)
    ValueRef* dst;
    ValueRef* src;

    XorInstruction(ValueRef* dst, ValueRef* src);

    void codegen(AsmBuilder *pBuilder,
                 std::map<std::string, int> &offset_table,
                 std::map<std::string, int> &size_table,
                 int frameSize) override;


    void outPut(std::ostream &os) override;

    void replace(ValueRef *old, ValueRef *now) override;
};

class GEPInstruction : public IRInstruction {
public:
    ValueRef* dst;
    ValueRef* src;
    ValueRef* index; // 暂时做成只取一层下标的

    GEPInstruction(ValueRef* dst, ValueRef* src, ValueRef* index);

    void codegen(AsmBuilder *pBuilder,
                 std::map<std::string, int> &offset_table,
                 std::map<std::string, int> &size_table,
                 int frameSize) override;

    void outPut(std::ostream &os) override;

    void replace(ValueRef *old, ValueRef *now) override;
};

class PhiInstruction : public IRInstruction {
public:
    ValueRef* symbol; // you shouldn't use this
    ValueRef* result;
    int branch_cnt;
    map<BasicBlock*, ValueRef*> mp;

    PhiInstruction(ValueRef* symbol,ValueRef* result,int cnt);
    
    void codegen(AsmBuilder *pBuilder,
                 std::map<std::string, int> &offset_table,
                 std::map<std::string, int> &size_table,
                 int frameSize) override
    {
        throw exception();
    }

    void outPut(std::ostream &os) override;

    void replace(ValueRef *old, ValueRef *now) override;
};

class FPTIInstruction : public IRInstruction {
public:
    ValueRef* dst;
    ValueRef* src;

    FPTIInstruction(ValueRef* dst, ValueRef* src);

    void codegen(AsmBuilder *pBuilder,
                 std::map<std::string, int> &offset_table,
                 std::map<std::string, int> &size_table,
                 int frameSize) override;

    void outPut(std::ostream &os) override;

    void replace(ValueRef *old, ValueRef *now) override;
};

class ITFPInstruction : public IRInstruction {
public:
    ValueRef* dst;
    ValueRef* src;

    ITFPInstruction(ValueRef* dst, ValueRef* src);

    void codegen(AsmBuilder *pBuilder,
                 std::map<std::string, int> &offset_table,
                 std::map<std::string, int> &size_table,
                 int frameSize) override;

    void outPut(std::ostream &os) override;

    void replace(ValueRef *old, ValueRef *now) override;
};

class FNegInstruction : public IRInstruction {
public:
    ValueRef* dst;
    ValueRef* src;

    FNegInstruction(ValueRef* dst, ValueRef* src);

    void codegen(AsmBuilder *pBuilder,
                 std::map<std::string, int> &offset_table,
                 std::map<std::string, int> &size_table,
                 int frameSize) override;

    void outPut(std::ostream &os) override;

    void replace(ValueRef *old, ValueRef *now) override;
};

class MemsetInstruction : public IRInstruction{
public:
    ValueRef * start; // 数组首地址(但不保证是arr型)
    int len;
    int byte; // 要填充的内容,8bit

    MemsetInstruction(ValueRef * start, int len, int byte);

    void outPut(std::ostream &os) override;

    void codegen(AsmBuilder *pBuilder,
                 std::map<std::string, int> &offset_table,
                 std::map<std::string, int> &size_table,
                 int frameSize) override;

    void replace(ValueRef *old,ValueRef *now) override;
};

class MoveInstruction : public IRInstruction{
public:
    ValueRef* src;
    ValueRef* dst;

    MoveInstruction(ValueRef* dst, ValueRef* src);

    void codegen(AsmBuilder *pBuilder,
                 std::map<std::string, int> &offset_table,
                 std::map<std::string, int> &size_table,
                 int frameSize) override;

    void outPut(std::ostream &os) override;

    void replace(ValueRef *old,ValueRef *now) override;
};

class SllInstruction : public IRInstruction {
public:
    ValueRef* src1;
    ValueRef* bits; // 移动多少位
    ValueRef* dst;

    SllInstruction(ValueRef* dst, ValueRef* src1, ValueRef* bits);

    void codegen(AsmBuilder *pBuilder,
                 std::map<std::string, int> &offset_table,
                 std::map<std::string, int> &size_table,
                 int frameSize) override;

    void outPut(std::ostream &os) override;

    void replace(ValueRef *old,ValueRef *now) override;
};

class SraInstruction : public IRInstruction {
public:
    ValueRef* src1;
    ValueRef* bits; // 移动多少位
    ValueRef* dst;

    SraInstruction(ValueRef* dst, ValueRef* src1, ValueRef* bits);

    void codegen(AsmBuilder *pBuilder,
                 std::map<std::string, int> &offset_table,
                 std::map<std::string, int> &size_table,
                 int frameSize) override;

    void outPut(std::ostream &os) override;

    void replace(ValueRef *old,ValueRef *now) override;
};
#endif //COMPILER_IRInstruction_H

