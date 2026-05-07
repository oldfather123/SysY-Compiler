#pragma once
#include <cassert>
#include "Variable.h"
#include "Function.h"
#include "Label.h"

struct Function;
struct BasicBlock;

enum InsID
{
    Return,
    Br,
    Alloca,
    Store,
    Load,
    Call,
    Bitcast,
    Ext,
    Sitofp,
    Fptosi,
    GEP,
    Binary,
    Fneg,
    Icmp,
    Fcmp,
    Phi
};

struct Instruction
{
    InsID type;
    shared_ptr<BasicBlock> basicblock;

    Instruction(InsID type, shared_ptr<BasicBlock> bb): type{type}, basicblock{bb} {}
    virtual void print() = 0;

    virtual bool replaceValue(ValuePtr target, ValuePtr newValue) = 0;

    void deleteSelfInBB();
};
typedef shared_ptr<Instruction> InstructionPtr;

struct ReturnInstruction : Instruction
{
    ValuePtr retValue;
    ReturnInstruction(ValuePtr retValue, shared_ptr<BasicBlock> bb) : Instruction{InsID::Return, bb}, retValue{retValue} {
        newUse(retValue.get(), this);
    }; // to-do: 改为 exp

    virtual void print() override;
    virtual bool replaceValue(ValuePtr target, ValuePtr newValue) override;
};

struct AllocaInstruction : Instruction
{
    ValuePtr des;
    AllocaInstruction(ValuePtr des, shared_ptr<BasicBlock> bb) : Instruction{InsID::Alloca, bb}, des{des} {
        des->I = this;
    };

    virtual void print() override;
    virtual bool replaceValue(ValuePtr target, ValuePtr newValue) override;
};

struct GetElementPtrInstruction : Instruction
{
    static int arrayIdxNum;
    static int arrayElementNum;
    ValuePtr reg;
    static ValuePtr getArrayIdxReg(TypePtr type) { return ValuePtr(new Reg(type, "arrayidx" + to_string(arrayIdxNum++))); }
    static ValuePtr getArrayElementReg(TypePtr type) { return ValuePtr(new Reg(type, "arrayinit.element" + to_string(arrayElementNum++))); }
    ValuePtr from;
    vector<ValuePtr> index;
    GetElementPtrInstruction(ValuePtr from, vector<ValuePtr> index, shared_ptr<BasicBlock> bb) : Instruction{InsID::GEP, bb}, from{from}, index{index}
    {
        if (index.size() == 1)
        {
            reg = getArrayElementReg(from->type);
            newUse(from.get(),this);
            newUse(index[0].get(),this); // bug
        }
        else
        {
            // std::cerr<<index.size()<<"\n";
            auto curr = from->type;
            for (int i = 1; i < index.size(); i++)
            {
                assert(curr->isArr());
                // std::cerr<<dynamic_cast<ArrType *>(curr.get())->getStr()<<"\n";
                curr = dynamic_cast<ArrType *>(curr.get())->inner;
                
                newUse(index[i].get(),this);
                // }else if(from->type->isPtr()){
                //     reg = getArrayIdxReg(dynamic_cast<PtrType*>(from->type.get())->inner);
                // else
                // {
                //     std::cerr << from->getTypeStr() << " " << from->type->isPtr() << endl;
                //     reg = getArrayElementReg(from->type);
                // }
            }
            reg = getArrayIdxReg(curr);
            newUse(from.get(),this);
            newUse(index[0].get(),this); // bug
        }
        reg->I = this;
    }
    virtual void print() override;
    virtual bool replaceValue(ValuePtr target, ValuePtr newValue) override;
};

struct StoreInstruction : Instruction
{
    ValuePtr des;
    ValuePtr value;
    shared_ptr<GetElementPtrInstruction> gep;
    
    StoreInstruction(shared_ptr<GetElementPtrInstruction> gep, ValuePtr value, shared_ptr<BasicBlock> bb) : Instruction{InsID::Store, bb}, gep{gep}, des{gep->reg}, value{value} {
        newUse(gep->reg.get(),this);
        newUse(gep->from.get(),this);
        newUse(value.get(),this);
    };
    
    StoreInstruction(ValuePtr des, ValuePtr value, shared_ptr<BasicBlock> bb) : Instruction{InsID::Store, bb}, des{des}, value{value} {
        newUse(des.get(),this);
        newUse(value.get(),this);
    };
    virtual void print() override;
    virtual bool replaceValue(ValuePtr target, ValuePtr newValue) override;
};

struct LoadInstruction : Instruction
{
    ValuePtr from;
    ValuePtr to;
    shared_ptr<GetElementPtrInstruction> gep;
    LoadInstruction(ValuePtr from, ValuePtr to, shared_ptr<BasicBlock> bb) : Instruction{InsID::Load, bb}, from{from}, to{to} {
        newUse(from.get(),this);
        //to即这条指令本身
        to->I = this;
    };

    LoadInstruction(shared_ptr<GetElementPtrInstruction> gep, ValuePtr to, shared_ptr<BasicBlock> bb) : Instruction{InsID::Load, bb}, gep{gep}, from{gep->reg}, to{to} {
        newUse(gep->reg.get(),this);
        newUse(gep->from.get(),this);
    };
    virtual void print() override;
    virtual bool replaceValue(ValuePtr target, ValuePtr newValue) override;
};

struct BitCastInstruction : Instruction
{
    ValuePtr reg;
    ValuePtr from;
    TypePtr toType;
    BitCastInstruction(ValuePtr from, ValuePtr reg, shared_ptr<BasicBlock> bb, TypePtr toType = TypePtr(new PtrType(Type::getInt8()))) : Instruction{InsID::Bitcast, bb}, from{from}, toType{toType}, reg{reg} {
        newUse(from.get(),this);
        //reg即这条指令本身
        reg->I = this;
    }
    virtual void print() override;
    virtual bool replaceValue(ValuePtr target, ValuePtr newValue) override;
};

struct ExtInstruction : Instruction
{
    static int extNum;
    static ValuePtr getExtReg(TypePtr type) { return ValuePtr(new Reg(type, "ext" + to_string(extNum++))); }

    ValuePtr from;
    TypePtr to;
    ValuePtr reg;
    bool isign;
    ExtInstruction(ValuePtr from, TypePtr to, bool isign, shared_ptr<BasicBlock> bb) : Instruction{InsID::Ext, bb}, from{from}, to{to}, reg{getExtReg(to)}, isign{isign} {
        newUse(from.get(),this);
        //reg即这条指令本身
        reg->I = this;
    }
    virtual void print() override;
    virtual bool replaceValue(ValuePtr target, ValuePtr newValue) override;
};

struct SitofpInstruction : Instruction
{
    static int convNum;
    static ValuePtr getConvReg(TypePtr type) { return ValuePtr(new Reg(type, "conv" + to_string(convNum++))); }
    ValuePtr from;
    ValuePtr reg;
    SitofpInstruction(ValuePtr from, shared_ptr<BasicBlock> bb) : Instruction{InsID::Sitofp, bb}, from{from}, reg{getConvReg(Type::getFloat())} {
        newUse(from.get(),this);
        //reg即这条指令本身
        reg->I = this;
    }
    virtual void print() override;
    virtual bool replaceValue(ValuePtr target, ValuePtr newValue) override;

    friend struct FptosiInstruction;
};

struct FptosiInstruction : Instruction
{
    ValuePtr from;
    ValuePtr reg;
    FptosiInstruction(ValuePtr from, shared_ptr<BasicBlock> bb) : Instruction{InsID::Fptosi, bb}, from{from}, reg{SitofpInstruction::getConvReg(Type::getInt())} {
        newUse(from.get(),this);
        //reg即这条指令本身
        reg->I = this;
    }
    virtual void print() override;
    virtual bool replaceValue(ValuePtr target, ValuePtr newValue) override;
};

struct CallInstruction : Instruction
{
    static int callRegNum;
    ValuePtr reg;
    static ValuePtr getCallReg(TypePtr type) { return ValuePtr(new Reg(type, "call" + to_string(callRegNum++))); }
    shared_ptr<Function> func;
    vector<ValuePtr> argv;
    CallInstruction(shared_ptr<Function> func, vector<ValuePtr> argv, shared_ptr<BasicBlock> bb);
    virtual void print() override;
    virtual bool replaceValue(ValuePtr target, ValuePtr newValue) override;
};

struct BinaryInstruction : Instruction
{
    static int BinaryRegNum;
    static ValuePtr getBinaryReg(TypePtr type) { return ValuePtr(new Reg(type, "binary" + to_string(BinaryRegNum++))); }
    ValuePtr reg;
    ValuePtr a;
    ValuePtr b;
    char op;
    BinaryInstruction(ValuePtr a, ValuePtr b, char op, shared_ptr<BasicBlock> bb) : Instruction{InsID::Binary, bb}, a{a}, b{b}, op{op}, reg{getBinaryReg(a->type)}
    {
        newUse(a.get(),this);
        newUse(b.get(),this);
        reg->I = this;
        if (op == '!')
            reg->type = Type::getBool();
    };
    virtual void print() override;
    virtual bool replaceValue(ValuePtr target, ValuePtr newValue) override;
};

struct FnegInstruction : Instruction
{
    static int FnegRegNum;
    static ValuePtr getFnegReg() { return ValuePtr(new Reg(Type::getFloat(), "fneg" + to_string(FnegRegNum++))); }
    ValuePtr reg;
    ValuePtr a;
    FnegInstruction(ValuePtr a, shared_ptr<BasicBlock> bb) : Instruction{InsID::Fneg, bb}, a{a}, reg{getFnegReg()} {
        newUse(a.get(),this);
        reg->I = this;
    }
    virtual void print() override;
    virtual bool replaceValue(ValuePtr target, ValuePtr newValue) override;
};

struct IcmpInstruction : Instruction
{
    static int cmpRegNum; // 这里本来应该是 block 来处理，我看 llvm ir 不会报错就偷懒没改了
    static ValuePtr getCmpReg() { return ValuePtr(new Reg(Type::getBool(), "cmp" + to_string(cmpRegNum++))); }
    ValuePtr reg;
    ValuePtr a;
    ValuePtr b;
    string op;
    IcmpInstruction(shared_ptr<BasicBlock> bb, ValuePtr a, ValuePtr b = ValuePtr(new Const(Type::getInt(), 0)), string op = "!=") : Instruction{InsID::Icmp, bb}, a{a}, b{b}, op{op}, reg{getCmpReg()} {
        newUse(a.get(),this);
        newUse(b.get(),this);
        reg->I = this;
    };
    virtual void print() override;
    virtual bool replaceValue(ValuePtr target, ValuePtr newValue) override;

    friend struct FcmpInstruction;
};

struct FcmpInstruction : Instruction
{
    ValuePtr reg;
    ValuePtr a;
    ValuePtr b;
    string op;
    FcmpInstruction(shared_ptr<BasicBlock> bb, ValuePtr a, ValuePtr b = ValuePtr(new Const(Type::getFloat(), (float)0)), string op = "!=") : Instruction{InsID::Fcmp,bb}, a{a}, b{b}, op{op}, reg{IcmpInstruction::getCmpReg()} {
        newUse(a.get(),this);
        newUse(b.get(),this);
        reg->I = this;
    };
    virtual void print() override;
    virtual bool replaceValue(ValuePtr target, ValuePtr newValue) override;
};

struct BrInstruction : Instruction
{
    static int ifThenNum;
    static int ifEndNum;
    static int ifElseNum;
    static int orNum;
    static int andNum;
    static int whileCondNum;
    static int whileBodyNum;
    static int whileEndNum;
    static string getifThenStr() { return "if.then" + to_string(ifThenNum++); }
    static string getifEndStr() { return "if.end" + to_string(ifEndNum++); }
    static string getifElseStr() { return "if.else" + to_string(ifElseNum++); }
    static string getOrStr() { return "lor.lhs.false" + to_string(orNum++); }
    static string getAndStr() { return "land.lhs.true" + to_string(andNum++); }
    static string getWhileCondStr() { return "while.cond" + to_string(whileCondNum++); }
    static string getwhileBodyStr() { return "while.body" + to_string(whileBodyNum++); }
    static string getwhileEndStr() { return "while.end" + to_string(whileEndNum++); }
    ValuePtr exp;
    LabelPtr label1;
    LabelPtr label2;
    BrInstruction(ValuePtr exp, LabelPtr label1, LabelPtr label2, shared_ptr<BasicBlock> bb) : Instruction{InsID::Br, bb}, exp{exp}, label1{label1}, label2{label2} {
        newUse(exp.get(),this);
    }
    BrInstruction(LabelPtr label, shared_ptr<BasicBlock> bb) : Instruction{InsID::Br, bb}, label1{label} {}
    virtual void print() override;
    virtual bool replaceValue(ValuePtr target, ValuePtr newValue) override;
};

struct PhiInstruction : Instruction
{
    static int phiRegNum; 
    static ValuePtr getPhiReg(TypePtr type) { return ValuePtr(new Reg(type, "phi" + to_string(phiRegNum++))); }
    ValuePtr val;
    ValuePtr reg;
    vector<std::pair<ValuePtr,shared_ptr<BasicBlock>>> from;

    string op;
    PhiInstruction(shared_ptr<BasicBlock> bb, ValuePtr val) : Instruction{InsID::Phi,bb}, val{val}, reg{getPhiReg(val->type)} {
        newUse(val.get(), this);
        reg->I = this;
    };
    void addFrom(ValuePtr value, shared_ptr<BasicBlock> pred){
        from.push_back({value,pred});
        newUse(value.get(), this);
    }
    virtual void print() override;
    virtual bool replaceValue(ValuePtr target, ValuePtr newValue) override;
};


void replaceVarByVar(ValuePtr from, ValuePtr to);