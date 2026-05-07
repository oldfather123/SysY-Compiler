#pragma once

#include <cassert>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <algorithm>

class Type;
class IntegerType;
class ArrayType;
class PointerType;
class FunctionType;
class Value;
class Constant;
class ConstantInt;
class ConstantFloat;
class ConstantArray;
class ConstantZero;
class Module;
class GlobalVariable;
class Function;
class BasicBlock;
class Argument;
class Instruction;
class BinaryInst;
class UnaryInst;
class ICmpInst;
class FCmpInst;
class CallInst;
class BranchInst;
class ReturnInst;
class GetElementPtrInst;
class StoreInst;
class LoadInst;
class AllocaInst;

struct Use {
    Value* val_;
    unsigned int arg_no_;  
    Use(Value* val, unsigned int no) : val_(val), arg_no_(no) {}
};

class IRType {
public:
    enum IRTypeID {
        VoidTyID,LabelTyID,IntegerTyID,FloatTyID,FunctionTyID,ArrayTyID,PointerTyID,   
    };
    explicit IRType(IRTypeID tid) : tid_(tid) {}
    ~IRType() = default;
    virtual std::string print();
    IRTypeID tid_;
};

class IntegerIRType : public IRType {
public:
    explicit IntegerIRType(unsigned num_bits) : IRType(IRType::IntegerTyID), num_bits_(num_bits) {}
    unsigned num_bits_;
};

class ArrayIRType : public IRType { //数组类型
public:
    ArrayIRType(IRType* contained, unsigned num_elements) : IRType(IRType::ArrayTyID), num_elements_(num_elements), contained_(contained) {}
    IRType* contained_;        //该数组的IRType
    unsigned num_elements_;    //数组内有多少元素
};

class PointerIRType : public IRType { //指针类型
public:
    PointerIRType(IRType* contained) : IRType(IRType::PointerTyID), contained_(contained) {}
    IRType* contained_;  //该指针指向的元素的IRType
};


class FunctionIRType : public IRType { //函数类型声明
public:
    FunctionIRType(IRType* result, std::vector<IRType*> params) : IRType(IRType::FunctionTyID) {
        result_ = result;
        for (IRType* p : params) {
            args_.push_back(p);
        }
    }
    IRType* result_;
    std::vector<IRType*> args_;
};


class Value {
public:
    explicit Value(IRType* ty, const std::string& name = "") : IRType_(ty), name_(name) {}
    ~Value() = default;
    virtual std::string print() = 0;
    
    void remove_use(Value *val) {
        auto is_val = [val] (const Use &use) { return use.val_ == val; };
        use_list_.remove_if(is_val);
    }

    
    std::list<Use>::iterator add_use(Value* val, unsigned arg_no) { 
        use_list_.emplace_back(Use(val, arg_no)); 
        std::list<Use>::iterator re = use_list_.end();
        return --re;
    }
    void remove_use(std::list<Use>::iterator it) {
        use_list_.erase(it);
    }
    bool is_constant();

    IRType* IRType_;
    std::string name_;
    std::list<Use> use_list_;  
};


class Constant : public Value { //常量
public:
    Constant(IRType* ty, const std::string& name = "") : Value(ty, name) {}
    ~Constant() = default;
};


class ConstantInt : public Constant { //整型常量
public:
    ConstantInt(IRType* ty, int val) : Constant(ty, ""), value_(val) {}
    virtual std::string print() override;
    int value_;
};


class ConstantFloat : public Constant { //浮点型常量
public:
    ConstantFloat(IRType* ty, float val) : Constant(ty, ""), value_(val) {}
    virtual std::string print() override;
    float value_;
    std::string print32();
};


class ConstantArray : public Constant { //数组型常量
public:
    ConstantArray(ArrayIRType* ty, const std::vector<Constant*>& val) : Constant(ty, "") { this->const_array.assign(val.begin(), val.end()); }
    ~ConstantArray() = default;
    virtual std::string print() override;
    std::vector<Constant*> const_array;
};


class ConstantZero : public Constant { //常数零
public:
    ConstantZero(IRType* ty) : Constant(ty, "") {}
    virtual std::string print() override;
};


class Module {
public:
    explicit Module() {
        void_ty_ = new IRType(IRType::VoidTyID);
        label_ty_ = new IRType(IRType::LabelTyID);
        int1_ty_ = new IntegerIRType(1);
        int32_ty_ = new IntegerIRType(32);
        float32_ty_ = new IRType(IRType::FloatTyID);
    }
    ~Module() {
        delete void_ty_;
        delete label_ty_;
        delete int1_ty_;
        delete int32_ty_;
        delete float32_ty_;
    }
    virtual std::string print();
    void add_global_variable(GlobalVariable* g) { global_list_.push_back(g); }
    void add_function(::Function* f) { function_list_.push_back(f); }
    PointerIRType* get_pointer_IRType(IRType* contained) {
        if (!pointer_map_.count(contained)) {
            pointer_map_[contained] = new PointerIRType(contained);
        }
        return pointer_map_[contained];
    }
    ArrayIRType* get_array_IRType(IRType* contained, unsigned num_elements) {
        if (!array_map_.count({contained, num_elements})) {
            array_map_[{contained, num_elements}] = new ArrayIRType(contained, num_elements);
        }
        return array_map_[{contained, num_elements}];
    }

    ::Function *getMainFunc();

    std::vector<GlobalVariable*> global_list_;
    std::vector<::Function*> function_list_;

    IntegerIRType* int1_ty_;
    IntegerIRType* int32_ty_;
    IRType* float32_ty_;
    IRType* label_ty_;
    IRType* void_ty_;
    std::map<IRType*, PointerIRType*> pointer_map_;
    std::map<std::pair<IRType*, int>, ArrayIRType*> array_map_;
};



class GlobalVariable : public Value {
public:
    GlobalVariable(std::string name, Module* m, IRType* ty, bool is_const, Constant* init = nullptr) : Value(m->get_pointer_IRType(ty), name), is_const_(is_const), init_val_(init) { m->add_global_variable(this); }
    virtual std::string print() override;
    bool is_const_; //是否为常量
    Constant* init_val_;    //初始化
};




class Argument : public Value {
public:
    explicit Argument(IRType* ty, const std::string& name = "", ::Function* f = nullptr, unsigned arg_no = 0) : Value(ty, name), parent_(f), arg_no_(arg_no) {}
    ~Argument() {}
    virtual std::string print() override;
    ::Function* parent_;
    unsigned arg_no_;  
};

class Function : public Value {
public:
    Function(FunctionIRType* ty, const std::string& name, Module* parent) : Value(ty, name), parent_(parent), seq_cnt_(0) {
        parent->add_function(this);
        size_t num_args = ty->args_.size();
        use_ret_cnt = 0;
        for (size_t i = 0; i < num_args; i++) {
            arguments_.push_back(new Argument(ty->args_[i], "", this, i));
        }
    }
    ~Function();
    virtual std::string print() override;
    void add_basic_block(BasicBlock* bb) { basic_blocks_.push_back(bb); }
    IRType* get_return_IRType() const { return static_cast<FunctionIRType*>(IRType_)->result_; }
    bool is_declaration() { return basic_blocks_.empty(); }
    void set_instr_name();
    void remove_bb(BasicBlock* bb);
    BasicBlock *getRetBB();

    std::vector<BasicBlock*> basic_blocks_;  
    std::vector<Argument*> arguments_;       
    Module* parent_;
    unsigned seq_cnt_;
    std::vector<std::set<Value *>> vreg_set_;
    int use_ret_cnt;        
};


class BasicBlock : public Value {
public:
    explicit BasicBlock(Module* m, const std::string& name, ::Function* parent) : Value(m->label_ty_, name), parent_(parent) { parent_->add_basic_block(this); }
    bool add_instruction(Instruction* instr) ; 
    bool add_instruction_front(Instruction* instr) ;
    bool add_instruction_before_terminator(Instruction* instr) ;
    bool add_instruction_before_inst(Instruction* new_inst,Instruction* inst) ;
    void add_pre_basic_block(BasicBlock* bb) { pre_bbs_.push_back(bb); }
    void add_succ_basic_block(BasicBlock* bb) { succ_bbs_.push_back(bb); }
    void remove_pre_basic_block(BasicBlock *bb) { 
        
        pre_bbs_.erase(std::remove(pre_bbs_.begin(), pre_bbs_.end(), bb) , pre_bbs_.end());
    }
    void remove_succ_basic_block(BasicBlock *bb) { 
        
        succ_bbs_.erase(std::remove(succ_bbs_.begin(), succ_bbs_.end(), bb) , succ_bbs_.end());
    }
    int isDominate( BasicBlock* bb2) { 
        if (!bb2 || this->parent_ != bb2->parent_)
            return -1;
        while(bb2->name_ != "label_entry") {
            if(bb2->idom_ == this)
                return 1;
            bb2 = bb2->idom_;
        }
        return 0;
    }    

    Instruction* get_terminator();
    bool delete_instr(Instruction *instr); 
    bool remove_instr(Instruction *instr); 
    virtual std::string print() override;

    std::list<Instruction*> instr_list_; //指令集

    ::Function* parent_; 

    std::vector<BasicBlock*> pre_bbs_;
    std::vector<BasicBlock*> succ_bbs_;

    std::set<BasicBlock*> dom_frontier_;
    std::set<BasicBlock*> rdom_frontier_;
    std::set<BasicBlock*> rdoms_;
    BasicBlock* idom_;
    std::set<Value*> live_in;
    std::set<Value*> live_out;
};

class Instruction : public Value {
public:
    enum OpID {
        Ret = 11,Br = 12,FNeg = 13,Add = 14,Sub = 15,Mul = 16,SDiv = 17,SRem = 18,UDiv = 19,URem = 20,FAdd = 21,
        FSub = 22, FMul = 23,FDiv = 24,Shl = 25,LShr = 26,AShr = 27,And = 28,Or = 29,Xor = 30,Alloca = 31,Load = 32,
        Store = 33,GetElementPtr = 34,ZExt = 35,FPtoSI = 36,SItoFP = 37,BitCast = 38,ICmp = 39,FCmp = 40,PHI = 41,Call = 42,
    };

    Instruction(IRType* ty, OpID id, unsigned num_ops, BasicBlock* parent) : Value(ty, ""), op_id_(id), num_ops_(num_ops), parent_(parent) {
        operands_.resize(num_ops_, nullptr);  
        use_pos_.resize(num_ops_);
        parent_->add_instruction(this);
    }
    
    Instruction(IRType* ty, OpID id, unsigned num_ops) : Value(ty, ""), op_id_(id), num_ops_(num_ops), parent_(nullptr) {
        operands_.resize(num_ops_, nullptr);
        use_pos_.resize(num_ops_);
    }
    Value* get_operand(unsigned i) const { return operands_[i]; }

    void set_operand(unsigned i, Value* v) {
        operands_[i] = v;
        use_pos_[i] = v->add_use(this, i);
    }
    void add_operand(Value *v) {    
        operands_.push_back(v);  
        use_pos_.emplace_back(v->add_use(this, num_ops_));
        num_ops_++;
    }
    void remove_use_of_ops() { 
        for (int i = 0; i < operands_.size(); i ++ ) {
            operands_[i]->remove_use(use_pos_[i]);
        }
    }

    void remove_operands(int index1, int index2) {
        for(int i = index1; i <= index2; i ++ ) {
            operands_[i]->remove_use(use_pos_[i]);
        }

        for (int i = index2 + 1; i < operands_.size(); i++) {
            for (auto &use : operands_[i]->use_list_) {
                if (use.val_ == this) {
                    use.arg_no_ -= index2 - index1 + 1;
                    break;
                }
            }
        }        
        operands_.erase(operands_.begin() + index1, operands_.begin() + index2 + 1);
        use_pos_.erase(use_pos_.begin() + index1, use_pos_.begin() + index2 + 1);
        num_ops_ = operands_.size();
    }

    bool is_phi()  { return op_id_ == PHI; }
    bool is_store() { return op_id_ == Store; }
    bool is_alloca() { return op_id_ == Alloca; }
    bool is_ret() { return op_id_ == Ret; }
    bool is_load() { return op_id_ == Load; }
    bool is_br() { return op_id_ == Br; }

    bool is_add() { return op_id_ == Add; }
    bool is_sub() { return op_id_ == Sub; }
    bool is_mul() { return op_id_ == Mul; }
    bool is_div() { return op_id_ == SDiv; }
    bool is_rem() { return op_id_ == SRem; }

    bool is_fadd() { return op_id_ == FAdd; }
    bool is_fsub() { return op_id_ == FSub; }
    bool is_fmul() { return op_id_ == FMul; }
    bool is_fdiv() { return op_id_ == FDiv; }

    bool is_cmp() { return op_id_ == ICmp; }
    bool is_fcmp() { return op_id_ == FCmp; }

    bool is_call() { return op_id_ == Call; }
    bool is_gep() { return op_id_ == GetElementPtr; }
    bool is_zext() { return op_id_ ==  ZExt; }
    bool is_fptosi() { return op_id_ == FPtoSI; }
    bool is_sitofp() { return op_id_ == SItoFP; }

    bool is_int_binary()
    {
        return (is_add() || is_sub() || is_mul() || is_div() || is_rem()) &&
               (num_ops_ == 2);
    }

    bool is_float_binary()
    {
        return (is_fadd() || is_fsub() || is_fmul() || is_fdiv()) &&
               (num_ops_ == 2);
    }

    bool is_binary(){ return is_int_binary() || is_float_binary();}

    bool isTerminator() { return is_br() || is_ret(); }


    
    virtual std::string print() = 0;
    BasicBlock* parent_;
    OpID op_id_;
    unsigned num_ops_;
    std::vector<Value*> operands_;  // 当前值的操作数
    std::vector<std::list<Use>::iterator> use_pos_; // 与操作数数组一一对应，是对应的操作数的uselist里面，与当前指令相关的use的迭代器
    std::vector<std::list<Instruction*>::iterator> pos_in_bb;// 在bb的指令list的位置迭代器,最多只能有一个
};


class BinaryInst : public Instruction {
public:
    BinaryInst(IRType* ty, OpID op, Value* v1, Value* v2, BasicBlock* bb) : Instruction(ty, op, 2, bb) {
        set_operand(0, v1);
        set_operand(1, v2);
    }
    
    BinaryInst(IRType* ty, OpID op, Value* v1, Value* v2, BasicBlock* bb, bool flag)
     : Instruction(ty, op, 2) {
        set_operand(0, v1);
        set_operand(1, v2);
        this->parent_ = bb;
    }
    virtual std::string print() override;
};


class UnaryInst : public Instruction {
public:
    UnaryInst(IRType* ty, OpID op, Value* val, BasicBlock* bb) : Instruction(ty, op, 1, bb) { set_operand(0, val); }
    virtual std::string print() override;
};

class ICmpInst : public Instruction {
public:
    enum ICmpOp {
        ICMP_EQ = 32,   //等于
        ICMP_NE = 33,   //不等于
        ICMP_UGT = 34,  //大于
        ICMP_UGE = 35,  //大于等于
        ICMP_ULT = 36,  //小于
        ICMP_ULE = 37,  //小于等于
        ICMP_SGT = 38,  //大于
        ICMP_SGE = 39,  //大于等于
        ICMP_SLT = 40,  //小于
        ICMP_SLE = 41   //小于等于
    };
    static const std::map<ICmpInst::ICmpOp, std::string> ICmpOpName;
    ICmpInst(ICmpOp op, Value* v1, Value* v2, BasicBlock* bb) : Instruction(bb->parent_->parent_->int1_ty_, Instruction::ICmp, 2, bb), icmp_op_(op) {
        set_operand(0, v1);
        set_operand(1, v2);
    }
    virtual std::string print() override;
    ICmpOp icmp_op_;
};

class FCmpInst : public Instruction {
public:
    enum FCmpOp {
        FCMP_FALSE = 10,FCMP_OEQ = 11,FCMP_OGT = 12,FCMP_OGE = 13,FCMP_OLT = 14,FCMP_OLE = 15,FCMP_ONE = 16,FCMP_ORD = 17,FCMP_UNO = 18,   
        FCMP_UEQ = 19,FCMP_UGT = 20,FCMP_UGE = 21,FCMP_ULT = 22,FCMP_ULE = 23,FCMP_UNE = 24,FCMP_TRUE = 25    
    };
    static const std::map<FCmpInst::FCmpOp, std::string> FCmpOpName;
    FCmpInst(FCmpOp op, Value* v1, Value* v2, BasicBlock* bb) : Instruction(bb->parent_->parent_->int1_ty_, Instruction::FCmp, 2, bb), fcmp_op_(op) {
        set_operand(0, v1);
        set_operand(1, v2);
    }
    virtual std::string print() override;
    FCmpOp fcmp_op_;
};


class CallInst : public Instruction {
public:
    CallInst(::Function* func, std::vector<Value*> args, BasicBlock* bb) : Instruction(static_cast<FunctionIRType*>(func->IRType_)->result_, Instruction::Call, args.size() + 1, bb) {
        int num_ops = args.size() + 1;
        for (int i = 0; i < num_ops - 1; i++) {
            set_operand(i, args[i]);
        }
        set_operand(num_ops - 1, func);
    }
    virtual std::string print() override;
};


class BranchInst : public Instruction {
public:

    BranchInst(Value* cond, BasicBlock* if_true, BasicBlock* if_false, BasicBlock* bb) : Instruction(if_true->parent_->parent_->void_ty_, Instruction::Br, 3, bb) {
        if_true->add_pre_basic_block(bb);
        if_false->add_pre_basic_block(bb);
        bb->add_succ_basic_block(if_false);
        bb->add_succ_basic_block(if_true);
        set_operand(0, cond);
        set_operand(1, if_true);
        set_operand(2, if_false);
    }

    BranchInst(BasicBlock* if_true, BasicBlock* bb) : Instruction(if_true->parent_->parent_->void_ty_, Instruction::Br, 1, bb) {
        if_true->add_pre_basic_block(bb);
        bb->add_succ_basic_block(if_true);
        set_operand(0, if_true);
    }
    virtual std::string print() override;
};


class ReturnInst : public Instruction {
public:
    ReturnInst(Value* val, BasicBlock* bb) : Instruction(bb->parent_->parent_->void_ty_, Instruction::Ret, 1, bb) { set_operand(0, val); }
    ReturnInst(Value* val, BasicBlock* bb, bool flag)
     : Instruction(bb->parent_->parent_->void_ty_, Instruction::Ret, 1) { 
        set_operand(0, val);
        this->parent_ = bb;
    }
    ReturnInst(BasicBlock* bb) : Instruction(bb->parent_->parent_->void_ty_, Instruction::Ret, 0, bb) {}
    virtual std::string print() override;
};


class GetElementPtrInst : public Instruction {  //IR中读取数组某一个元素需要的函数
public:
    GetElementPtrInst(Value* ptr, std::vector<Value*> idxs, BasicBlock* bb) :
        Instruction(bb->parent_->parent_->get_pointer_IRType(get_GEP_return_IRType(ptr, idxs.size())),
                Instruction::GetElementPtr, idxs.size() + 1, bb) {
        set_operand(0, ptr);
        for (size_t i = 0; i < idxs.size(); i++) {
            set_operand(i + 1, idxs[i]);
        }
    }
    IRType* get_GEP_return_IRType(Value* ptr, size_t idxs_size) {
        IRType* ty = static_cast<PointerIRType*>(ptr->IRType_)->contained_;  
        if (ty->tid_ == IRType::ArrayTyID) {
            ArrayIRType* arr_ty = static_cast<ArrayIRType*>(ty);
            for (size_t i = 1; i < idxs_size; i++) {
                ty = arr_ty->contained_;  
                if (ty->tid_ == IRType::ArrayTyID) {
                    arr_ty = static_cast<ArrayIRType*>(ty);
                }
            }
        }
        return ty;
    }
    virtual std::string print() override;
};


class StoreInst : public Instruction {
public:
    StoreInst(Value* val, Value* ptr, BasicBlock* bb) : Instruction(bb->parent_->parent_->void_ty_, Instruction::Store, 2, bb) {
        assert(val->IRType_ == static_cast<PointerIRType*>(ptr->IRType_)->contained_);
        set_operand(0, val);
        set_operand(1, ptr);
    }


    StoreInst(Value* val, Value* ptr, BasicBlock* bb, bool) : Instruction(bb->parent_->parent_->void_ty_, Instruction::Store, 2) {
        assert(val->IRType_ == static_cast<PointerIRType*>(ptr->IRType_)->contained_);
        set_operand(0, val);
        set_operand(1, ptr);
        this->parent_ = bb;
    }

    virtual std::string print() override;
};


class LoadInst : public Instruction {
public:
    LoadInst(Value* ptr, BasicBlock* bb) : Instruction(static_cast<PointerIRType*>(ptr->IRType_)->contained_, Instruction::Load, 1, bb) { set_operand(0, ptr); }
    virtual std::string print() override;
};


class AllocaInst : public Instruction {
public:
    AllocaInst(IRType* ty, BasicBlock* bb) : Instruction(bb->parent_->parent_->get_pointer_IRType(ty), Instruction::Alloca, 0, bb), alloca_ty_(ty) {}

    AllocaInst(IRType* ty, BasicBlock* bb, bool) : Instruction(bb->parent_->parent_->get_pointer_IRType(ty), Instruction::Alloca, 0), alloca_ty_(ty) {this->parent_ = bb;}

    virtual std::string print() override;
    IRType* alloca_ty_;
};

class ZextInst : public Instruction {
public:
    ZextInst(OpID op, Value* val, IRType* ty, BasicBlock* bb) : Instruction(ty, op, 1, bb), ZextInstTy(ty) { set_operand(0, val); }
    virtual std::string print() override;
    IRType* ZextInstTy;
};

class FpToSiInst : public Instruction {
public:
    FpToSiInst(OpID op, Value* val, IRType* ty, BasicBlock* bb) : Instruction(ty, op, 1, bb), FpToSiInstTy(ty) { set_operand(0, val); }
    virtual std::string print() override;
    IRType* FpToSiInstTy;
};

class SiToFpInst : public Instruction {
public:
    SiToFpInst(OpID op, Value* val, IRType* ty, BasicBlock* bb) : Instruction(ty, op, 1, bb), SiToFpInstTy(ty) { set_operand(0, val); }
    virtual std::string print() override;
    IRType* SiToFpInstTy;
};


class Bitcast : public Instruction {
public:
    Bitcast(OpID op, Value *val, IRType *ty, BasicBlock *bb) : Instruction(ty, op, 1, bb), BitcastTy(ty) {set_operand(0, val);}
    virtual std::string print() override;
    IRType *BitcastTy;
};


class PhiInst : public Instruction {
public:
    PhiInst(OpID op, std::vector<Value *> vals, std::vector<BasicBlock *> val_bbs, IRType *ty, BasicBlock *bb)
    : Instruction(ty, op, 2*vals.size()) {
        for ( int i = 0; i < vals.size(); i++) {
            set_operand(2*i, vals[i]);
            set_operand(2*i+1, val_bbs[i]);
        }
        this->parent_ = bb;
    }
    static PhiInst *create_phi(IRType *ty, BasicBlock *bb) {
        std::vector<Value *> vals;
        std::vector<BasicBlock *> val_bbs;
        return new PhiInst(Instruction::PHI, vals, val_bbs, ty, bb);
    }
    void add_phi_pair_operand(Value *val, Value *pre_bb) {
        this->add_operand(val);
        this->add_operand(pre_bb);
    }
    virtual std::string print() override;

    Value *l_val_;

};

class IRStmtBuilder {
public:
    BasicBlock* BB_;
    Module* m_;

    IRStmtBuilder(BasicBlock* bb, Module* m) : BB_(bb), m_(m){};
    ~IRStmtBuilder() = default;
    Module* get_module() { return m_; }
    BasicBlock* get_insert_block() { return this->BB_; }
    void set_insert_point(BasicBlock* bb) { this->BB_ = bb; }                                                                          
    BinaryInst* create_iadd(Value* v1, Value* v2) { return new BinaryInst(this->m_->int32_ty_, Instruction::Add, v1, v2, this->BB_); }  
    BinaryInst* create_isub(Value* v1, Value* v2) { return new BinaryInst(this->m_->int32_ty_, Instruction::Sub, v1, v2, this->BB_); }
    BinaryInst* create_imul(Value* v1, Value* v2) { return new BinaryInst(this->m_->int32_ty_, Instruction::Mul, v1, v2, this->BB_); }
    BinaryInst* create_isdiv(Value* v1, Value* v2) { return new BinaryInst(this->m_->int32_ty_, Instruction::SDiv, v1, v2, this->BB_); }
    BinaryInst* create_isrem(Value* v1, Value* v2) { return new BinaryInst(this->m_->int32_ty_, Instruction::SRem, v1, v2, this->BB_); }

    ICmpInst* create_icmp_eq(Value* v1, Value* v2) { return new ICmpInst(ICmpInst::ICMP_EQ, v1, v2, this->BB_); }
    ICmpInst* create_icmp_ne(Value* v1, Value* v2) { return new ICmpInst(ICmpInst::ICMP_NE, v1, v2, this->BB_); }
    ICmpInst* create_icmp_gt(Value* v1, Value* v2) { return new ICmpInst(ICmpInst::ICMP_SGT, v1, v2, this->BB_); }
    ICmpInst* create_icmp_ge(Value* v1, Value* v2) { return new ICmpInst(ICmpInst::ICMP_SGE, v1, v2, this->BB_); }
    ICmpInst* create_icmp_lt(Value* v1, Value* v2) { return new ICmpInst(ICmpInst::ICMP_SLT, v1, v2, this->BB_); }
    ICmpInst* create_icmp_le(Value* v1, Value* v2) { return new ICmpInst(ICmpInst::ICMP_SLE, v1, v2, this->BB_); }

    BinaryInst* create_fadd(Value* v1, Value* v2) { return new BinaryInst(this->m_->float32_ty_, Instruction::FAdd, v1, v2, this->BB_); }
    BinaryInst* create_fsub(Value* v1, Value* v2) { return new BinaryInst(this->m_->float32_ty_, Instruction::FSub, v1, v2, this->BB_); }
    BinaryInst* create_fmul(Value* v1, Value* v2) { return new BinaryInst(this->m_->float32_ty_, Instruction::FMul, v1, v2, this->BB_); }
    BinaryInst* create_fdiv(Value* v1, Value* v2) { return new BinaryInst(this->m_->float32_ty_, Instruction::FDiv, v1, v2, this->BB_); }

    FCmpInst* create_fcmp_eq(Value* v1, Value* v2) { return new FCmpInst(FCmpInst::FCMP_UEQ, v1, v2, this->BB_); }
    FCmpInst* create_fcmp_ne(Value* v1, Value* v2) { return new FCmpInst(FCmpInst::FCMP_UNE, v1, v2, this->BB_); }
    FCmpInst* create_fcmp_gt(Value* v1, Value* v2) { return new FCmpInst(FCmpInst::FCMP_UGT, v1, v2, this->BB_); }
    FCmpInst* create_fcmp_ge(Value* v1, Value* v2) { return new FCmpInst(FCmpInst::FCMP_UGE, v1, v2, this->BB_); }
    FCmpInst* create_fcmp_lt(Value* v1, Value* v2) { return new FCmpInst(FCmpInst::FCMP_ULT, v1, v2, this->BB_); }
    FCmpInst* create_fcmp_le(Value* v1, Value* v2) { return new FCmpInst(FCmpInst::FCMP_ULE, v1, v2, this->BB_); }

    Value* get_int32(int val) {
        return new ConstantInt(this->m_->int32_ty_, val);
    }


    CallInst* create_call(Value* func, std::vector<Value*> args) {
        return new CallInst(static_cast<::Function*>(func), args, this->BB_);
    }

    BranchInst* create_br(BasicBlock* if_true) {
        return new BranchInst(if_true, this->BB_);
    }
    BranchInst* create_cond_br(Value* cond, BasicBlock* if_true, BasicBlock* if_false) {
        return new BranchInst(cond, if_true, if_false, this->BB_);
    }

    ReturnInst* create_ret(Value* val) {
        return new ReturnInst(val, this->BB_);
    }
    ReturnInst* create_void_ret() {
        return new ReturnInst(this->BB_);
    }

    GetElementPtrInst* create_gep(Value* ptr, std::vector<Value*> idxs) {
        return new GetElementPtrInst(ptr, idxs, this->BB_);
    }

    StoreInst* create_store(Value* val, Value* ptr) {
        return new StoreInst(val, ptr, this->BB_);
    }
    LoadInst* create_load(IRType* ty, Value* ptr) {
        return new LoadInst(ptr, this->BB_);
    }
    LoadInst* create_load(Value* ptr) {
        return new LoadInst(ptr, this->BB_);
    }

    AllocaInst* create_alloca(IRType* ty) {
        return new AllocaInst(ty, this->BB_);
    }
    ZextInst* create_zext(Value* val, IRType* ty) {
        return new ZextInst(Instruction::ZExt, val, ty, this->BB_);
    }
    FpToSiInst* create_fptosi(Value* val, IRType* ty) {
        return new FpToSiInst(Instruction::FPtoSI, val, ty, this->BB_);
    }
    SiToFpInst* create_sitofp(Value* val, IRType* ty) {
        return new SiToFpInst(Instruction::SItoFP, val, ty, this->BB_);
    }
    Bitcast* create_bitcast(Value *val, IRType *ty) {
        return new Bitcast(Instruction::BitCast, val, ty, this->BB_);
    }

};
