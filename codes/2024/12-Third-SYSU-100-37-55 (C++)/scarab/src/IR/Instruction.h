#pragma once
#include <cassert>
#include <memory>
#include "Variable.h"
#include "Function.h"
#include "Label.h"
#include "utils.h"
#include <stdlib.h>
#include <map>
#include <iostream>

struct Function;
struct BasicBlock;
struct Instruction;
using std::enable_shared_from_this;

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
    Phi,
    Select,
    FuncCast,
    AtomicAdd
};

struct Instruction: public enable_shared_from_this<Instruction> {
    InsID type;
    shared_ptr<BasicBlock> basicblock;

    ValuePtr reg;

    Instruction(InsID type, shared_ptr<BasicBlock> bb, ValuePtr reg = nullptr): type{type}, basicblock{bb}, reg{reg} {}
    virtual ~Instruction(){}
    virtual void print() {}

    virtual bool replaceOperand(ValuePtr target, ValuePtr newValue) { return false; }
    virtual unsigned getOperandCount() { return 0; }
    virtual ValuePtr getOperand(unsigned i) { return nullptr; }

    bool isCommutative = false;
    bool isAssociative = false;
    bool isIdempotent = false;

    void moveBefore(shared_ptr<Instruction> targetIns, shared_ptr<Instruction> beforeIns);
    void insertBefore(shared_ptr<Instruction> targetIns, shared_ptr<Instruction> beforeIns);
    
    vector<shared_ptr<Instruction>>::iterator getIterator();

    void setRegisterName(string newName);
    string getRegisterName();
    void removeFromParent();
    shared_ptr<Instruction> getSharedThis() { return shared_from_this(); }
};
typedef shared_ptr<Instruction> InstructionPtr;

struct ReturnInstruction:public Instruction {
    ValuePtr retValue;
    ReturnInstruction(ValuePtr retValue, shared_ptr<BasicBlock> bb) : Instruction{InsID::Return, bb}, retValue{retValue} {
        newUse(retValue.get(), this);
    };
    ~ReturnInstruction();

    virtual void print() override;
    virtual bool replaceOperand(ValuePtr target, ValuePtr newValue) override;

    virtual unsigned getOperandCount() override;
    virtual ValuePtr getOperand(unsigned i) override;
};

struct AllocaInstruction:public Instruction {
    ValuePtr des;
    AllocaInstruction(ValuePtr des, shared_ptr<BasicBlock> bb) : Instruction{InsID::Alloca, bb}, des{des} {
        des->I = this;
    };
    ~AllocaInstruction();

    virtual void print() override;
    virtual bool replaceOperand(ValuePtr target, ValuePtr newValue) override;

    virtual unsigned getOperandCount() override;
    virtual ValuePtr getOperand(unsigned i) override;
};

struct GetElementPtrInstruction:public Instruction {
    static int arrayIdxNum;
    static int arrayElementNum;
    static ValuePtr getArrayIdxReg(TypePtr type) { return ValuePtr(new Reg(type, "arrayidx" + to_string(arrayIdxNum++))); }
    static ValuePtr getArrayElementReg(TypePtr type) { return ValuePtr(new Reg(type, "arrayinit.element" + to_string(arrayElementNum++))); }
    ValuePtr from;

    vector<ValuePtr> index;
    bool isConstIdx;
    vector<ValuePtr> allIndex;
    ValuePtr base;
    ~GetElementPtrInstruction();
    GetElementPtrInstruction(ValuePtr from, vector<ValuePtr> index, shared_ptr<BasicBlock> bb) : Instruction{InsID::GEP, bb}, from{from}, index{index}, isConstIdx{false} {
        if (index.size() == 1) {
            reg = getArrayElementReg(from->type);
            newUse(from.get(), this, reg.get());
            newUse(index[0].get(), this, reg.get());
        }
        else {
            auto curr = from->type;
            for (int i = 1; i < index.size(); i++) {
                assert(curr->isArr());
                curr = dynamic_cast<ArrType *>(curr.get())->inner;
                newUse(index[i].get(), this, reg.get());
            }
            reg = getArrayIdxReg(curr);
            newUse(from.get(), this, reg.get());
            newUse(index[0].get(), this, reg.get());
        }
        reg->I = this;
        clearAllIndex();
        calculateAllIndex();
    }
    virtual void print() override;
    virtual bool replaceOperand(ValuePtr target, ValuePtr newValue) override;
    virtual unsigned getOperandCount() override;
    virtual ValuePtr getOperand(unsigned i) override;

    void clearAllIndex();
    void calculateAllIndex();
};

struct StoreInstruction:public Instruction {
    ValuePtr des;
    ValuePtr value;
    shared_ptr<GetElementPtrInstruction> gep;
    ~StoreInstruction();
    StoreInstruction(shared_ptr<GetElementPtrInstruction> gep, ValuePtr value, shared_ptr<BasicBlock> bb) : Instruction{InsID::Store, bb}, gep{gep}, des{gep->reg}, value{value} {
        newUse(gep->reg.get(), this);
        newUse(value.get(), this);
    };
    
    StoreInstruction(ValuePtr des, ValuePtr value, shared_ptr<BasicBlock> bb) : Instruction{InsID::Store, bb}, des{des}, value{value} {
        newUse(des.get(), this);
        newUse(value.get(), this);
    };
    virtual void print() override;
    virtual bool replaceOperand(ValuePtr target, ValuePtr newValue) override;
    virtual unsigned getOperandCount() override;
    virtual ValuePtr getOperand(unsigned i) override;
};

struct LoadInstruction:public Instruction {
    ValuePtr from;
    ValuePtr to;
    shared_ptr<GetElementPtrInstruction> gep;
    LoadInstruction(ValuePtr from, ValuePtr to, shared_ptr<BasicBlock> bb) : Instruction{InsID::Load, bb}, from{from}, to{to} {
        newUse(from.get(), this, to.get());
        to->I = this;
        reg = to;
    };
    ~LoadInstruction();
    LoadInstruction(shared_ptr<GetElementPtrInstruction> gep, ValuePtr to, shared_ptr<BasicBlock> bb) : Instruction{InsID::Load, bb}, gep{gep}, from{gep->reg}, to{to} {
        newUse(gep->reg.get(), this, to.get());
        to->I = this;
    };
    virtual void print() override;
    virtual bool replaceOperand(ValuePtr target, ValuePtr newValue) override;
    virtual unsigned getOperandCount() override;
    virtual ValuePtr getOperand(unsigned i) override;
};

struct BitCastInstruction:public Instruction {
    ValuePtr from;
    TypePtr targetType;
    ~BitCastInstruction();
    BitCastInstruction(ValuePtr from, ValuePtr reg, shared_ptr<BasicBlock> bb, TypePtr targetType = TypePtr(new PtrType(Type::getInt8()))) : Instruction{InsID::Bitcast, bb, reg}, from{from}, targetType{targetType} {
        newUse(from.get(), this, reg.get());
        reg->I = this;
    }
    virtual void print() override;
    virtual bool replaceOperand(ValuePtr target, ValuePtr newValue) override;
    virtual unsigned getOperandCount() override;
    virtual ValuePtr getOperand(unsigned i) override;
};

struct ExtInstruction:public Instruction {
    static int extCount;
    static ValuePtr getExtReg(TypePtr type) { return ValuePtr(new Reg(type, "ext" + to_string(extCount++))); }
    ~ExtInstruction();
    ValuePtr from;
    TypePtr to;
    bool isSigned;
    ExtInstruction(ValuePtr from, TypePtr to, bool isSigned, shared_ptr<BasicBlock> bb) : Instruction{InsID::Ext, bb, getExtReg(to)}, from{from}, to{to}, isSigned{isSigned} {
        newUse(from.get(), this, reg.get());
        reg->I = this;
    }
    virtual void print() override;
    virtual bool replaceOperand(ValuePtr target, ValuePtr newValue) override;
    virtual unsigned getOperandCount() override;
    virtual ValuePtr getOperand(unsigned i) override;
};

struct SignToFloatInstruction:public Instruction {
    static int convNum;
    static ValuePtr getConvReg(TypePtr type) { return ValuePtr(new Reg(type, "conv" + to_string(convNum++))); }
    ValuePtr from;
    ~SignToFloatInstruction();
    SignToFloatInstruction(ValuePtr from, shared_ptr<BasicBlock> bb) : Instruction{InsID::Sitofp, bb, getConvReg(Type::getFloat())}, from{from} {
        newUse(from.get(), this, reg.get());
        reg->I = this;
    }
    virtual void print() override;
    virtual bool replaceOperand(ValuePtr target, ValuePtr newValue) override;
    virtual unsigned getOperandCount() override;
    virtual ValuePtr getOperand(unsigned i) override;

    friend struct FloatToSignInstruction;
};

struct FloatToSignInstruction:public Instruction {
    ValuePtr from;
    ~FloatToSignInstruction();
    FloatToSignInstruction(ValuePtr from, shared_ptr<BasicBlock> bb) : Instruction{InsID::Fptosi, bb, SignToFloatInstruction::getConvReg(Type::getInt())}, from{from} {
        newUse(from.get(), this, reg.get());
        reg->I = this;
    }
    virtual void print() override;
    virtual bool replaceOperand(ValuePtr target, ValuePtr newValue) override;
    virtual unsigned getOperandCount() override;
    virtual ValuePtr getOperand(unsigned i) override;
};

struct CallInstruction:public Instruction {
    static int callRegNum;
    static ValuePtr getCallReg(TypePtr type) { return ValuePtr(new Reg(type, "call" + to_string(callRegNum++))); }
    shared_ptr<Function> func;
    vector<ValuePtr> argv;
    ~CallInstruction();
    CallInstruction(shared_ptr<Function> func, vector<ValuePtr> argv, shared_ptr<BasicBlock> bb);
    virtual void print() override;
    virtual bool replaceOperand(ValuePtr target, ValuePtr newValue) override;
    virtual unsigned getOperandCount() override;
    virtual ValuePtr getOperand(unsigned i) override;
};

struct BinaryInstruction : public Instruction
{
    static int BinaryRegNum;
    static ValuePtr getBinaryReg(TypePtr type) { return ValuePtr(new Reg(type, "binary" + to_string(BinaryRegNum++))); }
    ValuePtr a;
    ValuePtr b;
    bool isrv64;
    char op;
    ~BinaryInstruction();
    BinaryInstruction(ValuePtr a, ValuePtr b, char op, shared_ptr<BasicBlock> bb) : Instruction{InsID::Binary, bb, getBinaryReg(a->type)}, a{a}, b{b}, isrv64{false}, op{op} {
        newUse(a.get(), this, reg.get());
        newUse(b.get(), this, reg.get());
        reg->I = this;
        if (op == '!')
            reg->type = Type::getBool();
        if(a->type->isInt()&&b->type->isInt()&&(op == '+'||op == '*'||op == '!')) {
            isCommutative = true;
            isAssociative = true;
        }
    };

    BinaryInstruction(ValuePtr a, ValuePtr b, char op, Instruction* I);

    void swapOperands(){
        std::swap(a,b);
    }

    void setOperands(unsigned index, ValuePtr newOp){
        assert(index<2&&"binaryInstruction only has two operands");
        assert(!reg->isConst&&!(dynamic_cast<Variable*>(newOp.get())&&dynamic_cast<Variable*>(newOp.get())->isGlobal)&&"cannot mutate a constant with setOperand");
        if(index == 0) {
            Use *use = a->useHead;
            while(use) {
                if(use->user == this) {
                    use->rmUse();
                    break;
                }
                use = use->next;
            }
            a = newOp;
            newUse(newOp.get(),this,reg.get());
        }
        else if(index == 1) {
            Use *use = b->useHead;
            while(use) {
                if(use->user == this) {
                    use->rmUse();
                    break;
                }
                use = use->next;
            }
            b = newOp;
            newUse(newOp.get(),this,reg.get());
        }
    }

    unsigned getOpcode(){
        if (a->type->isInt() || a->type->isBool()) {
            switch (op) {
                case '+':
                    return Add;
                    break;
                case '-':
                    return Sub;
                    break;
                case '*':
                    return Mul;
                    break;
                case '/':
                    return Div;
                    break;
                case '%':
                    return Rem;
                    break;
                case '!':
                    return Xor;
                    break;
                case ',':
                    return Shl;
                    break;
                case '.':
                    return AShr;
                    break;

                default:
                    assert(false && "Binary type errpr");
                    break;
            }
        }
        else if (a->type->isFloat()) {
            switch (op) {
                case '+':
                    return FAdd;
                    break;
                case '-':
                    return FSub;
                    break;
                case '*':
                    return FMul;
                    break;
                case '/':
                    return FDiv;
                    break;
                default:
                    assert(false && "Binary Float type error");
                    break;
            }
        }
        return 0;
    }

    virtual void print() override;
    virtual bool replaceOperand(ValuePtr target, ValuePtr newValue) override;
    virtual unsigned getOperandCount() override;
    virtual ValuePtr getOperand(unsigned i) override;
};

struct FnegInstruction:public Instruction {
    static int FnegRegNum;
    static ValuePtr getFnegReg() { return ValuePtr(new Reg(Type::getFloat(), "fneg" + to_string(FnegRegNum++))); }
    ValuePtr a;
    ~FnegInstruction();
    FnegInstruction(ValuePtr a, shared_ptr<BasicBlock> bb) : Instruction{InsID::Fneg, bb, getFnegReg()}, a{a} {
        newUse(a.get(), this, reg.get());
        reg->I = this;
    }
    virtual void print() override;
    virtual bool replaceOperand(ValuePtr target, ValuePtr newValue) override;
    virtual unsigned getOperandCount() override;
    virtual ValuePtr getOperand(unsigned i) override;
};

struct IcmpInstruction:public Instruction {
    static int cmpRegNum;
    static std::map<string, IcmpKind> kindMap;
    static ValuePtr getCmpReg() { return ValuePtr(new Reg(Type::getBool(), "cmp" + to_string(cmpRegNum++))); }
    ValuePtr a;
    ValuePtr b;
    string op;
    IcmpKind kind;

    void swapOperands(){
        ValuePtr temp;
        temp = a;
        a = b; 
        b = temp;
    }

    ~IcmpInstruction();
    IcmpInstruction(shared_ptr<BasicBlock> bb, ValuePtr a, ValuePtr b = Const::createConstant(Type::getInt(), 0)/*ValuePtr(new Const(Type::getInt(), 0))*/, string op = "!=") : Instruction{InsID::Icmp, bb, getCmpReg()}, a{a}, b{b}, op{op} {
        newUse(a.get(), this, reg.get());
        newUse(b.get(), this, reg.get());
        reg->I = this;
        kind = kindMap[op];
    };
    
    virtual void print() override;
    virtual bool replaceOperand(ValuePtr target, ValuePtr newValue) override;
    virtual unsigned getOperandCount() override;
    virtual ValuePtr getOperand(unsigned i) override;

    friend struct FcmpInstruction;
};

struct FcmpInstruction:public Instruction {
    ValuePtr a;
    ValuePtr b;
    string op;
    ~FcmpInstruction();
    FcmpInstruction(shared_ptr<BasicBlock> bb, ValuePtr a, ValuePtr b = Const::createConstant(Type::getFloat(), (float)0)/*ValuePtr(new Const(Type::getFloat(), (float)0))*/, string op = "!=") : Instruction{InsID::Fcmp, bb, IcmpInstruction::getCmpReg()}, a{a}, b{b}, op{op} {
        newUse(a.get(), this, reg.get());
        newUse(b.get(), this, reg.get());
        reg->I = this;
    };
    virtual void print() override;
    virtual bool replaceOperand(ValuePtr target, ValuePtr newValue) override;
    virtual unsigned getOperandCount() override;
    virtual ValuePtr getOperand(unsigned i) override;
};

struct BrInstruction:public Instruction {
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
    ValuePtr exp = nullptr;
    LabelPtr label1;
    LabelPtr label2;
    ~BrInstruction();
    BrInstruction(ValuePtr exp, LabelPtr label1, LabelPtr label2, shared_ptr<BasicBlock> bb) : Instruction{InsID::Br, bb}, exp{exp}, label1{label1}, label2{label2} {
        newUse(exp.get(), this);
    }
    BrInstruction(LabelPtr label, shared_ptr<BasicBlock> bb) : Instruction{InsID::Br, bb}, label1{label} {}
    virtual void print() override;
    virtual bool replaceOperand(ValuePtr target, ValuePtr newValue) override;
    virtual unsigned getOperandCount() override;
    virtual ValuePtr getOperand(unsigned i) override;
};

struct PhiInstruction:public Instruction {
    static int phiRegNum; 
    bool specialTag = false;
    static ValuePtr getPhiReg(TypePtr type) { return ValuePtr(new Reg(type, "phi" + to_string(phiRegNum++))); }
    ValuePtr val;
    vector<std::pair<ValuePtr,shared_ptr<BasicBlock>>> from;
    string op;
    ~PhiInstruction();
    PhiInstruction(shared_ptr<BasicBlock> bb, ValuePtr val) : Instruction{InsID::Phi, bb, getPhiReg(val->type)}, val{val} {
        reg->I = this;
    }; 
    PhiInstruction(shared_ptr<BasicBlock> bb, TypePtr type) : Instruction{InsID::Phi, bb, getPhiReg(type)}, val{nullptr} {
        val = Const::createConstant(type,0);
        reg->I = this;
    }; 
    void addFrom(ValuePtr value, shared_ptr<BasicBlock> pred){
        from.push_back({value,pred});
        newUse(value.get(), this, reg.get());
    }
    void removeIncomingByBB(shared_ptr<BasicBlock> bb);
    virtual void print() override;
    virtual bool replaceOperand(ValuePtr target, ValuePtr newValue) override;
    virtual unsigned getOperandCount() override;
    virtual ValuePtr getOperand(unsigned i) override;
};

struct SelectInstruction:public Instruction {
    static int selRegNum; 
    static ValuePtr getSelReg(TypePtr type) { return ValuePtr(new Reg(type, "sel" + to_string(selRegNum++))); }
    ValuePtr exp;
    ValuePtr val1,val2; 

    ~SelectInstruction();
    SelectInstruction(shared_ptr<BasicBlock> bb, ValuePtr exp, ValuePtr val1, ValuePtr val2) : Instruction{InsID::Select, bb, getSelReg(val1->type)}, exp{exp}, val1{val1}, val2{val2} {
        reg->I = this;
    }; 

    virtual void print() override;
    virtual bool replaceOperand(ValuePtr target, ValuePtr newValue) override;
    virtual unsigned getOperandCount() override;
    virtual ValuePtr getOperand(unsigned i) override;
};

struct FuncCastInstruction:public Instruction {
    string funcName;
    FuncCastInstruction(string name, shared_ptr<BasicBlock> bb) : Instruction{InsID::FuncCast, bb}, funcName{name} {

    };
    ~FuncCastInstruction();

    virtual void print() override;
    virtual bool replaceOperand(ValuePtr target, ValuePtr newValue) override;

    virtual unsigned getOperandCount() override;
    virtual ValuePtr getOperand(unsigned i) override;
};

struct AtomicAddInstruction:public Instruction {
    ValuePtr addValue;
    ValuePtr desVar;
    AtomicAddInstruction(ValuePtr val, ValuePtr var, shared_ptr<BasicBlock> bb) : Instruction{InsID::AtomicAdd, bb}, addValue{val}, desVar{var} {

    };
    ~AtomicAddInstruction();

    virtual void print() override;
    virtual bool replaceOperand(ValuePtr target, ValuePtr newValue) override;

    virtual unsigned getOperandCount() override;
    virtual ValuePtr getOperand(unsigned i) override;
};

void replaceVarByVar(ValuePtr from, ValuePtr to);
void replaceVarByVarForLCSSA(ValuePtr from, ValuePtr to, Use* use);
void deleteUser(ValuePtr user);
void deleteUser(InstructionPtr user);
void deleteUser(Instruction* user);

void insertInstructionBefore(InstructionPtr instruction, Instruction* InsertBefore);
InstructionPtr createBinaryBefore(ValuePtr op1, ValuePtr op2, char op, InstructionPtr insBefore);
bool isFneg(BinaryInstruction* bi);
bool mayBeDeadCode(Instruction* Ins);