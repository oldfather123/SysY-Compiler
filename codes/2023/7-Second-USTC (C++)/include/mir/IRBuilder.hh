#pragma once

#include "BasicBlock.hh"
#include "Instruction.hh"
#include "Value.hh"


class IRBuilder {
public:
    IRBuilder(BasicBlock *bb, Module *m) : BB_(bb), m_(m) {}
    ~IRBuilder() = default;

    Module *get_module() const { return m_; }

    BasicBlock *get_insert_block() const { return this->BB_; }
    void set_insert_point(BasicBlock *bb) { this->BB_ = bb; } //& insert instructions into the basic block".

    BinaryInst *create_iadd( Value *lhs, Value *rhs){ return BinaryInst::create_add( lhs, rhs, this->BB_, m_);}  
    BinaryInst *create_isub( Value *lhs, Value *rhs){ return BinaryInst::create_sub( lhs, rhs, this->BB_, m_);}
    BinaryInst *create_imul( Value *lhs, Value *rhs){ return BinaryInst::create_mul( lhs, rhs, this->BB_, m_);}
    BinaryInst *create_imul64( Value *lhs, Value *rhs){ return BinaryInst::create_mul64( lhs, rhs, this->BB_, m_);}
    BinaryInst *create_isdiv( Value *lhs, Value *rhs){ return BinaryInst::create_sdiv( lhs, rhs, this->BB_, m_);}
    BinaryInst *create_isrem( Value *lhs, Value *rhs){ return BinaryInst::create_srem( lhs, rhs, this->BB_, m_);}

    BinaryInst *create_iand( Value *lhs, Value *rhs){ return BinaryInst::create_and( lhs, rhs, this->BB_, m_);}
    BinaryInst *create_ior( Value *lhs, Value *rhs){ return BinaryInst::create_or( lhs, rhs, this->BB_, m_);}

    BinaryInst *create_fadd(Value *lhs, Value *rhs) { return BinaryInst::create_fadd(lhs, rhs, this->BB_, m_); }
    BinaryInst *create_fsub(Value *lhs, Value *rhs) { return BinaryInst::create_fsub(lhs, rhs, this->BB_, m_); }
    BinaryInst *create_fmul(Value *lhs, Value *rhs) { return BinaryInst::create_fmul(lhs, rhs, this->BB_, m_); }
    BinaryInst *create_fdiv(Value *lhs, Value *rhs) { return BinaryInst::create_fdiv(lhs, rhs, this->BB_, m_); }


    CmpInst *create_icmp_eq( Value *lhs, Value *rhs){ return CmpInst::create_cmp(EQ, lhs, rhs, this->BB_, m_); }
    CmpInst *create_icmp_ne( Value *lhs, Value *rhs){ return CmpInst::create_cmp(NE, lhs, rhs, this->BB_, m_); }
    CmpInst *create_icmp_gt( Value *lhs, Value *rhs){ return CmpInst::create_cmp(GT, lhs, rhs, this->BB_, m_); }
    CmpInst *create_icmp_ge( Value *lhs, Value *rhs){ return CmpInst::create_cmp(GE, lhs, rhs, this->BB_, m_); }
    CmpInst *create_icmp_lt( Value *lhs, Value *rhs){ return CmpInst::create_cmp(LT, lhs, rhs, this->BB_, m_); }
    CmpInst *create_icmp_le( Value *lhs, Value *rhs){ return CmpInst::create_cmp(LE, lhs, rhs, this->BB_, m_); }

    FCmpInst *create_fcmp_ne(Value *lhs, Value *rhs) { return FCmpInst::create_fcmp(NE, lhs, rhs, this->BB_, m_);}
    FCmpInst *create_fcmp_lt(Value *lhs, Value *rhs) { return FCmpInst::create_fcmp(LT, lhs, rhs, this->BB_, m_);}
    FCmpInst *create_fcmp_le(Value *lhs, Value *rhs) { return FCmpInst::create_fcmp(LE, lhs, rhs, this->BB_, m_);}
    FCmpInst *create_fcmp_ge(Value *lhs, Value *rhs) { return FCmpInst::create_fcmp(GE, lhs, rhs, this->BB_, m_);}
    FCmpInst *create_fcmp_gt(Value *lhs, Value *rhs) { return FCmpInst::create_fcmp(GT, lhs, rhs, this->BB_, m_);}
    FCmpInst *create_fcmp_eq(Value *lhs, Value *rhs) { return FCmpInst::create_fcmp(EQ, lhs, rhs, this->BB_, m_);}

    BinaryInst *create_asr(Value *lhs, Value *rhs) { return BinaryInst::create_asr(lhs, rhs, this->BB_, m_);}
    BinaryInst *create_lsl(Value *lhs, Value *rhs) { return BinaryInst::create_lsl(lhs, rhs, this->BB_, m_);}
    BinaryInst *create_lsr(Value *lhs, Value *rhs) { return BinaryInst::create_lsr(lhs, rhs, this->BB_, m_);}
    BinaryInst *create_asr64(Value *lhs, Value *rhs) { return BinaryInst::create_asr64(lhs, rhs, this->BB_, m_);}
    BinaryInst *create_lsl64(Value *lhs, Value *rhs) { return BinaryInst::create_lsl64(lhs, rhs, this->BB_, m_);}
    BinaryInst *create_lsr64(Value *lhs, Value *rhs) { return BinaryInst::create_lsr64(lhs, rhs, this->BB_, m_);}

    CallInst *create_call(Value *func, std::vector<Value *> args) {
        return CallInst::create_call(static_cast<Function *>(func) ,args, this->BB_); 
    }

    BranchInst *create_br(BasicBlock *if_true){ return BranchInst::create_br(if_true, this->BB_); }
    BranchInst *create_cond_br(Value *cond, BasicBlock *if_true, BasicBlock *if_false){ return BranchInst::create_cond_br(cond, if_true, if_false,this->BB_); }

    ReturnInst *create_ret(Value *val) { return ReturnInst::create_ret(val,this->BB_); }
    ReturnInst *create_void_ret() { return ReturnInst::create_void_ret(this->BB_); }

    GetElementPtrInst *create_gep(Value *ptr, std::vector<Value *> idxs) { return GetElementPtrInst::create_gep(ptr, idxs, this->BB_); }

    StoreInst *create_store(Value *val, Value *ptr) { return StoreInst::create_store(val, ptr, this->BB_ ); }
    LoadInst * create_load(Type *ty, Value *ptr) { return LoadInst::create_load(ty, ptr, this->BB_); }
    LoadInst * create_load(Value *ptr) {
        return LoadInst::create_load(ptr->get_type()->get_pointer_element_type(), ptr, this->BB_); 
    }
    MemsetInst *create_memset(Value *ptr) { return MemsetInst::create_memset(ptr, this->BB_ ); }

    AllocaInst *create_alloca(Type *ty) { return AllocaInst::create_alloca(ty, this->BB_); }

    ZextInst *create_zext(Value *val, Type *ty) { return ZextInst::create_zext(val, ty, this->BB_); }

    SiToFpInst *create_sitofp(Value *val, Type *ty) { return SiToFpInst::create_sitofp(val, ty, this->BB_); }
    FpToSiInst *create_fptosi(Value *val, Type *ty) { return FpToSiInst::create_fptosi(val, ty, this->BB_); }
private:
    BasicBlock *BB_;
    Module *m_;
};
