#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include "symbol.h"
#include <assert.h>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <set>
#include <cstdint>

#ifndef ERROR
#define ERROR(...)                                                                                                     \
    do {                                                                                                               \
        char message[256];                                                                                             \
        sprintf(message, __VA_ARGS__);                                                                                 \
        std::cerr << "\033[;31;1m";                                                                                    \
        std::cerr << "ERROR: ";                                                                                        \
        std::cerr << "\033[0;37;1m";                                                                                   \
        std::cerr << message << std::endl;                                                                             \
        std::cerr << "\033[0;33;1m";                                                                                   \
        std::cerr << "File: \033[4;37;1m" << __FILE__ << ":" << __LINE__ << std::endl;                                 \
        std::cerr << "\033[0m";                                                                                        \
        assert(false);                                                                                                 \
    } while (0)
#endif

#define ENABLE_TODO
#ifdef ENABLE_TODO
#ifndef TODO
#define TODO(...)                                                                                                      \
    do {                                                                                                               \
        char message[256];                                                                                             \
        sprintf(message, __VA_ARGS__);                                                                                 \
        std::cerr << "\033[;34;1m";                                                                                    \
        std::cerr << "TODO: ";                                                                                         \
        std::cerr << "\033[0;37;1m";                                                                                   \
        std::cerr << message << std::endl;                                                                             \
        std::cerr << "\033[0;33;1m";                                                                                   \
        std::cerr << "File: \033[4;37;1m" << __FILE__ << ":" << __LINE__ << std::endl;                                 \
        std::cerr << "\033[0m";                                                                                        \
        assert(false);                                                                                                 \
    } while (0)
#endif
#else
#ifndef TODO
#define TODO(...)
#endif
#endif

#define ENABLE_LOG
#ifdef ENABLE_LOG
#ifndef Log
#define Log(...)                                                                                                       \
    do {                                                                                                               \
        char message[256];                                                                                             \
        sprintf(message, __VA_ARGS__);                                                                                 \
        std::cerr << "\033[;35;1m[\033[4;33;1m" << __FILE__ << ":" << __LINE__ << "\033[;35;1m "                       \
                  << __PRETTY_FUNCTION__ << "]";                                                                       \
        std::cerr << "\033[0;37;1m ";                                                                                  \
        std::cerr << message << std::endl;                                                                             \
        std::cerr << "\033[0m";                                                                                        \
    } while (0)
#endif
#else
#ifndef Log
#define Log(...)
#endif
#endif

#ifndef Assert
#define Assert(EXP)                                                                                                    \
    do {                                                                                                               \
        if (!(EXP)) {                                                                                                  \
            ERROR("Assertion failed: %s", #EXP);                                                                       \
        }                                                                                                              \
    } while (0)
#endif


// 你首先需要阅读utils/Instruction_out.cc来了解指令是如何输出的
// 除注释特别说明外，成员函数不允许设置为nullptr，否则指令输出时会出现段错误

// 我们规定，对于GlobalOperand, LabelOperand, RegOperand, 只要操作数相同, 地址也相同
// 所以这些Operand的构造函数是private, 使用GetNew***Operand函数来获取新的操作数变量

// 对于不同位置的指令，即使内容完全相同，也不推荐使用相同的地址, 
// 当你向基本块中插入指令时，推荐先new一个，不要使用之前已经插入过的指令，你需要保证地址不同，
// 否则在代码优化阶段你会需要调试一些奇怪的bug。(例如你修改了一处指令的操作数，却发现好几条指令的操作数都被修改了)

// 对于Operand，大部分Operand只要内容相同，地址就相同
// 所以最好不要对Operand相关类增加修改私有变量的函数。
// 否则你可能修改了一个Operand的私有变量，然后发现所有Operand的私有变量都被修改了
// 如果你想修改一条指令的操作数，不要直接修改操作数的私有变量，
// 你可以先使用GetNew***Operand函数来获取一个新的操作数op2，然后设置指令的操作数为op2

// 请注意代码中的typedef，为了方便书写，将一些类的指针进行了重命名, 


class BasicOperand;
typedef BasicOperand *Operand;
// @operands in instruction
class BasicOperand {
public:
    enum operand_type { REG = 1, IMMI32 = 2, IMMF32 = 3, GLOBAL = 4, LABEL = 5, IMMI64 = 6 };

protected:
    operand_type operandType;

public:
    BasicOperand() {}
    virtual ~BasicOperand() = default;
    operand_type GetOperandType() { return operandType; }
    virtual std::string GetFullName() = 0;
    virtual BasicOperand* Clone() const = 0;  
};

// @register operand;%r+register No
class RegOperand : public BasicOperand {
    int reg_no;
    RegOperand(int RegNo) {
        this->operandType = REG;
        this->reg_no = RegNo;
    }

public:
    int GetRegNo() { return reg_no; }
    friend RegOperand *GetNewRegOperand(int RegNo);
    virtual std::string GetFullName() override;
    virtual BasicOperand* Clone() const override;
};
RegOperand *GetNewRegOperand(int RegNo);

// @integer32 immediate
class ImmI32Operand : public BasicOperand {
    int immVal;

public:
    int GetIntImmVal() { return immVal; }

    ImmI32Operand(int immVal) {
        this->operandType = IMMI32;
        this->immVal = immVal;
    }
    virtual std::string GetFullName() override;
    virtual BasicOperand* Clone() const override;
};

// @integer64 immediate
class ImmI64Operand : public BasicOperand {
    long long immVal;

public:
    long long GetLlImmVal() { return immVal; }

    ImmI64Operand(long long immVal) {
        this->operandType = IMMI64;
        this->immVal = immVal;
    }
    virtual std::string GetFullName() override;
    virtual BasicOperand* Clone() const override;
};

// @float32 immediate
class ImmF32Operand : public BasicOperand {
    float immVal;

public:
    float GetFloatVal() { return immVal; }

    long long GetFloatByteVal();

    ImmF32Operand(float immVal) {
        this->operandType = IMMF32;
        this->immVal = immVal;
    }
    virtual std::string GetFullName() override;
    virtual BasicOperand* Clone() const override;
};

// @label %L+label No
class LabelOperand : public BasicOperand {
    int label_no;
    LabelOperand(int LabelNo) {
        this->operandType = LABEL;
        this->label_no = LabelNo;
    }

public:
    int GetLabelNo() { return label_no; }

    friend LabelOperand *GetNewLabelOperand(int LabelNo);
    virtual std::string GetFullName() override;
    virtual BasicOperand* Clone() const override;
};

LabelOperand *GetNewLabelOperand(int RegNo);

// @global identifier @+name
class GlobalOperand : public BasicOperand {
    std::string name;
    GlobalOperand(std::string gloName) {
        this->operandType = GLOBAL;
        this->name = gloName;
    }

public:
    std::string GetName() { return name; }
    friend GlobalOperand *GetNewGlobalOperand(std::string name);
    virtual std::string GetFullName() override;
    virtual BasicOperand* Clone() const override;
};

GlobalOperand *GetNewGlobalOperand(std::string name);

class BasicInstruction;

typedef BasicInstruction *Instruction;

// @instruction
class BasicInstruction {
public:
    // @Instriction types
    enum LLVMIROpcode {
        OTHER = 0,
        LOAD = 1,
        STORE = 2,
        ADD = 3,
        SUB = 4,
        ICMP = 5,
        PHI = 6,
        ALLOCA = 7,
        MUL = 8,
        DIV = 9,
        BR_COND = 10,
        BR_UNCOND = 11,
        FADD = 12,
        FSUB = 13,
        FMUL = 14,
        FDIV = 15,
        FCMP = 16,
        MOD = 17,
        BITXOR = 18,
        RET = 19,
        ZEXT = 20,
        SHL = 21,
        FPTOSI = 24,
        GETELEMENTPTR = 25,
        CALL = 26,
        SITOFP = 27,
        GLOBAL_VAR = 28,
        GLOBAL_STR = 29,
        BITCAST = 30,
        
    };


    // @Operand datatypes
    enum LLVMType { I32 = 1, FLOAT32 = 2, PTR = 3, VOID = 4, I8 = 5, I1 = 6, I64 = 7, DOUBLE = 8 };

    // @ <cond> in icmp Instruction
    enum IcmpCond {
        eq = 1,     //: equal
        ne = 2,     //: not equal
        ugt = 3,    //: unsigned greater than
        uge = 4,    //: unsigned greater or equal
        ult = 5,    //: unsigned less than
        ule = 6,    //: unsigned less or equal
        sgt = 7,    //: signed greater than
        sge = 8,    //: signed greater or equal
        slt = 9,    //: signed less than
        sle = 10    //: signed less or equal
    };

    enum FcmpCond {
        FALSE = 1,    //: no comparison, always returns false
        OEQ = 2,      //: ordered and equal
        OGT = 3,      //: ordered and greater than
        OGE = 4,      //: ordered and greater than or equal
        OLT = 5,      //: ordered and less than
        OLE = 6,      //: ordered and less than or equal
        ONE = 7,      //: ordered and not equal
        ORD = 8,      //: ordered (no nans)
        UEQ = 9,      //: unordered or equal
        UGT = 10,     //: unordered or greater than
        UGE = 11,     //: unordered or greater than or equal
        ULT = 12,     //: unordered or less than
        ULE = 13,     //: unordered or less than or equal
        UNE = 14,     //: unordered or not equal
        UNO = 15,     //: unordered (either nans)
        TRUE = 16     //: no comparison, always returns true
    };

private:
    int BlockID = 0;

protected:
    LLVMIROpcode opcode;

public:
    int GetOpcode() { return opcode; }    // one solution: convert to pointer of subclasses
    int GetBlockID(){ return BlockID; }
    void SetBlockID(int id){ BlockID = id; }
    virtual ~BasicInstruction() = default;
    virtual void PrintIR(std::ostream &s) = 0;
    virtual Operand GetResult() = 0;
    virtual void SetResult(Operand op) = 0;
    virtual int GetDefRegno() = 0;
    virtual std::set<int> GetUseRegno() = 0;
    virtual void ChangeReg(const std::map<int, int> &store_map, const std::map<int, int> &use_map) =0;
    virtual void ChangeResult(const std::map<int, int> &regNo_map) = 0; 
    virtual std::vector<Operand> GetNonResultOperands() = 0;
    virtual void SetNonResultOperands(std::vector<Operand> ops) = 0;
    virtual bool isPhi() const { return opcode == PHI; }
    virtual bool isTerminator() const {
        return opcode == BR_COND || opcode == BR_UNCOND || opcode == RET;
    }
    virtual bool isSame(const Instruction& other)=0; //判断两指令内容是否完全一致
	// 如果要通过基类指针 Instruction 拷贝其实际指向的子类对象 (loopRoate 首次用到)
    virtual BasicInstruction* Clone() const = 0; 
	virtual bool mayReadFromMemory() const { return false; }
	virtual bool mayWriteToMemory() const { return false; }
	virtual enum LLVMType GetType() = 0; // 获取指令的类型
};

// load
// Syntax: <result>=load <ty>, ptr <pointer>
class LoadInstruction : public BasicInstruction {
    enum LLVMType type;
    Operand pointer;
    Operand result;

public:
    enum LLVMType GetDataType() { return type; }
    enum LLVMType GetType() override { return type; }
    Operand GetPointer() { return pointer; }
    void SetPointer(Operand op) { pointer = op; }
    Operand GetResult() override { return result; }

    LoadInstruction(enum LLVMType type, Operand pointer, Operand result) {
        opcode = LLVMIROpcode::LOAD;
        this->type = type;
        this->result = result;
        this->pointer = pointer;
    }
    void PrintIR(std::ostream &s) override;
    int GetDefRegno() override;
    std::set<int> GetUseRegno() override;
    void ChangeReg(const std::map<int, int> &store_map, const std::map<int, int> &use_map) override;
    void ChangeResult(const std::map<int, int> &regNo_map) override;
    std::vector<Operand> GetNonResultOperands() override {
        std::vector<Operand> vec;
        vec.push_back(pointer);
        return vec;
    }
    void SetNonResultOperands(std::vector<Operand> ops) override {
        if(ops.size() > 0) {
            pointer = ops[0];
        }
    }
    virtual BasicInstruction* Clone() const override;
    inline void SetResult(Operand op) override { result = op; }
	bool mayReadFromMemory() const override { return true; }
	bool mayWriteToMemory() const override { return false; }
    bool isSame(const Instruction& other) override {return false;}
};

// store
// Syntax: store <ty> <value>, ptr<pointer>
class StoreInstruction : public BasicInstruction {
    enum LLVMType type;
    Operand pointer;		// the address want to be store
    Operand value;			// the value want to be store

public:
    enum LLVMType GetDataType() { return type; }
    enum LLVMType GetType() override { return type; }
    Operand GetPointer() { return pointer; }
    Operand GetValue() { return value; }
    void SetValue(Operand op) { value = op; }
    void SetPointer(Operand op) { pointer = op; }

    StoreInstruction(enum LLVMType type, Operand pointer, Operand value) {
        opcode = LLVMIROpcode::STORE;
        this->type = type;
        this->pointer = pointer;
        this->value = value;
    }

    virtual void PrintIR(std::ostream &s) override;
    Operand GetResult() override { return nullptr; };
    int GetDefRegno() override;
    std::set<int> GetUseRegno() override;
    void ChangeReg(const std::map<int, int> &store_map, const std::map<int, int> &use_map) override;
    void ChangeResult(const std::map<int, int> &regNo_map) override;
    std::vector<Operand> GetNonResultOperands() override {
        std::vector<Operand> vec;
        vec.push_back(pointer);
        vec.push_back(value);
        return vec;
    }
    void SetNonResultOperands(std::vector<Operand> ops) override {
        if(ops.size() > 0) {
            pointer = ops[0];
        }
        if(ops.size() > 1) {
            value = ops[1];
        }
    }
    virtual BasicInstruction* Clone() const override;
    inline void SetResult(Operand op) override { }
	bool mayReadFromMemory() const override { return false; }
	bool mayWriteToMemory() const override { return true; }
    bool isSame(const Instruction& other) override {return false;}
};

//<result>=add <ty> <op1>,<op2>
//<result>=sub <ty> <op1>,<op2>
//<result>=mul <ty> <op1>,<op2>
//<result>=div <ty> <op1>,<op2>
//<result>=xor <ty> <op1>,<op2>
class ArithmeticInstruction : public BasicInstruction {
    enum LLVMType type;
    Operand op1;
    Operand op2;
    Operand result;

public:
    enum LLVMType GetDataType() { return type; }
    enum LLVMType GetType() override { return type; }
    Operand GetOperand1() { return op1; }
    Operand GetOperand2() { return op2; }
    Operand GetResult() override { return result; }
    void SetType(enum LLVMType newtype) { type = newtype; }
    void SetOperand1(Operand op) { op1 = op; }
    void SetOperand2(Operand op) { op2 = op; }
    void SetResultReg(Operand op) { result = op; }
    void SetOpcode(LLVMIROpcode id) { opcode = id; }
    ArithmeticInstruction(){}
    ArithmeticInstruction(LLVMIROpcode opcode, enum LLVMType type, Operand op1, Operand op2, Operand result) {
        this->opcode = opcode;
        this->op1 = op1;
        this->op2 = op2;
        this->result = result;
        this->type = type;
    }

    virtual void PrintIR(std::ostream &s) override;
    int GetDefRegno() override;
    std::set<int> GetUseRegno() override;
    void ChangeReg(const std::map<int, int> &store_map, const std::map<int, int> &use_map) override;
    void ChangeResult(const std::map<int, int> &regNo_map) override;
    std::vector<Operand> GetNonResultOperands() override {
        std::vector<Operand> vec;
        vec.push_back(op1);
        vec.push_back(op2);
        return vec;
    }
    void SetNonResultOperands(std::vector<Operand> ops) override {
        if(ops.size() > 0) {
            op1 = ops[0];
        }
        if(ops.size() > 1) {
            op2 = ops[1];
        }
    }
    int CompConst(int value1, int value2);
    float CompConst(float value1, float value2);
    virtual BasicInstruction* Clone() const override;
    inline void SetResult(Operand op) override { result = op; }
    bool isSame(const Instruction& other) override {
        ArithmeticInstruction* other_inst = (ArithmeticInstruction*) other;
        if(other_inst->GetOpcode()==this->opcode){
            if(this->op1->GetFullName()==other_inst->GetOperand1()->GetFullName() &&
               this->op2->GetFullName()==other_inst->GetOperand2()->GetFullName() ){
                return true;
               }
        }
        return false;
    }
};

//<result>=icmp <cond> <ty> <op1>,<op2>
class IcmpInstruction : public BasicInstruction {
    enum LLVMType type;
    Operand op1;
    Operand op2;
    IcmpCond cond;
    Operand result;

public:
    enum LLVMType GetDataType() { return type; }
    enum LLVMType GetType() override { return type; }
    Operand GetOp1() { return op1; }
    Operand GetOp2() { return op2; }
    IcmpCond GetCond() { return cond; }
    Operand GetResult() override { return result; }
    void SetOp1(Operand op) { op1 = op; }
    void SetOp2(Operand op) { op2 = op; }
    void SetCond(IcmpCond newcond) { cond = newcond; }

    IcmpInstruction(enum LLVMType type, Operand op1, Operand op2, IcmpCond cond, Operand result) {
        this->opcode = LLVMIROpcode::ICMP;
        this->type = type;
        this->op1 = op1;
        this->op2 = op2;
        this->cond = cond;
        this->result = result;
    }
    virtual void PrintIR(std::ostream &s) override;
    
    int GetDefRegno() override;
    std::set<int> GetUseRegno() override;
    void ChangeReg(const std::map<int, int> &store_map, const std::map<int, int> &use_map) override;
    void ChangeResult(const std::map<int, int> &regNo_map) override;
    std::vector<Operand> GetNonResultOperands() override {
        std::vector<Operand> vec;
        vec.push_back(op1);
        vec.push_back(op2);
        return vec;
    }
    void SetNonResultOperands(std::vector<Operand> ops) override {
        if(ops.size() > 0) {
            op1 = ops[0];
        }
        if(ops.size() > 1) {
            op2 = ops[1];
        }
    }
    int CompConst(int value1, int value2);
    virtual BasicInstruction* Clone() const override;
    inline void SetResult(Operand op) override { result = op; }
    IcmpCond GetCompareCondition() { return cond; }
    bool isSame(const Instruction& other) override {return false;}
};

//<result>=fcmp <cond> <ty> <op1>,<op2>
class FcmpInstruction : public BasicInstruction {
    enum LLVMType type;
    Operand op1;
    Operand op2;
    FcmpCond cond;
    Operand result;

public:
    enum LLVMType GetDataType() { return type; }
    enum LLVMType GetType() override { return type; }
    Operand GetOp1() { return op1; }
    Operand GetOp2() { return op2; }
    FcmpCond GetCond() { return cond; }
    Operand GetResult() override { return result; }

    FcmpInstruction(enum LLVMType type, Operand op1, Operand op2, FcmpCond cond, Operand result) {
        this->opcode = LLVMIROpcode::FCMP;
        this->type = type;
        this->op1 = op1;
        this->op2 = op2;
        this->cond = cond;
        this->result = result;
    }
    virtual void PrintIR(std::ostream &s) override;
    int GetDefRegno() override;
    std::set<int> GetUseRegno() override;
    void ChangeReg(const std::map<int, int> &store_map, const std::map<int, int> &use_map) override;
    void ChangeResult(const std::map<int, int> &regNo_map) override;
    std::vector<Operand> GetNonResultOperands() override {
        std::vector<Operand> vec;
        vec.push_back(op1);
        vec.push_back(op2);
        return vec;
    }
    void SetNonResultOperands(std::vector<Operand> ops) override {
        if(ops.size() > 0) {
            op1 = ops[0];
        }
        if(ops.size() > 1) {
            op2 = ops[1];
        }
    }
    float CompConst(float value1,float value2);
    virtual BasicInstruction* Clone() const override;
    inline void SetResult(Operand op) override { result = op; }
    FcmpCond GetCompareCondition() { return cond; }
    bool isSame(const Instruction& other) override {return false;}
};

// phi syntax:
//<result>=phi <ty> [val1,label1],[val2,label2],……
// pair.first -> label ; pair.second -> val
class PhiInstruction : public BasicInstruction {
private:
    enum LLVMType type;
    Operand result;
    int regno; // 新增，记录这个phi属于哪一个寄存器
    std::set<int> def_regno; // 记录这个phi的源值寄存器号
    std::vector<std::pair<Operand, Operand>> phi_list;// label-reg

public:
    enum LLVMType GetType() override { return type; }
    PhiInstruction(enum LLVMType type, Operand result, decltype(phi_list) val_labels) {
        this->opcode = LLVMIROpcode::PHI;
        this->type = type;
        this->result = result;
        this->phi_list = val_labels;
        for(auto &[label,reg]:phi_list){
            if(reg->GetOperandType()==BasicOperand::REG){
                def_regno.insert(((RegOperand*)reg)->GetRegNo());
            }
        }
    }
    PhiInstruction(enum LLVMType type, Operand result, int regno) {
        this->opcode = LLVMIROpcode::PHI;
        this->type = type;
        this->result = result;
        this->regno = regno;
    }
    int GetRegno() { return regno; }
    void AddPhi(std::pair<Operand, Operand> phi){ phi_list.push_back(phi); def_regno.insert(((RegOperand*)phi.second)->GetRegNo());}
    virtual void PrintIR(std::ostream &s) override;
    Operand GetResult() override { return result; };
    LLVMType GetResultType(){ return type; };
    std::vector<std::pair<Operand, Operand>> GetPhiList(){ return phi_list; }
    void SetPhiList(std::vector<std::pair<Operand, Operand>> new_list){ 
        phi_list=new_list;
        for(auto &[label,reg]:phi_list){
            if(reg->GetOperandType()==BasicOperand::REG){
                def_regno.insert(((RegOperand*)reg)->GetRegNo());
            }
        }
    }
    void ChangePhiPair(int index, std::pair<Operand, Operand> new_pair){ 
        if (phi_list[index].second->GetOperandType() == BasicOperand::REG) {
            def_regno.erase(((RegOperand*)phi_list[index].second)->GetRegNo());
        }
        phi_list[index] = new_pair; 
        if (phi_list[index].second->GetOperandType() == BasicOperand::REG) {
            def_regno.insert(((RegOperand*)phi_list[index].second)->GetRegNo());
        }
    }
    int GetDefRegno() override;
    std::set<int> GetUseRegno() override;
    void ChangeReg(const std::map<int, int> &store_map, const std::map<int, int> &use_map) override;
    void ChangeResult(const std::map<int, int> &regNo_map) override;
    std::vector<Operand> GetNonResultOperands() override {
        std::vector<Operand> vec;
        for(auto [labelop, valop] : phi_list){
            vec.push_back(valop);
        }
        return vec;
    }
    void SetNonResultOperands(std::vector<Operand> ops) override {
        for(int i = 0; i < ops.size() && i < phi_list.size(); i++) {
            phi_list[i].second = ops[i];
        }
    }
    bool NotEqual(Operand op1, Operand op2);
    virtual BasicInstruction* Clone() const override;
    inline void SetResult(Operand op) override { result = op; }
    bool isSame(const Instruction& other) override {return false;}
};

// alloca
// usage 1: <result>=alloca <type>
// usage 2: %3 = alloca [20 x [20 x i32]]
class AllocaInstruction : public BasicInstruction {
    enum LLVMType type;
    Operand result;
    std::vector<int> dims;

public:
    enum LLVMType GetDataType() { return type; }
    enum LLVMType GetType() override { return (dims.size() == 0) ? type : LLVMType::PTR; }
    Operand GetResult() override { return result; }
    std::vector<int> GetDims() { return dims; }
    AllocaInstruction(enum LLVMType dttype, Operand result) {
        this->opcode = LLVMIROpcode::ALLOCA;
        this->type = dttype;
        this->result = result;
    }
    AllocaInstruction(enum LLVMType dttype, std::vector<int> ArrDims, Operand result) {
        this->opcode = LLVMIROpcode::ALLOCA;
        this->type = dttype;
        this->result = result;
        dims = ArrDims;
    }

    virtual void PrintIR(std::ostream &s) override;
    
    int GetDefRegno() override;
    std::set<int> GetUseRegno() override;
    void ChangeReg(const std::map<int, int> &store_map, const std::map<int, int> &use_map) override;
    void ChangeResult(const std::map<int, int> &regNo_map) override;
    std::vector<Operand> GetNonResultOperands() override {
        std::vector<Operand> vec;
        return vec;
    }
    void SetNonResultOperands(std::vector<Operand> ops) override {
        // No operands to set
    }
    virtual BasicInstruction* Clone() const override;
    inline void SetResult(Operand op) override { result = op; }
    bool isSame(const Instruction& other) override {return false;}
};

// Conditional branch
// Syntax:
// br i1 <cond>, label <iftrue>, label <iffalse>
class BrCondInstruction : public BasicInstruction {
    Operand cond;
    Operand trueLabel;
    Operand falseLabel;

public:
    enum LLVMType GetType() override { return VOID; }
    Operand GetCond() { return cond; }
    Operand GetTrueLabel() { return trueLabel; }
    Operand GetFalseLabel() { return falseLabel; }
    BrCondInstruction(Operand cond, Operand trueLabel, Operand falseLabel) {
        this->opcode = BR_COND;
        this->cond = cond;
        this->trueLabel = trueLabel;
        this->falseLabel = falseLabel;
    }
    void SetCond(Operand op){ cond = op; }
    void SetTrueLabel(Operand op){ trueLabel = op; }
    void SetFalseLabel(Operand op){ falseLabel = op; }
    void ChangeTrueLabel(Operand op) { trueLabel=op; }
    void ChangeFalseLabel(Operand op) { falseLabel=op; }
    virtual void PrintIR(std::ostream &s) override;
    Operand GetResult() override { return nullptr; };
    int GetDefRegno() override;
    std::set<int> GetUseRegno() override;
    void ChangeReg(const std::map<int, int> &store_map, const std::map<int, int> &use_map) override;
    void ChangeResult(const std::map<int, int> &regNo_map) override;
    std::vector<Operand> GetNonResultOperands() override {
        std::vector<Operand> vec;
        vec.push_back(cond);
        vec.push_back(trueLabel);
        vec.push_back(falseLabel);
        return vec;
    }
    void SetNonResultOperands(std::vector<Operand> ops) override {
        if(ops.size() > 0) {
            cond = ops[0];
        }
        if(ops.size() > 1) {
            trueLabel = ops[1];
        }
        if(ops.size() > 2) {
            falseLabel = ops[2];
        }
    }
    virtual BasicInstruction* Clone() const override;
    inline void SetResult(Operand op) override { }
    bool isSame(const Instruction& other) override {return false;}
};

// Unconditional branch
// Syntax:
// br label <dest>
class BrUncondInstruction : public BasicInstruction {
    Operand destLabel;

public:
    enum LLVMType GetType() override { return VOID; }
    Operand GetDestLabel() { return destLabel; }
    BrUncondInstruction(Operand destLabel) {
        this->opcode = BR_UNCOND;
        this->destLabel = destLabel;
    }
    void SetTarget(Operand op){ destLabel = op; }
    virtual void PrintIR(std::ostream &s) override;
    Operand GetResult() override { return nullptr; };
    int GetDefRegno() override;
    std::set<int> GetUseRegno() override;
    void ChangeDestLabel(Operand op) { destLabel=op; }
    void ChangeReg(const std::map<int, int> &store_map, const std::map<int, int> &use_map) override;
    void ChangeResult(const std::map<int, int> &regNo_map) override;
    std::vector<Operand> GetNonResultOperands() override {
        std::vector<Operand> vec;
        vec.push_back(destLabel);
        return vec;
    }
    void SetNonResultOperands(std::vector<Operand> ops) override {
        if(ops.size() > 0) {
            destLabel = ops[0];
        }
    }
    virtual BasicInstruction* Clone() const override;
    inline void SetResult(Operand op) override { }
    bool isSame(const Instruction& other) override {return false;}
};

/*
Global Id Define Instruction Syntax
Example 1:
    @p = global [105 x i32] zeroinitializer
Example 2:
    @p = global [105 x [104 x i32]] [[104 x i32] [], [104 x i32] zeroinitializer, ...]
*/
class GlobalVarDefineInstruction : public BasicInstruction {
public:
    // 如果arrayval的dims为空, 则该定义为非数组, 初始值为init_val
    // 如果arrayval的dims为空, 且init_val为nullptr, 则初始值为0

    // 如果arrayval的dims不为空, 则该定义为数组, 初始值在arrayval中
    enum LLVMType type;
    std::string name;
    Operand init_val;
    VarAttribute arrayval;
    bool is_const; //源代码中是否声明为常量
    bool has_initval;//源代码中是否有初始值
    GlobalVarDefineInstruction(std::string nam, enum LLVMType typ, Operand i_val, bool is_const,bool is_inited)
        : name(nam), type(typ), init_val(i_val),is_const(is_const),has_initval(is_inited) {
        this->opcode = LLVMIROpcode::GLOBAL_VAR;
    }
    GlobalVarDefineInstruction(std::string nam, enum LLVMType typ, VarAttribute v, bool is_const )
        : name(nam), type(typ), arrayval(v), init_val{nullptr} ,is_const(is_const),has_initval(false) {//array的has_initval统一设为false，无实际含义
        this->opcode = LLVMIROpcode::GLOBAL_VAR;
    }
    virtual void PrintIR(std::ostream &s) override;
    Operand GetResult() override { return nullptr; };
    std::string GetName() const { return name; }
    LLVMType GetDataType() const { return type; }
    enum LLVMType GetType() override { return VOID; }
    Operand GetInitVal() const { return init_val; }
    bool IsConst() const { return is_const; }
    bool IsArray() const { return !arrayval.dims.empty(); }
    bool IsInited() {return has_initval;}
    
    int GetDefRegno() override;
    std::set<int> GetUseRegno() override;
    void ChangeReg(const std::map<int, int> &store_map, const std::map<int, int> &use_map) override;
    void ChangeResult(const std::map<int, int> &regNo_map) override;
    std::vector<Operand> GetNonResultOperands() override {
        std::vector<Operand> vec;
        if(init_val != nullptr){
            vec.push_back(init_val);
        }
        return vec;
    }
    void SetNonResultOperands(std::vector<Operand> ops) override {
        if(ops.size() > 0) {
            init_val = ops[0];
        }
    }
    virtual BasicInstruction* Clone() const override;
    inline void SetResult(Operand op) override { }
    bool isSame(const Instruction& other) override {return false;}
};

class GlobalStringConstInstruction : public BasicInstruction {
public:
    std::string str_val;
    std::string str_name;
    enum LLVMType GetType() override { return VOID; }
    GlobalStringConstInstruction(std::string strval, std::string strname) : str_val(strval), str_name(strname) {
        this->opcode = LLVMIROpcode::GLOBAL_STR;
    }

    virtual void PrintIR(std::ostream &s) override;
    
    Operand GetResult() override { return nullptr; };
    
    
    int GetDefRegno() override;
    std::set<int> GetUseRegno() override;
    void ChangeReg(const std::map<int, int> &store_map, const std::map<int, int> &use_map) override;
    void ChangeResult(const std::map<int, int> &regNo_map) override;
    std::vector<Operand> GetNonResultOperands() override {
        std::vector<Operand> vec;
        return vec;
    }
    void SetNonResultOperands(std::vector<Operand> ops) override {
        // No operands to set
    }
    virtual BasicInstruction* Clone() const override;
    inline void SetResult(Operand op) override { }
    bool isSame(const Instruction& other) override {return false;}
};

/*
Call Instruction Syntax
Example 1:
    %12 = call i32 (ptr, ...)@printf(ptr @.str,i32 %11)
Example 2:
    call void @DFS(i32 0,i32 %4)

如果你调用了void类型的函数，将result设置为nullptr即可
*/
class CallInstruction : public BasicInstruction {
    // Datas About the Instruction
private:
    enum LLVMType ret_type;
    Operand result;    // result can be null
    std::string name;
    std::vector<std::pair<enum LLVMType, Operand>> args;

public:
    // Construction Function:Set All datas
    CallInstruction(){}
    CallInstruction(enum LLVMType retType, Operand res, std::string FuncName,
                    std::vector<std::pair<enum LLVMType, Operand>> arguments)
        : ret_type(retType), result(res), name(FuncName), args(arguments) {
        this->opcode = CALL;
        if (res != NULL && res->GetOperandType() == BasicOperand::REG) {
            if (((RegOperand *)res)->GetRegNo() == -1) {
                result = NULL;
            }
        }
    }
    CallInstruction(enum LLVMType retType, Operand res, std::string FuncName)
        : ret_type(retType), result(res), name(FuncName) {
        this->opcode = CALL;
        if (res != NULL && res->GetOperandType() == BasicOperand::REG) {
            if (((RegOperand *)res)->GetRegNo() == -1) {
                result = NULL;
            }
        }
    }

    std::string GetFunctionName() { return name; }
    void SetFunctionName(std::string new_name) { name = new_name; }
    std::vector<std::pair<enum LLVMType, Operand>> GetParameterList() { return args; }
    enum LLVMType GetType() override { return ret_type; }
	void SetParameterList(std::vector<std::pair<enum LLVMType, Operand>> new_args) { args = new_args; }
    void push_back_Parameter(std::pair<enum LLVMType, Operand> newPara) { args.push_back(newPara); }
    void push_back_Parameter(enum LLVMType type, Operand val) { args.push_back(std::make_pair(type, val)); }
    virtual void PrintIR(std::ostream &s) override;
    Operand GetResult() override { return result; };
    LLVMType GetRetType(){ return ret_type; };
    int GetDefRegno() override;
    std::set<int> GetUseRegno() override;
    void ChangeReg(const std::map<int, int> &store_map, const std::map<int, int> &use_map) override;
    void ChangeResult(const std::map<int, int> &regNo_map) override;
    std::vector<Operand> GetNonResultOperands() override {
        std::vector<Operand> vec;
        for(auto [tp, op] : args){
            vec.push_back(op);
        }
        return vec;
    }
    void SetNonResultOperands(std::vector<Operand> ops) override {
        for(int i = 0; i < ops.size() && i < args.size(); i++) {
            args[i].second = ops[i];
        }
    }
    virtual BasicInstruction* Clone() const override;
    inline void SetResult(Operand op) override { result = op; }
	bool mayReadFromMemory() const override { return true; }  // conservatively assume both
	bool mayWriteToMemory() const override { return true; }
    bool isSame(const Instruction& other) override {return false;}
};

/*
Ret Instruction Syntax
Example 1:
    ret i32 0
Example 2:
    ret void
Example 3:
    ret i32 %r7

如果你需要不需要返回值，将ret_val设置为nullptr即可
*/
class RetInstruction : public BasicInstruction {
    // Datas About the Instruction
private:
    enum LLVMType ret_type;
    Operand ret_val;

public:
    // Construction Function:Set All datas
    RetInstruction(enum LLVMType retType, Operand res) : ret_type(retType), ret_val(res) { this->opcode = RET; }
    // Getters
    enum LLVMType GetType() override { return ret_type; }
    Operand GetRetVal() { return ret_val; }
    virtual void PrintIR(std::ostream &s) override;
    Operand GetResult() override { return nullptr; };
    int GetDefRegno() override;
    std::set<int> GetUseRegno() override;
    void ChangeReg(const std::map<int, int> &store_map, const std::map<int, int> &use_map) override;
    void ChangeResult(const std::map<int, int> &regNo_map) override;
    std::vector<Operand> GetNonResultOperands() override {
        std::vector<Operand> vec;
        if(ret_val != nullptr) {
            vec.push_back(ret_val);
        }
        return vec;
    }
    void SetNonResultOperands(std::vector<Operand> ops) override {
        if(ops.size() > 0) {
            ret_val = ops[0];
        }
    }
    virtual BasicInstruction* Clone() const override;
    inline void SetResult(Operand op) override { }
    bool isSame(const Instruction& other) override {return false;}
};

/*
    index_type 表示getelementptr的index类型, 这里我们为了简化实现，统一认为index要么是i32，要么是i64
    如果你有特殊需要，可以自行修改指令类
*/
class GetElementptrInstruction : public BasicInstruction {
private:
    enum LLVMType index_type;
    Operand result;
    Operand ptrval;

    enum LLVMType type;
    std::vector<int> dims; // example: i32, [4 x i32], [3 x [4 x float]]

    std::vector<Operand> indexes;

public:
    GetElementptrInstruction(){}
    GetElementptrInstruction(enum LLVMType typ, Operand res, Operand ptr, LLVMType idx_typ)
        : type(typ), index_type(idx_typ), result(res), ptrval(ptr) {
        opcode = GETELEMENTPTR;
    }
    GetElementptrInstruction(enum LLVMType typ, Operand res, Operand ptr, std::vector<int> dim, LLVMType idx_typ)
        : type(typ), index_type(idx_typ), result(res), ptrval(ptr), dims(dim) {
        opcode = GETELEMENTPTR;
    }
    GetElementptrInstruction(enum LLVMType typ, Operand res, Operand ptr, std::vector<int> dim,
                             std::vector<Operand> index, LLVMType idx_typ)
        : type(typ), index_type(idx_typ), result(res), ptrval(ptr), dims(dim), indexes(index) {
        opcode = GETELEMENTPTR;
    }
    // 新增：线性偏移GEP构造函数
    // typ: 元素类型（如I32），res: 结果reg，ptr: 基址，offset: 偏移量（元素个数），idx_typ: 索引类型
    GetElementptrInstruction(enum LLVMType typ, Operand res, Operand ptr, Operand offset, LLVMType idx_typ)
        : type(typ), index_type(idx_typ), result(res), ptrval(ptr) {
        opcode = GETELEMENTPTR;
        indexes.push_back(offset);
    }
    void set_ptr(Operand ptr) { ptrval = ptr; }
    void push_dim(int d) { dims.push_back(d); }
    void set_dims(std::vector<int> new_dims) { dims = new_dims; }
    void push_idx_reg(int idx_reg_no) { indexes.push_back(GetNewRegOperand(idx_reg_no)); }
    void push_idx_imm32(int imm_idx) { indexes.push_back(new ImmI32Operand(imm_idx)); }
    void push_index(Operand idx) { indexes.push_back(idx); }
    void change_index(int i, Operand op) { indexes[i] = op; }
    void set_indexes(std::vector<Operand> new_indexes) { indexes = new_indexes; }

    enum LLVMType GetType() override { return type; }
    enum LLVMType GetIndexType() { return index_type; }
    Operand GetResult() override { return result; }
    Operand GetPtrVal() { return ptrval; }
    std::vector<int> GetDims() { return dims; }
    std::vector<Operand> GetIndexes() { return indexes; }

    void PrintIR(std::ostream &s) override;
    int GetDefRegno() override;
    std::set<int> GetUseRegno() override;
    void ChangeReg(const std::map<int, int> &store_map, const std::map<int, int> &use_map) override;
    void ChangeResult(const std::map<int, int> &regNo_map) override;
    int ComputeIndex();
    std::vector<Operand> GetNonResultOperands() override {
        std::vector<Operand> vec;
        for(auto op : indexes){
            vec.push_back(op);
        }
        vec.push_back(ptrval);
        return vec;
    }
    void SetNonResultOperands(std::vector<Operand> ops) override {
        if(ops.size() > 0) {
            int i = 0;
            for(; i < ops.size() - 1 && i < indexes.size(); i++) {
                indexes[i] = ops[i];
            }
            if(i < ops.size()) {
                ptrval = ops[i];
            }
        }
    }
    virtual BasicInstruction* Clone() const override;
    inline void SetResult(Operand op) override { result = op; }
    bool isSame(const Instruction& other) override {
        GetElementptrInstruction* other_inst = (GetElementptrInstruction*) other;
        if(this->index_type == other_inst->GetIndexType() &&
           this->type == other_inst->GetType() &&
           this->ptrval->GetFullName() == other_inst->GetPtrVal()->GetFullName()){
            if(this->dims.size()==other_inst->GetDims().size() &&
               this->indexes.size() == other_inst->GetIndexes().size()){
                auto other_dims=other_inst->GetDims();
                auto other_indexes=other_inst->GetIndexes();
                for(int i=0;i<dims.size();i++){
                    if(dims[i]!=other_dims[i]){
                        return false;
                    }
                }
                for(int i=0;i<indexes.size();i++){
                    if(indexes[i]->GetFullName()!=other_indexes[i]->GetFullName()){
                        return false;
                    }
                }
                return true;
            }
        }
        return false;
    }
};

class FunctionDefineInstruction : public BasicInstruction {
private:
    enum LLVMType return_type;
    std::string Func_name;

public:
    std::vector<enum LLVMType> formals;
    std::vector<Operand> formals_reg;

    FunctionDefineInstruction(enum LLVMType t, std::string n) {
        return_type = t;
        Func_name = n;
    }
    void InsertFormal(enum LLVMType t) {
        formals.push_back(t);
        formals_reg.push_back(GetNewRegOperand(formals_reg.size()));
    }
    enum LLVMType GetReturnType() { return return_type; }
    enum LLVMType GetType() override { return VOID; }
    std::string GetFunctionName() { return Func_name; }
    void PrintIR(std::ostream &s) override;
    Operand GetResult() override { return nullptr; };
    
    int GetDefRegno() override;
    std::set<int> GetUseRegno() override;
    void ChangeReg(const std::map<int, int> &store_map, const std::map<int, int> &use_map) override;
    void ChangeResult(const std::map<int, int> &regNo_map) override;
    std::vector<Operand> GetNonResultOperands() override {
        std::vector<Operand> vec;
        for(auto op : formals_reg){
            vec.push_back(op);
        }
        return vec;
    }
    void SetNonResultOperands(std::vector<Operand> ops) override {
        for(int i = 0; i < ops.size() && i < formals_reg.size(); i++) {
            formals_reg[i] = ops[i];
        }
    }
    virtual BasicInstruction* Clone() const override;
    inline void SetResult(Operand op) override { }
    bool isSame(const Instruction& other) override {return false;}
};
typedef FunctionDefineInstruction *FuncDefInstruction;

class FunctionDeclareInstruction : public BasicInstruction {
private:
    enum LLVMType return_type;
    std::string Func_name;

public:
    std::vector<enum LLVMType> formals;
    FunctionDeclareInstruction(enum LLVMType t, std::string n) {
        return_type = t;
        Func_name = n;
    }
    void InsertFormal(enum LLVMType t) { formals.push_back(t); }
    enum LLVMType GetReturnType() { return return_type; }
    enum LLVMType GetType() override { return VOID; }
    std::string GetFunctionName() { return Func_name; }
    void PrintIR(std::ostream &s) override;
    
    Operand GetResult() override { return nullptr; };
    
    int GetDefRegno() override;
    std::set<int> GetUseRegno() override;
    void ChangeReg(const std::map<int, int> &store_map, const std::map<int, int> &use_map) override;
    void ChangeResult(const std::map<int, int> &regNo_map) override;
    std::vector<Operand> GetNonResultOperands() override {
        std::vector<Operand> vec;
        return vec;
    }
    void SetNonResultOperands(std::vector<Operand> ops) override {
        // No operands to set
    }
    virtual BasicInstruction* Clone() const override;
    inline void SetResult(Operand op) override { }
    bool isSame(const Instruction& other) override {return false;}
};

// 这条指令目前只支持float和i32的转换，如果你需要double, i64等类型，需要自己添加更多变量
class FptosiInstruction : public BasicInstruction {
private:
    Operand result;
    Operand value;

public:
    FptosiInstruction(Operand result_receiver, Operand value_for_cast)
        : result(result_receiver), value(value_for_cast) {
        this->opcode = FPTOSI;
    }
    Operand GetResult() override { return result; }
    enum LLVMType GetType() override { return I32; }
    Operand GetSrc() { return value; }
    void PrintIR(std::ostream &s) override;
    int GetDefRegno() override;
    std::set<int> GetUseRegno() override;
    void ChangeReg(const std::map<int, int> &store_map, const std::map<int, int> &use_map) override;
    void ChangeResult(const std::map<int, int> &regNo_map) override;
    std::vector<Operand> GetNonResultOperands() override {
        std::vector<Operand> vec;
        vec.push_back(value);
        return vec;
    }
    void SetNonResultOperands(std::vector<Operand> ops) override {
        if(ops.size() > 0) {
            value = ops[0];
        }
    }
    virtual BasicInstruction* Clone() const override;
    inline void SetResult(Operand op) override { result = op; }
    bool isSame(const Instruction& other) override {return false;}
};

// 这条指令目前只支持float和i32的转换，如果你需要double, i64等类型，需要自己添加更多变量
class SitofpInstruction : public BasicInstruction {
private:
    Operand result;
    Operand value;

public:
    SitofpInstruction(Operand result_receiver, Operand value_for_cast)
        : result(result_receiver), value(value_for_cast) {
        this->opcode = SITOFP;
    }

    Operand GetResult() override { return result; }
    enum LLVMType GetType() override { return FLOAT32; }
    Operand GetSrc() { return value; }
    void PrintIR(std::ostream &s) override;
    int GetDefRegno() override;
    std::set<int> GetUseRegno() override;
    void ChangeReg(const std::map<int, int> &store_map, const std::map<int, int> &use_map) override;
    void ChangeResult(const std::map<int, int> &regNo_map) override;
    std::vector<Operand> GetNonResultOperands() override {
        std::vector<Operand> vec;
        vec.push_back(value);
        return vec;
    }
    void SetNonResultOperands(std::vector<Operand> ops) override {
        if(ops.size() > 0) {
            value = ops[0];
        }
    }
    virtual BasicInstruction* Clone() const override;
    inline void SetResult(Operand op) override { result = op; }
    bool isSame(const Instruction& other) override {return false;}
};

// 无符号扩展，你大概率需要它来将i1无符号扩展至i32(即对应c语言bool类型转int)
class ZextInstruction : public BasicInstruction {
private:
    LLVMType from_type;
    LLVMType to_type;
    Operand result;
    Operand value;

public:
    Operand GetResult() override { return result; }
    enum LLVMType GetType() override { return to_type; }
    Operand GetSrc() { return value; }
    ZextInstruction(LLVMType to_type, Operand result_receiver, LLVMType from_type, Operand value_for_cast)
        : to_type(to_type), result(result_receiver), from_type(from_type), value(value_for_cast) {
        this->opcode = ZEXT;
    }
    void PrintIR(std::ostream &s) override;
    LLVMType GetFromType(){ return from_type; }
    LLVMType GetToType(){ return to_type; }
    int GetDefRegno() override;
    std::set<int> GetUseRegno() override;
    void ChangeReg(const std::map<int, int> &store_map, const std::map<int, int> &use_map) override;
    void ChangeResult(const std::map<int, int> &regNo_map) override;
    std::vector<Operand> GetNonResultOperands() override {
        std::vector<Operand> vec;
        vec.push_back(value);
        return vec;
    }
    void SetNonResultOperands(std::vector<Operand> ops) override {
        if(ops.size() > 0) {
            value = ops[0];
        }
    }
    virtual BasicInstruction* Clone() const override;
    inline void SetResult(Operand op) override { result = op; }
    bool isSame(const Instruction& other) override {return false;}
};

// 位转换指令，用于在不同类型之间进行位级别的转换
class BitCastInstruction : public BasicInstruction {
private:
    LLVMType from_type;
    LLVMType to_type;
    Operand result;
    Operand value;

public:
    Operand GetResult() override { return result; }
    enum LLVMType GetType() override { return to_type; }
    Operand GetSrc() { return value; }
    BitCastInstruction(Operand result_receiver, Operand value_for_cast, LLVMType from_type, LLVMType to_type)
        : result(result_receiver), value(value_for_cast), from_type(from_type), to_type(to_type) {
        this->opcode = BITCAST;
    }
    void PrintIR(std::ostream &s) override;
    LLVMType GetFromType(){ return from_type; }
    LLVMType GetToType(){ return to_type; }
    int GetDefRegno() override;
    std::set<int> GetUseRegno() override;
    void ChangeReg(const std::map<int, int> &store_map, const std::map<int, int> &use_map) override;
    void ChangeResult(const std::map<int, int> &regNo_map) override;
    std::vector<Operand> GetNonResultOperands() override {
        std::vector<Operand> vec;
        vec.push_back(value);
        return vec;
    }
    void SetNonResultOperands(std::vector<Operand> ops) override {
        if(ops.size() > 0) {
            value = ops[0];
        }
    }
    virtual BasicInstruction* Clone() const override;
    inline void SetResult(Operand op) override { result = op; }
    bool isSame(const Instruction& other) override {return false;}
};

std::ostream &operator<<(std::ostream &s, BasicInstruction::LLVMType type);
std::ostream &operator<<(std::ostream &s, BasicInstruction::LLVMIROpcode type);
std::ostream &operator<<(std::ostream &s, BasicInstruction::IcmpCond type);
std::ostream &operator<<(std::ostream &s, BasicInstruction::FcmpCond type);
std::ostream &operator<<(std::ostream &s, Operand op);


// for binary/unary exp irgen
BasicInstruction::LLVMIROpcode mapToLLVMOpcodeInt(OpType::Op op);
BasicInstruction::LLVMIROpcode mapToLLVMOpcodeFloat(OpType::Op op);
BasicInstruction::IcmpCond mapToIcmpCond(OpType::Op op);
BasicInstruction::FcmpCond mapToFcmpCond(OpType::Op op);


#endif
