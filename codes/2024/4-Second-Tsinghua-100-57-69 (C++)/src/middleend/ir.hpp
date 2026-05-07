#pragma once

#include "common/common.hpp"
#include "common/utils.hpp"
#include "common/type.hpp"
#include "middleend/temp.hpp"

#include <variant>
#include <unordered_set>
#include <iostream>
#include <unordered_map>

// #define GetValue(x) (x.type == Int ? x.iv : x.fv)

namespace middleend{
class CFG;

namespace ir{
class Function;
class BasicBlock;

/// @brief meta for data
class DataMeta {
public:
    explicit DataMeta() = default;
    DataMeta(Type type, bool is_global, const std::string & name = "") : 
        type_(type), is_global_(is_global), name_(name) {
            if(is_global){
                if(!type.is_array()){
                    if(type.base_type == Int)
                        init_value = ConstValue(0);
                    else init_value = ConstValue((float)0.0);
                }
            }
        }
    inline std::string to_str() {
        if(type_.is_array()){
            std::string ret = temp->to_str() + " : " + name_;
            for (auto dim : type_.dims) {
                if(dim == 0) ret += "[]";
                else ret += "[" + std::to_string(dim) + "]";
            }
            return ret;
        }
        else{
            return temp->to_str() + " : " + name_;
        }
    }
    inline Type get_data_type() const { return type_; }
    inline bool get_is_global() const { return is_global_; }
    inline std::string get_name() const { return name_; }
    inline Temp *get_temp() const { return temp; }
    inline void set_temp(Temp *temp) { this->temp = temp; }
    inline void set_init_value(ConstValue init_value) {
        if (type_.base_type == Int){
            if (init_value.type == Int)
                this->init_value = ConstValue(init_value.iv);
            else if (init_value.type == Float)
                this->init_value = ConstValue((int)init_value.fv);
        }
        else{
            if (init_value.type == Float)
                this->init_value = ConstValue(init_value.fv);
            else if (init_value.type == Int)
                this->init_value = ConstValue((float)init_value.iv);
        }
    }
    inline void set_init_value(std::vector<ConstValue> init_value) { this->init_value = init_value; }
    inline void add_init_value(ConstValue init_value) { std::get<1>(this->init_value).push_back(init_value); }
    inline std::variant<ConstValue, std::vector<ConstValue>> get_init_value() const { return init_value; }
    inline bool get_can_bss_init() const { return can_bss_init; }
    inline void set_can_bss_init(bool can_bss_init) { this->can_bss_init = can_bss_init; }
    inline bool get_not_init() const { return not_init; }
    inline void set_not_init(bool not_init) { this->not_init = not_init; }
protected:
    Type type_;
    bool is_global_;
    std::string name_;
    Temp *temp;
    std::variant<ConstValue, std::vector<ConstValue>> init_value;
    bool can_bss_init = false;
    bool not_init = false;
};

/// @brief variable
class DataVar : public DataMeta {
public:
    explicit DataVar() = default;
    DataVar(Type var_type, bool is_global, const std::string & name = "") : 
        DataMeta(var_type, is_global, name) {}
    // inline std::string to_str() { return temp->to_str() + " = " + name_; }
};

// array
class DataArray : public DataMeta {
public:
    explicit DataArray() = default;
    DataArray(Type var_type, bool is_global, const std::string & name = "") : 
        DataMeta(var_type, is_global, name) {}
    // inline std::string to_str() {
    //     std::string ret = temp->to_str() + " = " + name_;
    //     for (auto dim : type_.dims) {
    //         ret += "[" + std::to_string(dim) + "]";
    //     }
    //     return ret;
    // }
};

enum IRValueType {
    Value_Argument,
    Value_BasicBlock,
    Value_Constant,
    Value_Function,
    Value_GlobalVariable,
    Value_Instruction
};

/// @brief a typed value that may be used (among other things) as an operand to an instruction
class Value {
public:
    explicit Value() = default;
    Value(IRValueType type, const std::string & name = "") : type_(type), name_(name) {}
    std::string to_str() { return "none to_str method for Value(" + name_ + ")"; }
    IRValueType get_type() const { return type_; }
    std::string get_name() const { return name_; }
protected:
    IRValueType type_;
    std::string name_;
};

enum IRInstrKind {
    IRInstr_LABEL,
    IRInstr_SEQ,
    IRInstr_JMP,
    IRInstr_JMP_COND,
    IRInstr_CALL,
    IRInstr_RET,
};

/// @brief the common base class for instructions
class Instruction : public Value {
public:
    explicit Instruction() = default;
    Instruction(IRInstrKind instrkind, const std::vector<Temp*> & dsts, const std::vector<Temp*> & srcs) : 
        Value(IRValueType::Value_Instruction), instrkind_(instrkind), dsts_(dsts), srcs_(srcs), parent_(nullptr) {}
    virtual std::string to_str() = 0;
    virtual std::string to_llvm_str(int &llvm_temp_cnt) { return "* " + to_str(); }
    inline void set_parent(BasicBlock* parent) { parent_ = parent; }
    inline ir::BasicBlock* get_parent() const { return parent_; }
    inline IRInstrKind get_instr_kind() const { return instrkind_; }
    inline std::vector<Temp*>* get_dst() { return &dsts_; }
    inline std::vector<Temp*>* get_src() { return &srcs_; }
    void print(std::ostream &out) {
        if(instrkind_ != IRInstr_LABEL)print_indent(out, INDENT_LEN);
        out << to_str() << "\n";
    }
    void change_use(Temp* old_temp, Temp* new_temp) {
        for (auto &i: srcs_) {
            if (i == old_temp)
                i = new_temp;
        }
    }
    virtual bool is_output_inst() = 0;

protected:
    BasicBlock* parent_;
    IRInstrKind instrkind_;
    std::vector<Temp*> dsts_;
    std::vector<Temp*> srcs_;
};

/// @brief a single entry single exit section of the code
class BasicBlock : public Value {
public:
    explicit BasicBlock() = default;
    bool visit;
    void clear_visit() { visit = false; }
    BasicBlock(int idx): index_(idx) {}
    BasicBlock(Function* parent, const std::vector<Instruction*> &instructions, int index) : 
        Value(IRValueType::Value_BasicBlock, "_B" + std::to_string(index)), parent_(parent), instructions_(instructions), index_(index) {}
    inline std::vector<Instruction*>* get_instructions() { return &instructions_; }
    inline void add_instruction(Instruction* ins) { instructions_.push_back(ins); ins->set_parent(this); }
    inline void add_instruction_at(int pos, Instruction* ins) { instructions_.insert(instructions_.begin() + pos, ins); ins->set_parent(this); }
    inline void add_instruction_front(Instruction* ins) { instructions_.insert(instructions_.begin(), ins); ins->set_parent(this); }
    inline void add_instruction_before_terminal(Instruction* ins) {
        instructions_.insert(instructions_.end() - 1, ins);
        ins->set_parent(this);
    }
    inline void add_instruction_after_inst(Instruction* ins, Instruction* other) {
        for (auto i = instructions_.begin(); i != instructions_.end(); i++) {
            if (*i == other) {
                instructions_.insert(i+1, ins);
                ins->set_parent(this);
                return;
            }
        }
    }
    // inline void add_instruction_front_after_phi(Instruction* ins) {
    //     for (int i = 0; i < instructions_.size(); ++i) {
    //         TypeCase(phi, ir::instruction::Phi*, instructions_[i]) {
    //             continue;
    //         } else {
    //             instructions_.insert(instructions_.begin() + i, ins);
    //             ins->set_parent(this);
    //             break;
    //         }
    //     }
    // }
    inline void remove_instruction(Instruction* ins) { 
        for (auto it = instructions_.begin(); it != instructions_.end(); ++it) {
            if (*it == ins) {
                instructions_.erase(it);
                break;
            }
        }
    }
    void print(std::ostream &out) const {
        out << name_ << ":\n";
        for (auto &child : instructions_) {
            child->print(out);
        }
    }
    inline int get_index() const { return index_; }
    Function* get_parent() { return parent_; }
    void set_parent(Function* parent) { parent_ = parent; }
protected:
    int index_;
    Function* parent_;
    std::vector<Instruction*> instructions_;
    std::vector<Temp*> define;
    std::vector<Temp*> live_in;
    std::vector<Temp*> live_out;
    std::vector<Temp*> live_use;
};

// class Loop {
// public:
//     Loop *outer;
//     BasicBlock *header;
//     int level;
//     bool no_inner;
//     Loop(BasicBlock *head)
//         : header(head), outer(nullptr), level(-1), no_inner(true) {}
// };


class Module;

/// @brief a single procedure in the program
class Function : public Value {
public:
    explicit Function() = default;
    Function(Module *parent, Type ret_type, const std::vector<DataMeta*> &arguments, const std::string & name) : 
        Value(IRValueType::Value_Function, name), ret_type_(ret_type), parent_(parent), arguments_(arguments), basicblock_used_(0) {}
    inline bool is_main() { return name_ == "main"; }
    inline bool has_return_value() { return ret_type_.base_type != Void; }
    inline Type get_ret_type() const { return ret_type_; }
    inline Module * get_parent() const { return parent_; }
    inline int get_arg_num() const { return arguments_.size(); }
    inline const std::vector<DataMeta*>* get_arguments() const { return &arguments_; }
    inline std::vector<BasicBlock*>* get_basic_blocks() { return &basic_blocks_; }
    inline const std::vector<Instruction*>* get_instructions() const { return &instructions_; }
    inline void add_argument(DataMeta *arg) { arguments_.push_back(arg); }
    inline void add_instruction(Instruction *inst) { instructions_.push_back(inst); }
    inline void add_basic_block(BasicBlock *bb) { basic_blocks_.push_back(bb); }
    inline BasicBlock* get_entry() const { return basic_blocks_[0]; }
    int get_temp_used() { return temp_used_; }
    void set_temp_used(int used) { temp_used_ = used; }
    int get_bb_used() { return basicblock_used_; }
    void set_bb_used(int used) { basicblock_used_ = used; }
    void add_arg_temp(Temp *temp){ arg_temp.push_back(temp); }
    void clear_visit() { for(auto& bb: basic_blocks_) { bb->clear_visit();} }
    std::vector<Temp*>* get_arg_temp() {return &arg_temp;}
    inline bool is_param(Temp *temp) {
        for (int i = 0; i < arg_temp.size(); ++i) {
            if (arg_temp[i] == temp) {
                return true;
            }
        }
        return false;
    }
    void print(std::ostream &out) const {
        out << type_string(ret_type_) << ' ' << name_ << "(";
        for (int i = 0; i < arg_temp.size(); ++i) {
            out << arg_temp[i]->to_str();
            if(i != arg_temp.size() - 1)
                out << ", ";
        }
        out << ") {\n";
        for (auto &child : basic_blocks_) {
            child->print(out);
        }
        out << "}\n";
    }
    bool is_pure() { return pure == 1; }

    int pure = -1;
    int only_load_arg = -1;
    int has_side_effect = -1;
    std::map<std::pair<BasicBlock *, BasicBlock *>, double> branch_frequency;
    std::unordered_map<Temp *, std::string> global_temps;
protected:
    Type ret_type_;
    Module *parent_;
    std::vector<DataMeta*> arguments_;
    std::vector<Temp*> arg_temp;
    std::vector<BasicBlock*> basic_blocks_;
    std::vector<Instruction*> instructions_;
    int temp_used_;
    int basicblock_used_;
};

class LibFunction: public Function{
public:
    explicit LibFunction() = default;
    LibFunction(Module *parent, Type ret_type, const std::vector<DataMeta*> &arguments, const std::string & name) : 
        Function(parent, ret_type, arguments, name) {}
};

namespace instruction{

class Assign : public Instruction {
public:
    explicit Assign() = default;
    Assign(Temp* dst, Temp* src) : 
        Instruction(IRInstrKind::IRInstr_SEQ, std::vector<Temp*>{dst}, std::vector<Temp*>{src}) {}
    inline std::string to_str() { return dsts_[0]->to_str() + " = " + srcs_[0]->to_str(); }
    inline std::string to_llvm_str(int &llvm_temp_cnt) {
        if (dsts_[0]->get_type() == Int) {
            return dsts_[0]->to_llvm_str() + " = add " + type_llvm_string(dsts_[0]->get_type()) + " 0, " + srcs_[0]->to_llvm_str();
        } else {
            return dsts_[0]->to_llvm_str() + " = fadd " + type_llvm_string(dsts_[0]->get_type()) + " 0.0, " + srcs_[0]->to_llvm_str();
        }
    }
    inline void setsrc(Temp * src) { srcs_[0] = src; }
    inline Temp* getdst() const { return dsts_[0]; }
    inline Temp* getsrc() const { return srcs_[0]; }
    bool is_output_inst() { return true; }
};

class LoadImm4 : public Instruction {
public:
    explicit LoadImm4() = default;
    LoadImm4(Temp* dst, ConstValue imm) : 
        Instruction(IRInstrKind::IRInstr_SEQ, std::vector<Temp*>{dst}, std::vector<Temp*>{}), imm_(imm) {}
    inline std::string to_str() { return dsts_[0]->to_str() + " = " + imm_.to_string(); }
    inline std::string to_llvm_str(int &llvm_temp_cnt) {
        if (dsts_[0]->get_type() == Int) {
            return dsts_[0]->to_llvm_str() + " = add " + type_llvm_string(dsts_[0]->get_type()) + " 0, " + imm_.to_string();
        } else {
            return dsts_[0]->to_llvm_str() + " = fadd " + type_llvm_string(dsts_[0]->get_type()) + " 0.0, " + imm_.to_string();
        }
    }
    inline Temp* getdst() const { return dsts_[0]; }
    inline ConstValue getimm() const { return imm_; }
    void setimm(int i) { imm_ = i; }
    bool is_output_inst() { return true; }
protected:
    ConstValue imm_;
};

class Unary : public Instruction {
public:
    // enum IRInstrUnary {
    //     IR_Instr_NEG,
    //     IR_Instr_BITNOT,
    //     IR_Instr_LOGICNOT
    // };
    explicit Unary() = default;
    Unary(UnaryOp type, Temp* dst, Temp* src) : 
        Instruction(IRInstrKind::IRInstr_SEQ, std::vector<Temp*>{dst}, std::vector<Temp*>{src}), type_(type) {}
    inline std::string to_str() { 
        if (type_ == UnaryOp::Add) return dsts_[0]->to_str() + " = +" + srcs_[0]->to_str();
        if (type_ == UnaryOp::Sub) return dsts_[0]->to_str() + " = -" + srcs_[0]->to_str();
        if (type_ == UnaryOp::Not) return dsts_[0]->to_str() + " = !" + srcs_[0]->to_str();
        assert(false);
    }
    inline std::string to_llvm_str(int &llvm_temp_cnt) {
        if (dsts_[0]->get_type() == Int) {
            if (type_ == UnaryOp::Add) return dsts_[0]->to_llvm_str() + " = add " + type_llvm_string(srcs_[0]->get_type()) + " 0, " + srcs_[0]->to_llvm_str();
            if (type_ == UnaryOp::Sub) return dsts_[0]->to_llvm_str() + " = sub " + type_llvm_string(srcs_[0]->get_type()) + " 0, " + srcs_[0]->to_llvm_str();
            
            std::string ret = "";
            if (type_ == UnaryOp::Not) ret += "%TT" + std::to_string(llvm_temp_cnt ++) + " = icmp eq " + type_llvm_string(srcs_[0]->get_type()) + " 0, " + srcs_[0]->to_llvm_str();
            return ret + "\n" + dsts_[0]->to_llvm_str() + " = zext i1 %TT" + std::to_string(llvm_temp_cnt - 1) + " to i32";
        } else {
            if (type_ == UnaryOp::Add) return dsts_[0]->to_llvm_str() + " = fadd " + type_llvm_string(srcs_[0]->get_type()) + " 0, " + srcs_[0]->to_llvm_str();
            if (type_ == UnaryOp::Sub) return dsts_[0]->to_llvm_str() + " = fsub " + type_llvm_string(srcs_[0]->get_type()) + " 0, " + srcs_[0]->to_llvm_str();
            
            std::string ret = "";
            if (type_ == UnaryOp::Not) ret += "%TT" + std::to_string(llvm_temp_cnt ++) + " = fcmp eq " + type_llvm_string(srcs_[0]->get_type()) + " 0, " + srcs_[0]->to_llvm_str();
            return ret + "\n" + dsts_[0]->to_llvm_str() + " = zext i1 %TT" + std::to_string(llvm_temp_cnt - 1) + " to i32";
        }
        assert(false);
    }
    inline std::string op_str() {
        if (type_ == UnaryOp::Add) return " + ";
        if (type_ == UnaryOp::Sub) return " - ";
        if (type_ == UnaryOp::Not) return " ! ";
        assert(false);
    }
    inline ConstValue to_const(ConstValue src) {
        if (type_ == UnaryOp::Add) return src;
        if (type_ == UnaryOp::Sub) return src.getNeg();
        if (type_ == UnaryOp::Not) return src.getNot();
        assert(false);
    }
    inline void setsrc(Temp* src) { srcs_[0] = src; }
    inline Temp* getdst() const { return dsts_[0]; }
    inline Temp* getsrc() const { return srcs_[0]; }
    inline UnaryOp get_type() const { return type_; }
    bool is_output_inst() { return true; }
protected:
    UnaryOp type_;
};

class Binary : public Instruction {
public:
    explicit Binary() = default;
    Binary(BinaryOp type, Temp* dst, Temp* lhs, Temp* rhs) : 
        Instruction(IRInstrKind::IRInstr_SEQ, std::vector<Temp*>{dst}, std::vector<Temp*>{lhs, rhs}), type_(type){}
    inline std::string to_str() { 
        if (type_ == BinaryOp::And) return dsts_[0]->to_str() + " = " + srcs_[0]->to_str() + " & " + srcs_[1]->to_str();
        if (type_ == BinaryOp::Or ) return dsts_[0]->to_str() + " = " + srcs_[0]->to_str() + " | " + srcs_[1]->to_str();

        if (type_ == BinaryOp::Add) return dsts_[0]->to_str() + " = " + srcs_[0]->to_str() + " + " + srcs_[1]->to_str();
        if (type_ == BinaryOp::Sub) return dsts_[0]->to_str() + " = " + srcs_[0]->to_str() + " - " + srcs_[1]->to_str();
        if (type_ == BinaryOp::Mul) return dsts_[0]->to_str() + " = " + srcs_[0]->to_str() + " * " + srcs_[1]->to_str();
        if (type_ == BinaryOp::Div) return dsts_[0]->to_str() + " = " + srcs_[0]->to_str() + " / " + srcs_[1]->to_str();
        if (type_ == BinaryOp::Mod) return dsts_[0]->to_str() + " = " + srcs_[0]->to_str() + " % " + srcs_[1]->to_str();
        
        if (type_ == BinaryOp::Eq) return dsts_[0]->to_str() + " = " + srcs_[0]->to_str() + " == " + srcs_[1]->to_str();
        if (type_ == BinaryOp::Neq) return dsts_[0]->to_str() + " = " + srcs_[0]->to_str() + " != " + srcs_[1]->to_str();
        if (type_ == BinaryOp::Lt) return dsts_[0]->to_str() + " = " + srcs_[0]->to_str() + " < " + srcs_[1]->to_str();
        if (type_ == BinaryOp::Leq) return dsts_[0]->to_str() + " = " + srcs_[0]->to_str() + " <= " + srcs_[1]->to_str();
        if (type_ == BinaryOp::Gt) return dsts_[0]->to_str() + " = " + srcs_[0]->to_str() + " > " + srcs_[1]->to_str();
        if (type_ == BinaryOp::Geq) return dsts_[0]->to_str() + " = " + srcs_[0]->to_str() + " >= " + srcs_[1]->to_str();
        if (type_ == BinaryOp::Shr) return dsts_[0]->to_str() + " = " + srcs_[0]->to_str() + " >> " + srcs_[1]->to_str();
        if (type_ == BinaryOp::Shl) return dsts_[0]->to_str() + " = " + srcs_[0]->to_str() + " << " + srcs_[1]->to_str();
        assert(false);
    }
    inline std::string to_llvm_str(int &llvm_temp_cnt) {
        if (dsts_[0]->get_type() == Int) {
            if (type_ == BinaryOp::And) return dsts_[0]->to_llvm_str() + " = and " + type_llvm_string(dsts_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + srcs_[1]->to_llvm_str();
            if (type_ == BinaryOp::Or ) return dsts_[0]->to_llvm_str() + " = or " + type_llvm_string(dsts_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + srcs_[1]->to_llvm_str();

            if (type_ == BinaryOp::Add) return dsts_[0]->to_llvm_str() + " = add " + type_llvm_string(dsts_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + srcs_[1]->to_llvm_str();
            if (type_ == BinaryOp::Sub) return dsts_[0]->to_llvm_str() + " = sub " + type_llvm_string(dsts_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + srcs_[1]->to_llvm_str();
            if (type_ == BinaryOp::Mul) return dsts_[0]->to_llvm_str() + " = mul " + type_llvm_string(dsts_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + srcs_[1]->to_llvm_str();
            if (type_ == BinaryOp::Div) return dsts_[0]->to_llvm_str() + " = sdiv " + type_llvm_string(dsts_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + srcs_[1]->to_llvm_str();
            if (type_ == BinaryOp::Mod) return dsts_[0]->to_llvm_str() + " = srem " + type_llvm_string(dsts_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + srcs_[1]->to_llvm_str();

            if (type_ == BinaryOp::Shr) return dsts_[0]->to_llvm_str() + " = ashr " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + srcs_[1]->to_llvm_str();
            if (type_ == BinaryOp::Shl) return dsts_[0]->to_llvm_str() + " = shl " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + srcs_[1]->to_llvm_str();

            std::string ret = "";
            if (type_ == BinaryOp::Eq) ret += "%TT" + std::to_string(llvm_temp_cnt ++) + " = icmp eq " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + srcs_[1]->to_llvm_str();
            if (type_ == BinaryOp::Neq) ret += "%TT" + std::to_string(llvm_temp_cnt ++) + " = icmp ne " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + srcs_[1]->to_llvm_str();
            if (type_ == BinaryOp::Lt) ret += "%TT" + std::to_string(llvm_temp_cnt ++) + " = icmp slt " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + srcs_[1]->to_llvm_str();
            if (type_ == BinaryOp::Leq) ret += "%TT" + std::to_string(llvm_temp_cnt ++) + " = icmp sle " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + srcs_[1]->to_llvm_str();
            if (type_ == BinaryOp::Gt) ret += "%TT" + std::to_string(llvm_temp_cnt ++) + " = icmp sgt " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + srcs_[1]->to_llvm_str();
            if (type_ == BinaryOp::Geq) ret += "%TT" + std::to_string(llvm_temp_cnt ++) + " = icmp sge " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + srcs_[1]->to_llvm_str();
            return ret + "\n" + dsts_[0]->to_llvm_str() + " = zext i1 %TT" + std::to_string(llvm_temp_cnt - 1) + " to i32";
        } else {
            if (type_ == BinaryOp::And) return dsts_[0]->to_llvm_str() + " = and " + type_llvm_string(dsts_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + srcs_[1]->to_llvm_str();
            if (type_ == BinaryOp::Or ) return dsts_[0]->to_llvm_str() + " = or " + type_llvm_string(dsts_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + srcs_[1]->to_llvm_str();

            if (type_ == BinaryOp::Add) return dsts_[0]->to_llvm_str() + " = fadd " + type_llvm_string(dsts_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + srcs_[1]->to_llvm_str();
            if (type_ == BinaryOp::Sub) return dsts_[0]->to_llvm_str() + " = fsub " + type_llvm_string(dsts_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + srcs_[1]->to_llvm_str();
            if (type_ == BinaryOp::Mul) return dsts_[0]->to_llvm_str() + " = fmul " + type_llvm_string(dsts_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + srcs_[1]->to_llvm_str();
            if (type_ == BinaryOp::Div) return dsts_[0]->to_llvm_str() + " = fdiv " + type_llvm_string(dsts_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + srcs_[1]->to_llvm_str();
            if (type_ == BinaryOp::Mod) return dsts_[0]->to_llvm_str() + " = frem " + type_llvm_string(dsts_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + srcs_[1]->to_llvm_str();

            if (type_ == BinaryOp::Shr) return dsts_[0]->to_llvm_str() + " = ashr " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + srcs_[1]->to_llvm_str();
            if (type_ == BinaryOp::Shl) return dsts_[0]->to_llvm_str() + " = shl " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + srcs_[1]->to_llvm_str();

            std::string ret = "";
            if (type_ == BinaryOp::Eq) ret += "%TT" + std::to_string(llvm_temp_cnt ++) + " = fcmp eq " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + srcs_[1]->to_llvm_str();
            if (type_ == BinaryOp::Neq) ret += "%TT" + std::to_string(llvm_temp_cnt ++) + " = fcmp ne " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + srcs_[1]->to_llvm_str();
            if (type_ == BinaryOp::Lt) ret += "%TT" + std::to_string(llvm_temp_cnt ++) + " = fcmp slt " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + srcs_[1]->to_llvm_str();
            if (type_ == BinaryOp::Leq) ret += "%TT" + std::to_string(llvm_temp_cnt ++) + " = fcmp sle " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + srcs_[1]->to_llvm_str();
            if (type_ == BinaryOp::Gt) ret += "%TT" + std::to_string(llvm_temp_cnt ++) + " = fcmp sgt " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + srcs_[1]->to_llvm_str();
            if (type_ == BinaryOp::Geq) ret += "%TT" + std::to_string(llvm_temp_cnt ++) + " = fcmp sge " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + srcs_[1]->to_llvm_str();
            return ret + "\n" + dsts_[0]->to_llvm_str() + " = zext i1 %TT" + std::to_string(llvm_temp_cnt - 1) + " to i32";
        }
    }
    inline std::string op_str() { 
        if (type_ == BinaryOp::And) return " & " ;
        if (type_ == BinaryOp::Or ) return " | " ;

        if (type_ == BinaryOp::Add) return " + " ;
        if (type_ == BinaryOp::Sub) return " - " ;
        if (type_ == BinaryOp::Mul) return " * " ;
        if (type_ == BinaryOp::Div) return " / " ;
        if (type_ == BinaryOp::Mod) return " % " ;

        if (type_ == BinaryOp::Eq)  return " == ";
        if (type_ == BinaryOp::Neq) return " != ";
        if (type_ == BinaryOp::Lt)  return " < " ;
        if (type_ == BinaryOp::Leq) return " <= ";
        if (type_ == BinaryOp::Gt)  return " > " ;
        if (type_ == BinaryOp::Geq) return " >= ";
        if (type_ == BinaryOp::Shr) return " >> ";
        if (type_ == BinaryOp::Shl) return " << ";
        assert(false);
    }
    inline ConstValue to_const(ConstValue lhs, ConstValue rhs) {
        if (lhs.type == Int && rhs.type == Int) {
            if (type_ == BinaryOp::And) return ConstValue(lhs.iv & rhs.iv);
            if (type_ == BinaryOp::Or ) return ConstValue(lhs.iv | rhs.iv);
            if (type_ == BinaryOp::Add) return ConstValue(lhs.iv + rhs.iv);
            if (type_ == BinaryOp::Sub) return ConstValue(lhs.iv - rhs.iv);
            if (type_ == BinaryOp::Mul) return ConstValue(lhs.iv * rhs.iv);
            if (type_ == BinaryOp::Div) return ConstValue(lhs.iv / rhs.iv);
            if (type_ == BinaryOp::Mod) return ConstValue(lhs.iv % rhs.iv);

            if (type_ == BinaryOp::Lt)  return ConstValue(lhs.iv < rhs.iv);
            if (type_ == BinaryOp::Leq) return ConstValue(lhs.iv <= rhs.iv);
            if (type_ == BinaryOp::Gt)  return ConstValue(lhs.iv > rhs.iv);
            if (type_ == BinaryOp::Geq) return ConstValue(lhs.iv >= rhs.iv);
            if (type_ == BinaryOp::Shr) return ConstValue(lhs.iv >> rhs.iv);
            if (type_ == BinaryOp::Shl) return ConstValue(lhs.iv << rhs.iv);
        } else if (lhs.type == Float && rhs.type == Int) {
            if (type_ == BinaryOp::And) return ConstValue((int)lhs.fv & rhs.iv);
            if (type_ == BinaryOp::Or ) return ConstValue((int)lhs.fv | rhs.iv);
            if (type_ == BinaryOp::Add) return ConstValue(lhs.fv + rhs.iv);
            if (type_ == BinaryOp::Sub) return ConstValue(lhs.fv - rhs.iv);
            if (type_ == BinaryOp::Mul) return ConstValue(lhs.fv * rhs.iv);
            if (type_ == BinaryOp::Div) return ConstValue(lhs.fv / rhs.iv);
            if (type_ == BinaryOp::Mod) return ConstValue((int)lhs.fv % rhs.iv);

            if (type_ == BinaryOp::Lt)  return ConstValue(lhs.fv < rhs.iv);
            if (type_ == BinaryOp::Leq) return ConstValue(lhs.fv <= rhs.iv);
            if (type_ == BinaryOp::Gt)  return ConstValue(lhs.fv > rhs.iv);
            if (type_ == BinaryOp::Geq) return ConstValue(lhs.fv >= rhs.iv);
            if (type_ == BinaryOp::Shr) return ConstValue((int)lhs.fv >> rhs.iv);
            if (type_ == BinaryOp::Shl) return ConstValue((int)lhs.fv << rhs.iv);
        } else if (lhs.type == Int && rhs.type == Float) {
            if (type_ == BinaryOp::And) return ConstValue(lhs.iv & (int)rhs.fv);
            if (type_ == BinaryOp::Or ) return ConstValue(lhs.iv | (int)rhs.fv);
            if (type_ == BinaryOp::Add) return ConstValue(lhs.iv + rhs.fv);
            if (type_ == BinaryOp::Sub) return ConstValue(lhs.iv - rhs.fv);
            if (type_ == BinaryOp::Mul) return ConstValue(lhs.iv * rhs.fv);
            if (type_ == BinaryOp::Div) return ConstValue(lhs.iv / rhs.fv);
            if (type_ == BinaryOp::Mod) return ConstValue(lhs.iv % (int)rhs.fv);

            if (type_ == BinaryOp::Lt)  return ConstValue(lhs.iv < rhs.fv);
            if (type_ == BinaryOp::Leq) return ConstValue(lhs.iv <= rhs.fv);
            if (type_ == BinaryOp::Gt)  return ConstValue(lhs.iv > rhs.fv);
            if (type_ == BinaryOp::Geq) return ConstValue(lhs.iv >= rhs.fv);
            if (type_ == BinaryOp::Shr) return ConstValue(lhs.iv >> (int)rhs.fv);
            if (type_ == BinaryOp::Shl) return ConstValue(lhs.iv << (int)rhs.fv);
        } else if (lhs.type == Float && rhs.type == Float) {
            if (type_ == BinaryOp::And) return ConstValue((int)lhs.fv & (int)rhs.fv);
            if (type_ == BinaryOp::Or ) return ConstValue((int)lhs.fv | (int)rhs.fv);
            if (type_ == BinaryOp::Add) return ConstValue(lhs.fv + rhs.fv);
            if (type_ == BinaryOp::Sub) return ConstValue(lhs.fv - rhs.fv);
            if (type_ == BinaryOp::Mul) return ConstValue(lhs.fv * rhs.fv);
            if (type_ == BinaryOp::Div) return ConstValue(lhs.fv / rhs.fv);
            if (type_ == BinaryOp::Mod) return ConstValue((int)lhs.fv % (int)rhs.fv);

            if (type_ == BinaryOp::Lt)  return ConstValue(lhs.fv < rhs.fv);
            if (type_ == BinaryOp::Leq) return ConstValue(lhs.fv <= rhs.fv);
            if (type_ == BinaryOp::Gt)  return ConstValue(lhs.fv > rhs.fv);
            if (type_ == BinaryOp::Geq) return ConstValue(lhs.fv >= rhs.fv);
            if (type_ == BinaryOp::Shr) return ConstValue((int)lhs.fv >> (int)rhs.fv);
            if (type_ == BinaryOp::Shl) return ConstValue((int)lhs.fv << (int)rhs.fv);
        }

        if (type_ == BinaryOp::Eq) return lhs == rhs;
        if (type_ == BinaryOp::Neq) return lhs != rhs;

        assert(false);
        // if (type_ == BinaryOp::And) return ConstValue((int)GetValue(lhs) & (int)GetValue(rhs));
        // if (type_ == BinaryOp::Or ) return ConstValue((int)GetValue(lhs) | (int)GetValue(rhs));

        // if (lhs.type == Float || rhs.type == Float) {
        //     if (type_ == BinaryOp::Add) return ConstValue(GetValue(lhs) + GetValue(rhs));
        //     if (type_ == BinaryOp::Sub) return ConstValue(GetValue(lhs) - GetValue(rhs));
        //     if (type_ == BinaryOp::Mul) return ConstValue(GetValue(lhs) * GetValue(rhs));
        //     if (type_ == BinaryOp::Div) return ConstValue(GetValue(lhs) / GetValue(rhs));
        // }
        // else {
        //     if (type_ == BinaryOp::Add) return ConstValue((int)GetValue(lhs) + (int)GetValue(rhs));
        //     if (type_ == BinaryOp::Sub) return ConstValue((int)GetValue(lhs) - (int)GetValue(rhs));
        //     if (type_ == BinaryOp::Mul) return ConstValue((int)GetValue(lhs) * (int)GetValue(rhs));
        //     if (type_ == BinaryOp::Div) return ConstValue((int)GetValue(lhs) / (int)GetValue(rhs));
        // }
        // if (type_ == BinaryOp::Mod) return ConstValue((int)GetValue(lhs) % (int)GetValue(rhs));
        
        // if (type_ == BinaryOp::Lt) return ConstValue(GetValue(lhs) < GetValue(rhs));
        // if (type_ == BinaryOp::Leq) return ConstValue(GetValue(lhs) <= GetValue(rhs));
        // if (type_ == BinaryOp::Gt) return ConstValue(GetValue(lhs) > GetValue(rhs));
        // if (type_ == BinaryOp::Geq) return ConstValue(GetValue(lhs) >= GetValue(rhs));
        // if (type_ == BinaryOp::Shr) return ConstValue((int)GetValue(lhs) >> (int)GetValue(rhs));
        // if (type_ == BinaryOp::Shl) return ConstValue((int)GetValue(lhs) << (int)GetValue(rhs));
    }
    inline void setlhs(Temp* lhs) { srcs_[0] = lhs; }
    inline void setrhs(Temp* rhs) { srcs_[1] = rhs; }
    inline void settype(BinaryOp type) { type_ = type; }
    inline void swap_lhs_rhs() { std::swap(srcs_[0], srcs_[1]); }
    inline Temp* getdst() const { return dsts_[0]; }
    inline Temp* getlhs() const { return srcs_[0]; }
    inline Temp* getrhs() const { return srcs_[1]; }
    inline BinaryOp get_type() const { return type_; }
    inline BinaryOp get_swap_type() const {
        if (type_ == BinaryOp::Lt) return BinaryOp::Gt;
        if (type_ == BinaryOp::Leq) return BinaryOp::Geq;
        if (type_ == BinaryOp::Gt) return BinaryOp::Lt;
        if (type_ == BinaryOp::Geq) return BinaryOp::Leq;
        return type_;
    }
    bool is_output_inst() { return true; }
protected:
    BinaryOp type_;
};

class BinaryImm : public Instruction {
public:
    explicit BinaryImm() = default;
    BinaryImm(BinaryOp type, Temp* dst, Temp* lhs, ConstValue rhs) : 
        Instruction(IRInstrKind::IRInstr_SEQ, std::vector<Temp*>{dst}, std::vector<Temp*>{lhs}), type_(type), imm_(rhs.iv){}
    inline std::string to_str() { 
        if (type_ == BinaryOp::And) return dsts_[0]->to_str() + " = " + srcs_[0]->to_str() + " & " + std::to_string(imm_);
        if (type_ == BinaryOp::Or ) return dsts_[0]->to_str() + " = " + srcs_[0]->to_str() + " | " + std::to_string(imm_);

        if (type_ == BinaryOp::Add) return dsts_[0]->to_str() + " = " + srcs_[0]->to_str() + " + " + std::to_string(imm_);
        if (type_ == BinaryOp::Sub) return dsts_[0]->to_str() + " = " + srcs_[0]->to_str() + " - " + std::to_string(imm_);
        if (type_ == BinaryOp::Mul) return dsts_[0]->to_str() + " = " + srcs_[0]->to_str() + " * " + std::to_string(imm_);
        if (type_ == BinaryOp::Div) return dsts_[0]->to_str() + " = " + srcs_[0]->to_str() + " / " + std::to_string(imm_);
        if (type_ == BinaryOp::Mod) return dsts_[0]->to_str() + " = " + srcs_[0]->to_str() + " % " + std::to_string(imm_);
        
        if (type_ == BinaryOp::Eq) return dsts_[0]->to_str() + " = " + srcs_[0]->to_str() + " == " + std::to_string(imm_);
        if (type_ == BinaryOp::Neq) return dsts_[0]->to_str() + " = " + srcs_[0]->to_str() + " != " + std::to_string(imm_);
        if (type_ == BinaryOp::Lt) return dsts_[0]->to_str() + " = " + srcs_[0]->to_str() + " < " + std::to_string(imm_);
        if (type_ == BinaryOp::Leq) return dsts_[0]->to_str() + " = " + srcs_[0]->to_str() + " <= " + std::to_string(imm_);
        if (type_ == BinaryOp::Gt) return dsts_[0]->to_str() + " = " + srcs_[0]->to_str() + " > " + std::to_string(imm_);
        if (type_ == BinaryOp::Geq) return dsts_[0]->to_str() + " = " + srcs_[0]->to_str() + " >= " + std::to_string(imm_);
        if (type_ == BinaryOp::Shr) return dsts_[0]->to_str() + " = " + srcs_[0]->to_str() + " >> " + std::to_string(imm_);
        if (type_ == BinaryOp::Shl) return dsts_[0]->to_str() + " = " + srcs_[0]->to_str() + " << " + std::to_string(imm_);
        return "error";
    }
    inline std::string to_llvm_str(int &llvm_temp_cnt) {
        if (dsts_[0]->get_type() == Int) {
            if (type_ == BinaryOp::And) return dsts_[0]->to_llvm_str() + " = and " + type_llvm_string(dsts_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + std::to_string(imm_);
            if (type_ == BinaryOp::Or ) return dsts_[0]->to_llvm_str() + " = or " + type_llvm_string(dsts_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + std::to_string(imm_);

            if (type_ == BinaryOp::Add) return dsts_[0]->to_llvm_str() + " = add " + type_llvm_string(dsts_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + std::to_string(imm_);
            if (type_ == BinaryOp::Sub) return dsts_[0]->to_llvm_str() + " = sub " + type_llvm_string(dsts_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + std::to_string(imm_);
            if (type_ == BinaryOp::Mul) return dsts_[0]->to_llvm_str() + " = mul " + type_llvm_string(dsts_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + std::to_string(imm_);
            if (type_ == BinaryOp::Div) return dsts_[0]->to_llvm_str() + " = sdiv " + type_llvm_string(dsts_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + std::to_string(imm_);
            if (type_ == BinaryOp::Mod) return dsts_[0]->to_llvm_str() + " = srem " + type_llvm_string(dsts_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + std::to_string(imm_ + 1); // TODO: 根据imm是不是mask进行调整

            if (type_ == BinaryOp::Shr) return dsts_[0]->to_llvm_str() + " = ashr " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + std::to_string(imm_);
            if (type_ == BinaryOp::Shl) return dsts_[0]->to_llvm_str() + " = shl " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + std::to_string(imm_);

            std::string ret = "";
            if (type_ == BinaryOp::Eq) ret += "%TT" + std::to_string(llvm_temp_cnt ++) + " = icmp eq " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + std::to_string(imm_);
            if (type_ == BinaryOp::Neq) ret += "%TT" + std::to_string(llvm_temp_cnt ++) + " = icmp ne " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + std::to_string(imm_);
            if (type_ == BinaryOp::Lt) ret += "%TT" + std::to_string(llvm_temp_cnt ++) + " = icmp slt " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + std::to_string(imm_);
            if (type_ == BinaryOp::Leq) ret += "%TT" + std::to_string(llvm_temp_cnt ++) + " = icmp sle " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + std::to_string(imm_);
            if (type_ == BinaryOp::Gt) ret += "%TT" + std::to_string(llvm_temp_cnt ++) + " = icmp sgt " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + std::to_string(imm_);
            if (type_ == BinaryOp::Geq) ret += "%TT" + std::to_string(llvm_temp_cnt ++) + " = icmp sge " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + std::to_string(imm_);
            return ret + "\n" + dsts_[0]->to_llvm_str() + " = zext i1 %TT" + std::to_string(llvm_temp_cnt - 1) + " to i32";
        } else {
            if (type_ == BinaryOp::And) return dsts_[0]->to_llvm_str() + " = and " + type_llvm_string(dsts_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + std::to_string(imm_);
            if (type_ == BinaryOp::Or ) return dsts_[0]->to_llvm_str() + " = or " + type_llvm_string(dsts_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + std::to_string(imm_);

            if (type_ == BinaryOp::Add) return dsts_[0]->to_llvm_str() + " = fadd " + type_llvm_string(dsts_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + std::to_string(imm_);
            if (type_ == BinaryOp::Sub) return dsts_[0]->to_llvm_str() + " = fsub " + type_llvm_string(dsts_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + std::to_string(imm_);
            if (type_ == BinaryOp::Mul) return dsts_[0]->to_llvm_str() + " = fmul " + type_llvm_string(dsts_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + std::to_string(imm_);
            if (type_ == BinaryOp::Div) return dsts_[0]->to_llvm_str() + " = fdiv " + type_llvm_string(dsts_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + std::to_string(imm_);
            if (type_ == BinaryOp::Mod) return dsts_[0]->to_llvm_str() + " = frem " + type_llvm_string(dsts_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + std::to_string(imm_ + 1); // TODO: 根据imm是不是mask进行调整

            if (type_ == BinaryOp::Shr) return dsts_[0]->to_llvm_str() + " = ashr " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + std::to_string(imm_);
            if (type_ == BinaryOp::Shl) return dsts_[0]->to_llvm_str() + " = shl " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + std::to_string(imm_);

            std::string ret = "";
            if (type_ == BinaryOp::Eq) ret += "%TT" + std::to_string(llvm_temp_cnt ++) + " = fcmp eq " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + std::to_string(imm_);
            if (type_ == BinaryOp::Neq) ret += "%TT" + std::to_string(llvm_temp_cnt ++) + " = fcmp ne " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + std::to_string(imm_);
            if (type_ == BinaryOp::Lt) ret += "%TT" + std::to_string(llvm_temp_cnt ++) + " = fcmp slt " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + std::to_string(imm_);
            if (type_ == BinaryOp::Leq) ret += "%TT" + std::to_string(llvm_temp_cnt ++) + " = fcmp sle " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + std::to_string(imm_);
            if (type_ == BinaryOp::Gt) ret += "%TT" + std::to_string(llvm_temp_cnt ++) + " = fcmp sgt " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + std::to_string(imm_);
            if (type_ == BinaryOp::Geq) ret += "%TT" + std::to_string(llvm_temp_cnt ++) + " = fcmp sge " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", " + std::to_string(imm_);
            return ret + "\n" + dsts_[0]->to_llvm_str() + " = zext i1 %TT" + std::to_string(llvm_temp_cnt - 1) + " to i32";
        }
    }
    inline std::string op_str() { 
        if (type_ == BinaryOp::And) return " & " ;
        if (type_ == BinaryOp::Or ) return " | " ;

        if (type_ == BinaryOp::Add) return " + " ;
        if (type_ == BinaryOp::Sub) return " - " ;
        if (type_ == BinaryOp::Mul) return " * " ;
        if (type_ == BinaryOp::Div) return " / " ;
        if (type_ == BinaryOp::Mod) return " % " ;

        if (type_ == BinaryOp::Eq)  return " == ";
        if (type_ == BinaryOp::Neq) return " != ";
        if (type_ == BinaryOp::Lt)  return " < " ;
        if (type_ == BinaryOp::Leq) return " <= ";
        if (type_ == BinaryOp::Gt)  return " > " ;
        if (type_ == BinaryOp::Geq) return " >= ";
        if (type_ == BinaryOp::Shr) return " >> ";
        if (type_ == BinaryOp::Shl) return " << ";
        assert(false);
    }
    inline ConstValue to_const(ConstValue lhs) {
        if (lhs.type == Int) {
            if (type_ == BinaryOp::And) return ConstValue(lhs.iv & imm_);
            if (type_ == BinaryOp::Or ) return ConstValue(lhs.iv | imm_);
            if (type_ == BinaryOp::Add) return ConstValue(lhs.iv + imm_);
            if (type_ == BinaryOp::Sub) return ConstValue(lhs.iv - imm_);
            if (type_ == BinaryOp::Mul) return ConstValue(lhs.iv * imm_);
            if (type_ == BinaryOp::Div) return ConstValue(lhs.iv / imm_);
            if (type_ == BinaryOp::Mod) return ConstValue(lhs.iv % imm_);

            if (type_ == BinaryOp::Lt)  return ConstValue(lhs.iv < imm_);
            if (type_ == BinaryOp::Leq) return ConstValue(lhs.iv <= imm_);
            if (type_ == BinaryOp::Gt)  return ConstValue(lhs.iv > imm_);
            if (type_ == BinaryOp::Geq) return ConstValue(lhs.iv >= imm_);
            if (type_ == BinaryOp::Shr) return ConstValue(lhs.iv >> imm_);
            if (type_ == BinaryOp::Shl) return ConstValue(lhs.iv << imm_);
        }

        if (type_ == BinaryOp::Eq) return lhs.iv == imm_;
        if (type_ == BinaryOp::Neq) return lhs.iv != imm_;

        assert(false);
    }
    inline void setlhs(Temp* lhs) { srcs_[0] = lhs; }
    inline Temp* getdst() const { return dsts_[0]; }
    inline Temp* getlhs() const { return srcs_[0]; }
    inline int32_t getimm() const { return imm_; }
    inline ConstValue getconst() const { return ConstValue(imm_); }
    inline BinaryOp get_type() const { return type_; }
    bool is_output_inst() { return true; }
protected:
    BinaryOp type_;
    int32_t imm_;
};

class Label : public Instruction {
public:
    explicit Label() = default;
    Label(const std::string & label) : 
        Instruction(IRInstrKind::IRInstr_LABEL, std::vector<Temp*>{}, std::vector<Temp*>{}), label_(label) {}
    inline std::string to_str() { return label_; }
    inline std::string get_label() const { return label_; }
    inline void set_label(const std::string & label) { label_ = label; }
    bool is_output_inst() { return false; }
protected:
    std::string label_;
};

class Call : public Instruction {
public:
    explicit Call() = default;
    Call(Temp* dst, const std::vector<Temp*> & srcs, Function* func) : 
        Instruction(IRInstrKind::IRInstr_CALL, std::vector<Temp*>{dst}, srcs), func_(func) {}
    inline Temp* getdst() const { return dsts_[0]; }
    inline Function* getfunc() const { return func_; }
    inline std::vector<Temp *> get_srcs() const { return srcs_; }
    inline std::string to_str() { 
        std::string ret = "";
        if(dsts_[0] != nullptr)ret = dsts_[0]->to_str() + " = ";
        ret += "call " + func_->get_name() + "(";
        for (int i = 0; i < srcs_.size(); ++i) {
            ret += srcs_[i]->to_str();
            if (i != srcs_.size() - 1) ret += ", ";
        }
        ret += ")";
        return ret;
    }
    inline std::string to_llvm_str(int &llvm_temp_cnt) {
        std::string func_name = func_->get_name();
        if (func_name == "starttime") {
            func_name = "_sysy_starttime";
        } else if (func_name == "stoptime") {
            func_name = "_sysy_stoptime";
        }
        std::string ret = "";
        if(dsts_[0] != nullptr)ret = dsts_[0]->to_llvm_str() + " = ";
        ret += "call " + type_llvm_string(func_->get_ret_type()) + " @" + func_name + "(";
        for (int i = 0; i < srcs_.size(); ++i) {
            ret += type_llvm_string(srcs_[i]->get_type()) + " " + srcs_[i]->to_llvm_str();
            if (i != srcs_.size() - 1) ret += ", ";
        }
        ret += ")";
        return ret;
    }
    bool is_output_inst() { return (dsts_[0] != nullptr); }
    void add_global_load(Temp* temp) { global_load_.push_back(temp); }
    void clear_global_load() { global_load_.clear(); }
    std::vector<Temp*>* get_global_load() { return &global_load_; }
    std::string get_func_name() { return func_->get_name(); }
protected:
    Function* func_;
    std::vector<Temp*> global_load_;
};

class Branch : public Instruction {
public:
    explicit Branch() = default;
    Branch(BasicBlock* bb) : 
        Instruction(IRInstrKind::IRInstr_JMP, std::vector<Temp*>{}, std::vector<Temp*>{}), bb_(bb) {}
    inline std::string to_str() { return "jump " + bb_->get_name(); }
    inline std::string to_llvm_str(int &llvm_temp_cnt) { return "br label %B" + std::to_string(bb_->get_index()); }
    inline BasicBlock* get_bb() const { return bb_; }
    inline void set_target(BasicBlock* bb) { bb_ = bb; }
    bool is_output_inst() { return false; }
protected:
    BasicBlock* bb_;
};

class CondBranch : public Instruction {
public:
    enum IRInstrCondBranch {
        IR_Instr_BEQ,
        IR_Instr_BNE,
        IR_Instr_BEQ_ONLY,
    };
    explicit CondBranch() = default;
    CondBranch(IRInstrCondBranch type, BasicBlock* true_bb, BasicBlock* false_bb, Temp* cond) : 
        Instruction(IRInstrKind::IRInstr_JMP_COND, std::vector<Temp*>{}, std::vector<Temp*>{cond}), type_(type), true_bb_(true_bb), false_bb_(false_bb) {}
    inline std::string to_str() { return "if " + srcs_[0]->to_str() + " " + (type_ != IR_Instr_BNE ? "==" : "!=") + " 0 jump " + false_bb_->get_name() + " else jump " + true_bb_->get_name(); }
    inline std::string to_llvm_str(int &llvm_temp_cnt) {
        std::string ret = "%TT" + std::to_string(llvm_temp_cnt ++) + " = icmp ne " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", 0\n";
        if (type_ == IR_Instr_BEQ) return ret + "br i1 %TT" + std::to_string(llvm_temp_cnt -1) + ", label %B" + std::to_string(true_bb_->get_index()) + ", label %B" + std::to_string(false_bb_->get_index());
        if (type_ == IR_Instr_BNE) return ret + "br i1 %TT" + std::to_string(llvm_temp_cnt -1) + ", label %B" + std::to_string(false_bb_->get_index()) + ", label %B" + std::to_string(true_bb_->get_index());
    }
    inline Temp* getcond() const { return srcs_[0]; }
    void setcond(Temp* src) { srcs_ = {src}; }
    inline BasicBlock* get_true_bb() const { return true_bb_; }
    inline BasicBlock* get_false_bb() const { return false_bb_; }
    inline IRInstrCondBranch get_type() const { return type_; }
    inline void set_true_bb(BasicBlock* bb) { true_bb_ = bb; }
    inline void set_false_bb(BasicBlock* bb) { false_bb_ = bb; }
    bool is_output_inst() { return false; }
protected:
    IRInstrCondBranch type_;
    BasicBlock* true_bb_;
    BasicBlock* false_bb_;
};

class Return : public Instruction {
public:
    explicit Return() = default;
    Return(Temp* value) : 
        Instruction(IRInstrKind::IRInstr_RET, std::vector<Temp*>{}, std::vector<Temp*>{value}) {}
    inline std::string to_str() {
        if(srcs_[0] == nullptr)return "return";
        return "return " + srcs_[0]->to_str();
    }
    inline std::string to_llvm_str(int &llvm_temp_cnt) {
        if(srcs_[0] == nullptr)return "ret void";
        return "ret " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str();
    }
    inline bool has_return_value() const { return srcs_[0] != nullptr; }
    inline Temp* getvalue() const { return srcs_[0]; }
    inline void setvalue(Temp* value) { srcs_[0] = value; }
    bool is_output_inst() { return false; }
};

class Alloca : public Instruction {
public:
    explicit Alloca() = default;
    Alloca(Temp* addr, Type type) : 
        Instruction(IRInstrKind::IRInstr_SEQ, std::vector<Temp*>{addr}, std::vector<Temp*>{}), type_(type) {}
    inline std::string to_str() { return "alloca " + dsts_[0]->to_str() + " = " + std::to_string(type_.size()); }
    inline std::string to_llvm_str(int &llvm_temp_cnt) { return dsts_[0]->to_llvm_str() + " = alloca " + type_llvm_string(type_) + ", i32 " + std::to_string(type_.size()); }
    inline int get_size() { return type_.size(); }
    inline Temp* getaddr() const { return dsts_[0]; }
    inline Type gettype() const { return type_; }
    bool is_output_inst() { return true; }
protected:
    Type type_;
};

extern int &llvm_temp_cnt;

class Store : public Instruction {
public:
    explicit Store() = default;
    Store(Temp* addr, Temp* src, bool not_delete, int offset = 0) : 
        Instruction(IRInstrKind::IRInstr_SEQ, std::vector<Temp*>{}, std::vector<Temp*>{addr, src}), offset_(offset), not_delete_(not_delete) {}
    inline std::string to_str() { return "store *(" + srcs_[0]->to_str() + " + " + std::to_string(offset_) + ") = " + srcs_[1]->to_str(); }
    inline std::string to_llvm_str(int &llvm_temp_cnt) {
        if (offset_ == 0) return "store " + type_llvm_string(srcs_[1]->get_type()) + " " + srcs_[1]->to_llvm_str() + ", " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str();
        auto ret = "%TT" + std::to_string(llvm_temp_cnt) + " = getelementptr " + type_llvm_string(srcs_[0]->get_type(), true) + ", " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", i32 " + std::to_string(offset_ / 4);
        return ret + "\nstore " + type_llvm_string(srcs_[1]->get_type()) + " " + srcs_[1]->to_llvm_str() + ", " + type_llvm_string(srcs_[0]->get_type()) + " %TT" + std::to_string(llvm_temp_cnt++);
    }
    inline Temp* getsrc() const { return srcs_[1]; }
    inline Temp* getaddr() const { return srcs_[0]; }
    inline void setsrc(Temp* src) { srcs_[1] = src; }
    inline void setaddr(Temp* addr) { srcs_[0] = addr; }
    inline void setimm(int imm_) { offset_ = imm_; }
    inline int getoffset() const { return offset_; }
    inline bool not_delete() const { return not_delete_; }
    inline void set_not_delete(bool not_delete) { not_delete_ = not_delete; }
    bool is_output_inst() { return false; }
protected:
    int offset_;
    bool not_delete_;
};

class ArrayStore : public Instruction {
public:
    explicit ArrayStore() = default;
    ArrayStore(Temp* dst, Temp* dep, Temp* addr, Temp* src, int offset, std::unordered_set<Temp*> &used, bool after_call = false) : 
        Instruction(IRInstrKind::IRInstr_SEQ, std::vector<Temp*>{dst}, std::vector<Temp*>{dep, addr, src}), offset_(offset), used_(used), after_call_(after_call) {}
    inline std::string to_str() {
        std::string ret = "arraystore dst " + dsts_[0]->to_str() + " dep " + srcs_[0]->to_str() + " to " + srcs_[1]->to_str() + "[" + std::to_string(offset_) + "] from " + srcs_[2]->to_str();
        ret += " used: ";
        for (auto temp : used_) {
            ret += temp->to_str() + " ";
        }
        return ret;
    }
    inline std::string to_llvm_str(int &llvm_temp_cnt) {
        return "arraystore not supported";
    }
    inline Temp* getdst() const { return dsts_[0]; }
    inline Temp* getdep() const { return srcs_[0]; }
    inline Temp* getaddr() const { return srcs_[1]; }
    inline Temp* getsrc() const { return srcs_[2]; }
    inline int getoffset() const { return offset_; }
    inline void setaddr(Temp* addr) { srcs_[1] = addr; }
    inline void setsrc(Temp* src) { srcs_[2] = src; }
    inline void setoffset(int index) { offset_ = index; }
    bool is_output_inst() { return false; }
    bool after_call() { return after_call_; }
    std::unordered_set<Temp*>* get_used() { return &used_; }
private:
    int offset_;
    bool after_call_;
    std::unordered_set<Temp*> used_;
};

class Load : public Instruction {
public:
    explicit Load() = default;
    Load(Temp* dst, Temp* addr, bool not_delete, int offset = 0) : 
        Instruction(IRInstrKind::IRInstr_SEQ, std::vector<Temp*>{dst}, std::vector<Temp*>{addr}), offset_(offset), not_delete_(not_delete) {}
    inline std::string to_str() { return "load " + dsts_[0]->to_str() + " = *(" + srcs_[0]->to_str() + " + " + std::to_string(offset_) + ")"; }
    inline std::string to_llvm_str(int &llvm_temp_cnt) {
        if(offset_ == 0) return dsts_[0]->to_llvm_str() + " = load " + type_llvm_string(dsts_[0]->get_type()) + ", " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str();
        auto ret = "%TT" + std::to_string(llvm_temp_cnt) + " = getelementptr " + type_llvm_string(srcs_[0]->get_type(), true) + ", " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", i32 " + std::to_string(offset_ / 4);
        return ret + "\n" + dsts_[0]->to_llvm_str() + " = load " + type_llvm_string(dsts_[0]->get_type()) + ", " + type_llvm_string(srcs_[0]->get_type()) + " %TT" + std::to_string(llvm_temp_cnt++);
    }
    inline Temp* getdst() const { return dsts_[0]; }
    inline Temp* getaddr() const { return srcs_[0]; }
    inline void setaddr(Temp* addr) { srcs_[0] = addr; }
    inline void setimm(int imm_) { offset_ = imm_; }
    inline int getoffset() const { return offset_; }
    inline bool not_delete() const { return not_delete_; }
    inline void set_not_delete(bool not_delete) { not_delete_ = not_delete; }
    bool is_output_inst() { return true; }
protected:
    int offset_;
    bool not_delete_;
};

class ArrayLoad : public Instruction {
public:
    explicit ArrayLoad() = default;
    ArrayLoad(Temp* dst, Temp* dep, Temp* addr, int offset, bool before_call = false) : 
        Instruction(IRInstrKind::IRInstr_SEQ, std::vector<Temp*>{dst}, std::vector<Temp*>{dep, addr}), offset_(offset), before_call_(before_call) {}
    inline std::string to_str() { return "arrayload " + dsts_[0]->to_str() + " = dep " + srcs_[0]->to_str() + " from " + srcs_[1]->to_str() + "[" + std::to_string(offset_) + "]"; }
    inline std::string to_llvm_str(int &llvm_temp_cnt) {
        return "arrayload not supported";
    }
    inline Temp* getdst() const { return dsts_[0]; }
    inline Temp* getdep() const { return srcs_[0]; }
    inline Temp* getaddr() const { return srcs_[1]; }
    inline int getoffset() const { return offset_; }
    inline void setdep(Temp* dep) { srcs_[0] = dep; }
    inline void setaddr(Temp* addr) { srcs_[1] = addr; }
    inline void setoffset(int index) { offset_ = index; }
    bool is_output_inst() { return true; }
    bool before_call() { return before_call_; }
private:
    int offset_;
    bool before_call_;
};

class LoadAddr : public Instruction {
public:
    explicit LoadAddr() = default;
    LoadAddr(Temp* addr, std::string name, Type type) : 
        Instruction(IRInstrKind::IRInstr_SEQ, std::vector<Temp*>{addr}, std::vector<Temp*>{}), name_(name), type_(type) {}
    inline std::string to_str() { return dsts_[0]->to_str() + " = &" + name_; }
    inline std::string to_llvm_str(int &llvm_temp_cnt) {
        if (dsts_[0]->get_type().is_array()) {
            if (dsts_[0]->get_type().base_type == Int) return dsts_[0]->to_llvm_str() + " = getelementptr [" + std::to_string(type_.nr_elems()) + " x i32], [" + std::to_string(type_.nr_elems()) + " x i32]* @"+ name_ + ", i32 0, i32 0";
            if (dsts_[0]->get_type().base_type == Float) return dsts_[0]->to_llvm_str() + " = getelementptr [" + std::to_string(type_.nr_elems()) + " x float], [" + std::to_string(type_.nr_elems()) + " x float]* @"+ name_ + ", i32 0, i32 0";
        } else {
            if (dsts_[0]->get_type().base_type == Int) return dsts_[0]->to_llvm_str() + " = getelementptr i32, i32* @"+ name_;
            if (dsts_[0]->get_type().base_type == Float) return dsts_[0]->to_llvm_str() + " = getelementptr float, float* @"+ name_;
        }
        return "";
    }
    inline Temp* get_addr() const {return dsts_[0]; }
    inline std::string get_name() const {return name_; }
    bool is_output_inst() { return true; }
    Type type_;
protected:
    std::string name_;
};

class ElementPtr : public Instruction {
public:
    explicit ElementPtr() = default;
    ElementPtr(Temp* dst, Temp* base, std::vector<Temp*> indices) : 
        Instruction(IRInstrKind::IRInstr_SEQ, std::vector<Temp*>{dst}, std::vector<Temp*>{}) {
            srcs_.push_back(base);
            srcs_.insert(srcs_.end(), indices.begin(), indices.end());
        }  
    inline std::string to_str() {
        std::string s = dsts_[0]->to_str() + " = elementptr: " + srcs_[0]->to_str();
        auto indices_ = std::vector<Temp*>(srcs_.begin() + 1, srcs_.end());
        for (auto i : indices_) {
            s += "[" + i->to_str() + "]";
        }
        return s;
    }
    inline std::string to_llvm_str(int &llvm_temp_cnt) {
        if (srcs_[0]->get_type().dims[0] == 0) {
            if (srcs_[0]->get_type().nr_dims() == srcs_.size() - 1) {
                std::string s = "";
                if (srcs_[0]->get_type().nr_dims() == srcs_.size() - 1) {
                    s += "%TT" + std::to_string(llvm_temp_cnt++) + " = getelementptr " + type_llvm_string(srcs_[0]->get_type(), true) + ", " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", i32 " + srcs_[srcs_.size() - 1]->to_llvm_str() + "\n";
                } else {
                    s += "%TT" + std::to_string(llvm_temp_cnt++) + " = getelementptr " + type_llvm_string(srcs_[0]->get_type(), true) + ", " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + "\n";
                }
                int block_size = srcs_[0]->get_type().dims[srcs_[0]->get_type().nr_dims() - 1];
                for (int i = srcs_[0]->get_type().nr_dims() - 1; i > 0; i--) {
                    if (i < srcs_.size() - 1) {
                        s += "%TT" + std::to_string(llvm_temp_cnt++) + " = mul i32 " + srcs_[i]->to_llvm_str() + ", " + std::to_string(block_size) + "\n";
                        s += "%TT" + std::to_string(llvm_temp_cnt) + " = getelementptr " + type_llvm_string(srcs_[0]->get_type(), true) + ", " + type_llvm_string(srcs_[0]->get_type()) + " %TT" + std::to_string(llvm_temp_cnt - 2) + ", i32 %TT" + std::to_string(llvm_temp_cnt - 1) + "\n";
                        llvm_temp_cnt ++;
                    }
                    block_size *= srcs_[0]->get_type().dims[i - 1];
                }
                s += dsts_[0]->to_llvm_str() + " = getelementptr " + type_llvm_string(srcs_[0]->get_type(), true) + ", " + type_llvm_string(srcs_[0]->get_type()) + " %TT" + std::to_string(llvm_temp_cnt - 1);
                return s;
            } else {
                std::string s = "";
                if (srcs_[0]->get_type().nr_dims() == srcs_.size()) {
                    s += "%TT" + std::to_string(llvm_temp_cnt++) + " = getelementptr " + type_llvm_string(srcs_[0]->get_type(), true) + ", " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + ", i32 " + srcs_[srcs_.size() - 1]->to_llvm_str() + "\n";
                } else {
                    s += "%TT" + std::to_string(llvm_temp_cnt++) + " = getelementptr " + type_llvm_string(srcs_[0]->get_type(), true) + ", " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + "\n";
                }
                int block_size = srcs_[0]->get_type().dims[srcs_[0]->get_type().nr_dims() - 1];
                for (int i = srcs_[0]->get_type().nr_dims() - 2; i > 0; i--) {
                    if (i < srcs_.size()) {
                        s += "%TT" + std::to_string(llvm_temp_cnt++) + " = mul i32 " + srcs_[i]->to_llvm_str() + ", " + std::to_string(block_size) + "\n";
                        s += "%TT" + std::to_string(llvm_temp_cnt) + " = getelementptr " + type_llvm_string(srcs_[0]->get_type(), true) + ", " + type_llvm_string(srcs_[0]->get_type()) + " %TT" + std::to_string(llvm_temp_cnt - 2) + ", i32 %TT" + std::to_string(llvm_temp_cnt - 1) + "\n";
                        llvm_temp_cnt ++;
                    }
                    block_size *= srcs_[0]->get_type().dims[i];
                }
                s += dsts_[0]->to_llvm_str() + " = getelementptr " + type_llvm_string(srcs_[0]->get_type(), true) + ", " + type_llvm_string(srcs_[0]->get_type()) + " %TT" + std::to_string(llvm_temp_cnt - 1);
                return s;
            }
        } else {
            assert(false);
        }
    }
    inline Temp* get_dst() const { return dsts_[0]; }
    inline Temp* get_base() const { return srcs_[0]; }
    inline void set_base(Temp* base) { srcs_[0] = base; }
    inline std::vector<Temp*> get_indices() const { return std::vector<Temp*>(srcs_.begin() + 1, srcs_.end()); }
    bool is_output_inst() { return true; }
protected:
};

class Phi : public Instruction {
public:
    explicit Phi() = default;
    Phi(Temp* dst, const std::vector<Temp*> & values, const std::vector<BasicBlock*> & bbs, bool need_change = false, bool is_arrayssa = false) : 
        Instruction(IRInstrKind::IRInstr_SEQ, std::vector<Temp*>{dst}, values), bbs_(bbs), need_change(need_change), is_arrayssa_(is_arrayssa) {}
    inline std::string to_str() { 
        std::string ret = dsts_[0]->to_str() + " = phi ";
        for (int i = 0; i < srcs_.size(); ++i) {
            ret += "[" + srcs_[i]->to_str() + ", " + bbs_[i]->get_name() + "]";
            if (i != srcs_.size() - 1) ret += ", ";
        }
        ret += " used_size: " + std::to_string(used_.size());
        ret += " used: ";
        for (auto temp : used_) {
            ret += temp->to_str() + " ";
        }
        return ret;
    }
    inline std::string to_llvm_str(int &llvm_temp_cnt) {
        std::string ret = dsts_[0]->to_llvm_str() + " = phi " + type_llvm_string(dsts_[0]->get_type()) + " ";
        for (int i = 0; i < srcs_.size(); ++i) {
            ret += "[ " + srcs_[i]->to_llvm_str() + ", %B" + std::to_string(bbs_[i]->get_index()) + " ]";
            if (i != srcs_.size() - 1) ret += ", ";
        }
        return ret;
    }
    inline void add_src_and_bb(BasicBlock* bb, Temp* src) {
        int bb_idx = 0;
        for (auto i: bbs_) {
            if (i == bb) {
                srcs_[bb_idx] = src;
                return;
            }
            bb_idx++;
        }
        srcs_.push_back(src);
        bbs_.push_back(bb);
    }
    inline void erase_src_temp(BasicBlock* bb) {
        int bb_idx = 0;
        for (auto i = bbs_.begin(); i != bbs_.end(); i++) {
            if (*i == bb) {
                bbs_.erase(i);
                break;
            }
            bb_idx++;
        }
        int src_idx = 0;
        for (auto i = srcs_.begin(); i != srcs_.end(); i++) {
            if (src_idx == bb_idx) {
                srcs_.erase(i);
                return;
            }
            src_idx++;
        }
    }
    inline int getsize() const { return srcs_.size(); }
    inline Temp* getdst() const { return dsts_[0]; }
    inline void setdst(Temp *temp) { dsts_[0] = temp;}
    inline std::vector<Temp*>* getvalues() { return &srcs_; }
    inline std::vector<std::pair<Temp*, BasicBlock*>> getpairs() { 
        std::vector<std::pair<Temp*, BasicBlock*>> pairs;
        int idx = 0;
        for (auto i = bbs_.begin(); i != bbs_.end(); i++) {
            pairs.push_back({srcs_[idx++], *i});
        }
        return pairs;
    }
    inline void setpairs(std::unordered_map<BasicBlock*, Temp*> pairs) { 
        bbs_.clear(); srcs_.clear();
        for (auto i: pairs) {
            bbs_.push_back(i.first);
            srcs_.push_back(i.second);
        }
    }
    inline std::vector<BasicBlock*>* getbbs() { return &bbs_; }
    inline Temp* get_src_temp(BasicBlock* bb) {
        int bb_idx = 0;
        for (auto i: bbs_) {
            if (i == bb) {
                return srcs_[bb_idx];
            }
            bb_idx++;
        }
        return nullptr;
    }
    inline void set_src_temp(BasicBlock* old_bb, BasicBlock* new_bb, Temp* temp) {
        int bb_idx = 0;
        for (auto &i: bbs_) {
            if (i == old_bb) {
                bbs_[bb_idx] = new_bb;
                srcs_[bb_idx] = temp;
            }
            bb_idx++;
        }
    }
    inline void add_pair(BasicBlock* bb, Temp* temp) {
        srcs_.push_back(temp);
        bbs_.push_back(bb);
    }
    bool is_output_inst() { return true; }
    inline void set_used(std::unordered_set<Temp*> used) { used_ = used; }
    inline std::unordered_set<Temp*>* get_used() { return &used_; }
    inline bool is_arrayssa() { return is_arrayssa_; }
    bool need_change;
protected:
    std::vector<BasicBlock*> bbs_;
    std::unordered_set<Temp*> used_;
    bool is_arrayssa_;
};

class Cast : public Instruction {
public:
    explicit Cast() = default;
    Cast(Temp* dst, Temp* src, Type type) : 
        Instruction(IRInstrKind::IRInstr_SEQ, std::vector<Temp*>{dst}, std::vector<Temp*>{src}), type_(type) {}
    inline std::string to_str() { return "cast " + dsts_[0]->to_str() + " = " + srcs_[0]->to_str(); }
    inline std::string to_llvm_str(int &llvm_temp_cnt) {
        if (type_.base_type == Int) return dsts_[0]->to_llvm_str() + " = fptosi " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + " to " + type_llvm_string(dsts_[0]->get_type());
        if (type_.base_type == Float) return dsts_[0]->to_llvm_str() + " = sitofp " + type_llvm_string(srcs_[0]->get_type()) + " " + srcs_[0]->to_llvm_str() + " to " + type_llvm_string(dsts_[0]->get_type());
        return "";
    }
    inline Temp* getdst() const { return dsts_[0]; }
    inline Temp* getsrc() const { return srcs_[0]; }
    inline Type gettype() const { return type_; }
    bool is_output_inst() { return true; }
protected:
    Type type_;
};

class Minimum : public Instruction {
public:
    explicit Minimum() = default;
    Minimum(Temp* dst, Temp* src1, Temp* src2) : 
        Instruction(IRInstrKind::IRInstr_SEQ, std::vector<Temp*>{dst}, std::vector<Temp*>{src1, src2}) {}
    inline std::string to_str() { return "minimum " + dsts_[0]->to_str() + " = min(" + srcs_[0]->to_str() + ", " + srcs_[1]->to_str() + ")"; }
    inline std::string to_llvm_str(int &llvm_temp_cnt) {
        return "not supported";
    }
    inline Temp* getdst() const { return dsts_[0]; }
    inline Temp* getsrc1() const { return srcs_[0]; }
    inline Temp* getsrc2() const { return srcs_[1]; }
    bool is_output_inst() { return true; }
};

class Maximum : public Instruction {
public:
    explicit Maximum() = default;
    Maximum(Temp* dst, Temp* src1, Temp* src2) : 
        Instruction(IRInstrKind::IRInstr_SEQ, std::vector<Temp*>{dst}, std::vector<Temp*>{src1, src2}) {}
    inline std::string to_str() { return "maximum " + dsts_[0]->to_str() + " = max(" + srcs_[0]->to_str() + ", " + srcs_[1]->to_str() + ")"; }
    inline std::string to_llvm_str(int &llvm_temp_cnt) {
        return "not supported";
    }
    inline Temp* getdst() const { return dsts_[0]; }
    inline Temp* getsrc1() const { return srcs_[0]; }
    inline Temp* getsrc2() const { return srcs_[1]; }
    bool is_output_inst() { return true; }
};
} // namespace instruction

/// @brief functions, global variables and a symboltable
class Module {
public:
    explicit Module() = default;
    Module(const std::vector<Function*> & functions, const std::vector<DataMeta*> global_variables) : 
        functions_(functions), global_variables_(global_variables) {
            if (!check_global_variables()) {
                printf("global variables are not global.\n");
                assert(false);
            }
        }
    inline std::vector<Function*> * get_functions() { return &functions_; }
    inline std::vector<DataMeta*> * get_global_variables() { return &global_variables_; }
    inline std::unordered_map<std::string, Function*> get_func_map() { return func_map; }
    inline void add_function(Function * function) {
        // func_map.emplace(function->get_name(), function);
        functions_.push_back(function);
    }
    inline void add_global_variable(DataMeta * global_variable) {
        global_variables_.push_back(global_variable);
        global_var_map.emplace(global_variable->get_name(), global_variable);
    }
    bool check_global_variables() {
        for (auto global_variable : global_variables_) {
            if (!global_variable->get_is_global()) {
                return false;
            }
        }
        return true;
    }
    void print(std::ostream &out) const {
        for (auto &global : global_variables_){
            if(global->get_data_type().is_array()){
                out << "init global " << type_string(global->get_data_type()) << ' ' << global->get_name() << " = {";
                // auto init_values = std::get<1>(global->get_init_value());
                // for (auto &init_value : init_values){
                //     if(init_value.type == Int)
                //         out << init_value.iv << ' ';
                //     else{
                //         out << init_value.fv << ' ';
                //     }
                // }
                out << "}" << std::endl;
            }
            else{
                if(std::get<0>(global->get_init_value()).type == Int)
                    out << "init global " << type_string(global->get_data_type()) << ' ' << global->get_name() << " with " << std::get<0>(global->get_init_value()).iv << std::endl;
                else{
                    out << "init global " << type_string(global->get_data_type()) << ' ' << global->get_name() << " with " << std::get<0>(global->get_init_value()).fv << std::endl;
                }
            }
        }
        for (auto &child : functions_) {
            child->print(out);
        }
    }
    virtual ~Module() {};
    std::unordered_map<std::string, Function*> func_map;
    std::unordered_map<std::string, LibFunction*> lib_funcs;
    std::unordered_map<std::string, DataMeta*> global_var_map;
    void set_use_memset_zero(bool use_memset_zero) { this->use_memset_zero = use_memset_zero; }
    bool get_use_memset_zero() const { return use_memset_zero; }
protected:
    std::vector<Function*> functions_;
    std::vector<DataMeta*> global_variables_;
    bool use_memset_zero = false;
};

/// @brief Valid data type : var, varptr, array, arrayptr
// enum DataType {
//     Var,
//     VarPtr,
//     Array,
//     ArrayPtr
// };

/// @brief variable type
// enum VarType {
//     Int,
//     Float,
//     Double,
//     Char,
//     Bool,
//     Void
// };


/// @brief a connection between Value and User
class Use {

};

/// @brief the common base class of IR that may refer to Values
class User : public Value {
protected:
    std::vector<Value*> operands_;
public:
    explicit User() = default;
    User(IRValueType type, const std::vector<Value*> & operands, const std::string & name = "") : Value(type, name), operands_(operands) {}
};

} // namespace ir

} // namespace middleend