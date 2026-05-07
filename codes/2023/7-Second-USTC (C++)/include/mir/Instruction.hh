#pragma once

#include <string>
#include <vector>

#include "User.hh"
#include "Type.hh"
#include "Constant.hh"
#include "BasicBlock.hh"

class BasicBlock;
class Function;

class Instruction : public User {
public:
    enum OpID {
      //& Terminator Instructions
      ret,
      br,
      //& Standard binary operators
      add,
      sub,
      mul,
      mul64,
      sdiv,
      srem,
      //& float binary operators
      fadd,
      fsub,
      fmul,
      fdiv,
      //& Memory operators
      alloca,
      load,
      store,
      memset,
      //& Other operators
      cmp,
      fcmp,
      phi,
      call,
      getelementptr,
      //& Logical operators
      land,
      lor,
      lxor,
      //& Shift operators
      asr,
      shl,
      lsr,
      asr64,
      shl64,
      lsr64,
      //& Zero extend
      zext,
      //& type cast bewteen float and singed integer
      fptosi,
      sitofp,
      //& LIR operators
      cmpbr,
      fcmpbr,
      loadoffset,
      storeoffset
    };

public:
    OpID get_instr_type() { return op_id_; }

    static std::string get_instr_op_name(OpID id) {
      switch(id) {
        case ret: return "ret"; break;
        case br: return "br"; break;

        case add: return "add"; break;
        case sub: return "sub"; break;
        case mul: return "mul"; break;
        case mul64: return "mulh64"; break;
        case sdiv: return "sdiv"; break;
        case srem: return "srem"; break;

        case fadd: return "fadd"; break;
        case fsub: return "fsub"; break;
        case fmul: return "fmul"; break;
        case fdiv: return "fdiv"; break;

        case alloca: return "alloca"; break;
        case load: return "load"; break;
        case store: return "store"; break;
        case memset: return "memset"; break;

        case cmp: return "cmp"; break;
        case fcmp: return "fcmp"; break;
        case phi: return "phi"; break;
        case call: return "call"; break;
        case getelementptr: return "getelementptr"; break;

        case land: return "and"; break;
        case lor: return "or"; break;
        case lxor: return "xor"; break;

        case asr: return "ashr"; break;
        case shl: return "shl"; break;
        case lsr: return "lshr"; break; 
        case asr64: return "asr64"; break;
        case shl64: return "shl64"; break;
        case lsr64: return "lsr64"; break; 

        case zext: return "zext"; break;

        case fptosi: return "fptosi"; break; 
        case sitofp: return "sitofp"; break; 

        case cmpbr: return "cmpbr"; break;
        case fcmpbr: return "fcmpbr"; break;
        case loadoffset: return "loadoffset"; break;
        case storeoffset: return "storeoffset"; break;

        default: return ""; break; 
      }
    }

public:
    //& create instruction, auto insert to bb, ty here is result type
    Instruction(Type *ty, OpID id, unsigned num_ops, BasicBlock *parent);
    Instruction(Type *ty, OpID id, unsigned num_ops);

    inline const BasicBlock *get_parent() const { return parent_; }
    inline BasicBlock *get_parent() { return parent_; }
    void set_parent(BasicBlock *parent) { this->parent_ = parent; }

    //// Return the function this instruction belongs to.
    Function *get_function();
    
    Module *get_module();

    bool is_void() {
        return ((op_id_ == ret) || (op_id_ == br) || (op_id_ == store) || (op_id_ == cmpbr) || (op_id_ == fcmpbr) || (op_id_ == storeoffset) || (op_id_ == memset) ||
                (op_id_ == call && this->get_type()->is_void_type()));
    }

    bool is_ret() { return op_id_ == ret; } 
    bool is_br() { return op_id_ ==  br; } 

    bool is_add() { return op_id_ ==  add; } 
    bool is_sub() { return op_id_ ==  sub; }
    bool is_mul() { return op_id_ ==  mul; } 
    bool is_mul64() { return op_id_ == mul64; }
    bool is_div() { return op_id_ ==  sdiv; }
    bool is_rem() { return op_id_ ==  srem; } 

    bool is_fadd() { return op_id_ ==  fadd; } 
    bool is_fsub() { return op_id_ ==  fsub; } 
    bool is_fmul() { return op_id_ ==  fmul; } 
    bool is_fdiv() { return op_id_ ==  fdiv; } 

    bool is_alloca() { return op_id_ ==  alloca; } 
    bool is_load() { return op_id_ ==  load; } 
    bool is_store() { return op_id_ ==  store; } 
    bool is_memset() { return op_id_ == memset; }

    bool is_cmp() { return op_id_ ==  cmp; }
    bool is_fcmp() { return op_id_ ==  fcmp; } 
    bool is_phi() { return op_id_ ==  phi; } 
    bool is_call() { return op_id_ ==  call; }
    bool is_gep(){ return op_id_ ==  getelementptr; } 

    bool is_and() { return op_id_ ==  land; } 
    bool is_or() { return op_id_ ==  lor; }
    bool is_xor() { return op_id_ == lxor; } 

    bool is_asr() { return op_id_ ==  asr; }
    bool is_lsl() { return op_id_ ==  shl; } 
    bool is_lsr() { return op_id_ ==  lsr; } 
    bool is_asr64() { return op_id_ ==  asr64; }
    bool is_lsl64() { return op_id_ ==  shl64; } 
    bool is_lsr64() { return op_id_ ==  lsr64; }

    bool is_zext() { return op_id_ ==  zext; } 
    
    bool is_fptosi(){ return op_id_ ==  fptosi; } 
    bool is_sitofp(){ return op_id_ ==  sitofp; } 

    bool is_cmpbr() { return op_id_ == cmpbr; }
    bool is_fcmpbr() { return op_id_ == fcmpbr; }
    bool is_loadoffset() { return op_id_ == loadoffset; }
    bool is_storeoffset() { return op_id_ == storeoffset; }

    bool is_extend_br() { return (op_id_ ==  br || op_id_ == cmpbr || op_id_ == fcmpbr); }
    bool is_extend_cond_br() const { return get_num_operands() == 3 || get_num_operands() == 4; }


    bool is_int_binary() {
        return (is_add() || is_sub() || is_mul() || is_div() || is_rem() || is_mul64() ||
                is_and() || is_or() || is_xor() || 
                is_asr() || is_lsl() || is_lsr() || is_asr64() || is_lsl64() || is_lsr64()) &&
               (get_num_operands() == 2);
    }

    bool is_float_binary() {
        return (is_fadd() || is_fsub() || is_fmul() || is_fdiv()) && (get_num_operands() == 2);
    }

    bool is_binary() {
        return is_int_binary() || is_float_binary();

    }

    bool is_terminator() { return is_br() || is_ret() || is_cmpbr() || is_fcmpbr(); }

    void set_id(int id) { id_ = id; }
    int get_id() { return id_; }

    virtual Instruction *copy_inst(BasicBlock *bb) = 0;

private:
    OpID op_id_;
    unsigned num_ops_;
    BasicBlock* parent_;
    int id_;
};

class BinaryInst : public Instruction {
public:
    static BinaryInst *create_add(Value *v1, Value *v2, BasicBlock *bb, Module *m);
    static BinaryInst *create_sub(Value *v1, Value *v2, BasicBlock *bb, Module *m);
    static BinaryInst *create_mul(Value *v1, Value *v2, BasicBlock *bb, Module *m);
    static BinaryInst *create_mul64(Value *v1, Value *v2, BasicBlock *bb, Module *m);
    static BinaryInst *create_sdiv(Value *v1, Value *v2, BasicBlock *bb, Module *m);
    static BinaryInst *create_srem(Value *v1, Value *v2, BasicBlock *bb, Module *m);
    static BinaryInst *create_fadd(Value *v1, Value *v2, BasicBlock *bb, Module *m);
    static BinaryInst *create_fsub(Value *v1, Value *v2, BasicBlock *bb, Module *m);
    static BinaryInst *create_fmul(Value *v1, Value *v2, BasicBlock *bb, Module *m);
    static BinaryInst *create_fdiv(Value *v1, Value *v2, BasicBlock *bb, Module *m);

    static BinaryInst *create_and(Value *v1, Value *v2, BasicBlock *bb, Module *m);
    static BinaryInst *create_or(Value *v1, Value *v2, BasicBlock *bb, Module *m);
    static BinaryInst *create_xor(Value *v1, Value *v2, BasicBlock *bb, Module *m);

    static BinaryInst *create_asr(Value *v1, Value *v2, BasicBlock *bb, Module *m);
    static BinaryInst *create_lsl(Value *v1, Value *v2, BasicBlock *bb, Module *m);
    static BinaryInst *create_lsr(Value *v1, Value *v2, BasicBlock *bb, Module *m);
    static BinaryInst *create_asr64(Value *v1, Value *v2, BasicBlock *bb, Module *m);
    static BinaryInst *create_lsl64(Value *v1, Value *v2, BasicBlock *bb, Module *m);
    static BinaryInst *create_lsr64(Value *v1, Value *v2, BasicBlock *bb, Module *m);

    virtual std::string print() override;

    Instruction *copy_inst(BasicBlock *bb) override final {
        return new BinaryInst(get_type(), get_instr_type(), get_operand(0), get_operand(1), bb);
    }

private:
    BinaryInst(Type *ty, OpID id, Value *v1, Value *v2, BasicBlock *bb); 

    //~ void assert_valid();
};


enum CmpOp {
    EQ, // ==
    NE, // !=
    GT, // >
    GE, // >=
    LT, // <
    LE  // <=
};
class CmpInst : public Instruction {
public:
    static CmpInst *create_cmp(CmpOp op, Value *lhs, Value *rhs, BasicBlock *bb, Module *m);

    CmpOp get_cmp_op() { return cmp_op_; }

    void negation();

    virtual std::string print() override;

    Instruction *copy_inst(BasicBlock *bb) override final {
        return new CmpInst(get_type(), cmp_op_, get_operand(0), get_operand(1), bb);
    }

private:
    CmpInst(Type *ty, CmpOp op, Value *lhs, Value *rhs, BasicBlock *bb); 
    //~ void assert_valid();
    
private:
    CmpOp cmp_op_;
    
};

class FCmpInst : public Instruction {
  public:
    static FCmpInst *create_fcmp(CmpOp op, Value *lhs, Value *rhs, BasicBlock *bb, Module *m);

    CmpOp get_cmp_op() { return cmp_op_; }

    void negation();

    virtual std::string print() override;
    Instruction *copy_inst(BasicBlock *bb) override final {
        return new FCmpInst(get_type(), cmp_op_, get_operand(0), get_operand(1), bb);
    }

  private:
    FCmpInst(Type *ty, CmpOp op, Value *lhs, Value *rhs, BasicBlock *bb);

    //~ void assert_valid();

  private:
    CmpOp cmp_op_;
};

class CallInst : public Instruction {
public:
    static CallInst *create_call(Function *func, std::vector<Value *>args, BasicBlock *bb);
    
    FunctionType *get_function_type() const { return static_cast<FunctionType *>(get_operand(0)->get_type()); }

    virtual std::string print() override;

    Instruction *copy_inst(BasicBlock *bb) override final{
        std::vector<Value *> args;
        for (auto i = 1; i < get_num_operands(); i++){
            args.push_back(get_operand(i));
        }
        auto new_inst = new CallInst(get_function_type()->get_return_type(),args,bb);
        new_inst->set_operand(0, get_operand(0));
        return new_inst;
    }

protected:
    CallInst(Function *func, std::vector<Value *>args, BasicBlock *bb);
    CallInst(Type *ret_ty, std::vector<Value *> args, BasicBlock *bb);
};

class BranchInst : public Instruction {
public:
    static BranchInst *create_cond_br(Value *cond, BasicBlock *if_true, BasicBlock *if_false, BasicBlock *bb);
    static BranchInst *create_br(BasicBlock *if_true, BasicBlock *bb);

   
    bool is_cond_br() const { return get_num_operands() == 3; }
    // bool is_extend_cond_br() const { return get_num_operands() == 3 || get_num_operands() == 4; }

    virtual std::string print() override;

    Instruction *copy_inst(BasicBlock *bb) override final{
        if (get_num_operands() == 1){
            auto new_inst = new BranchInst(bb);
            new_inst->set_operand(0, get_operand(0));
            return new_inst;
        } else {
            auto new_inst = new BranchInst(get_operand(0),bb);
            new_inst->set_operand(1, get_operand(1));
            new_inst->set_operand(2, get_operand(2));
            return new_inst;
        }
    }

private:
    BranchInst(Value *cond, BasicBlock *if_true, BasicBlock *if_false,
               BasicBlock *bb);
    BranchInst(BasicBlock *if_true, BasicBlock *bb);
    BranchInst(BasicBlock *bb);
    BranchInst(Value *cond, BasicBlock *bb);
};

class ReturnInst : public Instruction {
public:
    static ReturnInst *create_ret(Value *val, BasicBlock *bb);
    static ReturnInst *create_void_ret(BasicBlock *bb);

    bool is_void_ret() const { return get_num_operands() == 0; }

    Type * get_ret_type() const { return get_operand(0)->get_type(); }

    virtual std::string print() override;

    Instruction *copy_inst(BasicBlock *bb) override final{
        if (is_void_ret()){
            return new ReturnInst(bb);
        } else {
            return new ReturnInst(get_operand(0),bb);
        }
    }

private:
    ReturnInst(Value *val, BasicBlock *bb);
    ReturnInst(BasicBlock *bb);
};

class GetElementPtrInst : public Instruction {
public:
    static Type *get_element_type(Value *ptr, std::vector<Value *> idxs);
    static GetElementPtrInst *create_gep(Value *ptr, std::vector<Value *> idxs, BasicBlock *bb);
    Type *get_element_type() const { return element_ty_; }

    virtual std::string print() override;

    Instruction *copy_inst(BasicBlock *bb) override final{
        std::vector<Value *> idxs;
        for (auto i = 1; i < get_num_operands(); i++) {
            idxs.push_back(get_operand(i));
        }
        return new GetElementPtrInst(get_operand(0),idxs,bb);
    }

private:
    GetElementPtrInst(Value *ptr, std::vector<Value *> idxs, BasicBlock *bb);

private:
    Type *element_ty_;
};


class StoreInst : public Instruction {
public:
    static StoreInst *create_store(Value *val, Value *ptr, BasicBlock *bb);

    Value *get_rval() { return this->get_operand(0); }
    Value *get_lval() { return this->get_operand(1); }

    virtual std::string print() override;

    Instruction *copy_inst(BasicBlock *bb) override final{
        return new StoreInst(get_operand(0),get_operand(1),bb);
    }

private:
    StoreInst(Value *val, Value *ptr, BasicBlock *bb);
};

//& 加速使用全0初始化数组的代码优化分析和代码生成
class MemsetInst : public Instruction {
public:
    static MemsetInst *create_memset(Value *ptr, BasicBlock *bb);

    Value *get_lval() { return this->get_operand(0); }

    virtual std::string print() override;

    Instruction *copy_inst(BasicBlock *bb) override final{
        return new MemsetInst(get_operand(0),bb);
    }

private:
    MemsetInst(Value *ptr, BasicBlock *bb);
};

class LoadInst : public Instruction {
public:
    static LoadInst *create_load(Type *ty, Value *ptr, BasicBlock *bb);
    
    Value * get_lval() { return this->get_operand(0); }

    Type *get_load_type() const { return static_cast<PointerType *>(get_operand(0)->get_type())->get_element_type(); }

    virtual std::string print() override;

    Instruction *copy_inst(BasicBlock *bb) override final{
        return new LoadInst(get_type(),get_operand(0),bb);
    }

private:
    LoadInst(Type *ty, Value *ptr, BasicBlock *bb);
};


class AllocaInst : public Instruction {
public:
    static AllocaInst *create_alloca(Type *ty, BasicBlock *bb);

    Type *get_alloca_type() const { return alloca_ty_; }

    virtual std::string print() override;

    Instruction *copy_inst(BasicBlock *bb) override final{
        return new AllocaInst(alloca_ty_,bb);
    }

private:
    AllocaInst(Type *ty, BasicBlock *bb);

private:
    Type *alloca_ty_;
};

class ZextInst : public Instruction {
public:
    static ZextInst *create_zext(Value *val, Type *ty, BasicBlock *bb);

    Type *get_dest_type() const { return dest_ty_; }

    virtual std::string print() override;

    Instruction *copy_inst(BasicBlock *bb) override final{
        return new ZextInst(get_instr_type(),get_operand(0),dest_ty_,bb);
    }

private:
    ZextInst(OpID op, Value *val, Type *ty, BasicBlock *bb);

private:
  Type *dest_ty_;
};

class SiToFpInst : public Instruction {
public:
    static SiToFpInst *create_sitofp(Value *val, Type *ty, BasicBlock *bb);

    Type *get_dest_type() const { return dest_ty_; }

    virtual std::string print() override;

    Instruction *copy_inst(BasicBlock *bb) override final{
        return new SiToFpInst(get_instr_type(), get_operand(0), get_dest_type(), bb);
    }

private:
    SiToFpInst(OpID op, Value *val, Type *ty, BasicBlock *bb);

private:
    Type *dest_ty_;
};

class FpToSiInst : public Instruction {
public:
    static FpToSiInst *create_fptosi(Value *val, Type *ty, BasicBlock *bb);

    Type *get_dest_type() const { return dest_ty_; }

    virtual std::string print() override;

    Instruction *copy_inst(BasicBlock *bb) override final{
        return new FpToSiInst(get_instr_type(), get_operand(0), get_dest_type(), bb);
    }

private:
    FpToSiInst(OpID op, Value *val, Type *ty, BasicBlock *bb);

private:
    Type *dest_ty_;
};

class PhiInst : public Instruction {
public:
    static PhiInst *create_phi(Type *ty, BasicBlock *bb);

    Value *get_lval() { return l_val_; }
    void set_lval(Value *l_val) { l_val_ = l_val; }

    void add_phi_pair_operand(Value *val, Value *pre_bb) {
        this->add_operand(val);
        this->add_operand(pre_bb);
    }

    virtual std::string print() override;

    Instruction *copy_inst(BasicBlock *bb) override final{
        auto new_inst = create_phi(get_type(), bb);
        for (auto op : get_operands()){
            new_inst->add_operand(op);
        }
        return new_inst;
    }
private:
     PhiInst(OpID op, std::vector<Value *> vals, std::vector<BasicBlock *> val_bbs, Type *ty, BasicBlock *bb);

private:
    Value *l_val_;
};

class CmpBrInst: public Instruction {

public:
    static CmpBrInst *create_cmpbr(CmpOp op, Value *lhs, Value *rhs, BasicBlock *if_true, BasicBlock *if_false, BasicBlock *bb, Module *m);

    CmpOp get_cmp_op() { return cmp_op_; }

    bool is_cmp_br() const;

    virtual std::string print() override;

    Instruction *copy_inst(BasicBlock *bb) override final{
        auto new_inst = new CmpBrInst(cmp_op_,get_operand(0),get_operand(1),bb);
        new_inst->set_operand(2, get_operand(2));
        new_inst->set_operand(3, get_operand(3));
        return new_inst;
    }

private:
    CmpBrInst(CmpOp op, Value *lhs, Value *rhs, BasicBlock *if_true, BasicBlock *if_false, BasicBlock *bb);
    CmpBrInst(CmpOp op, Value *lhs, Value *rhs, BasicBlock *bb);
private:
    CmpOp cmp_op_;

};


class FCmpBrInst : public Instruction {
public:
    static FCmpBrInst *create_fcmpbr(CmpOp op, Value *lhs, Value *rhs, BasicBlock *if_true, BasicBlock *if_false, BasicBlock *bb, Module *m);

    CmpOp get_cmp_op() { return cmp_op_; }

    bool is_fcmp_br() const;

    virtual std::string print() override;

    Instruction *copy_inst(BasicBlock *bb) override final{
        auto new_inst = new FCmpBrInst(cmp_op_,get_operand(0),get_operand(1),bb);
        new_inst->set_operand(2, get_operand(2));
        new_inst->set_operand(3, get_operand(3));
        return new_inst;
    }


private:
    FCmpBrInst(CmpOp op, Value *lhs, Value *rhs, BasicBlock *if_true, BasicBlock *if_false, BasicBlock *bb);
    FCmpBrInst(CmpOp op, Value *lhs, Value *rhs, BasicBlock *bb);
private:
    CmpOp cmp_op_;
};

class LoadOffsetInst: public Instruction {
public:
    static LoadOffsetInst *create_loadoffset(Type *ty, Value *ptr, Value *offset, BasicBlock *bb);

    Value *get_lval() { return this->get_operand(0); }
    Value *get_offset() { return this->get_operand(1); }

    Type *get_load_type() const;

    virtual std::string print() override;

    Instruction *copy_inst(BasicBlock *bb) override final{
        auto new_inst = new LoadOffsetInst(get_type(), get_operand(0), bb);
        new_inst->set_operand(1, get_operand(1));
        return new_inst;
    }

private:
    LoadOffsetInst(Type *ty, Value *ptr, Value *offset, BasicBlock *bb);
    LoadOffsetInst(Type *ty, Value *ptr, BasicBlock *bb);
};


class StoreOffsetInst: public Instruction {

public:
    static StoreOffsetInst *create_storeoffset(Value *val, Value *ptr, Value *offset, BasicBlock *bb);

    Type *get_store_type() const;

    Value *get_rval() { return this->get_operand(0); }
    Value *get_lval() { return this->get_operand(1); }
    Value *get_offset() { return this->get_operand(2); }

    virtual std::string print() override;

    Instruction *copy_inst(BasicBlock *bb) override final{
        auto new_inst = new StoreOffsetInst(get_operand(0), get_operand(1), bb);
        new_inst->set_operand(2, get_operand(2));
        return new_inst;
    }

private:
    StoreOffsetInst(Value *val, Value *ptr, Value *offset, BasicBlock *bb);
    StoreOffsetInst(Value *val, Value *ptr, BasicBlock *bb);
};








