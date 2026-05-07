#include <algorithm>

#include "IRprinter.hh"
#include "Function.hh"
#include "Module.hh"


//& Instruction
Instruction::Instruction(Type *ty, OpID id, unsigned num_ops, BasicBlock *parent)
    : User(ty, "", num_ops), op_id_(id), num_ops_(num_ops), parent_(parent) {
    parent_->add_instruction(this);
}

Instruction::Instruction(Type *ty, OpID id, unsigned num_ops)
    : User(ty, "", num_ops), op_id_(id), num_ops_(num_ops), parent_(nullptr) {}


Function *Instruction::get_function() { 
    return parent_->get_parent(); 
}

Module *Instruction::get_module() { 
    return parent_->get_module();
}

//& BinaryInst

BinaryInst::BinaryInst(Type *ty, OpID id, Value *v1, Value *v2, BasicBlock *bb)
    : Instruction(ty, id, 2, bb) {
        set_operand(0, v1);
        set_operand(1, v2);
}

BinaryInst *BinaryInst::create_add(Value *v1, Value *v2, BasicBlock *bb, Module *m) {
    return new BinaryInst(v1->get_type()->is_pointer_type() ? v1->get_type() : v2->get_type(), Instruction::add, v1, v2, bb);
}

BinaryInst *BinaryInst::create_sub(Value *v1, Value *v2, BasicBlock *bb, Module *m) {
    return new BinaryInst(Type::get_int32_type(m), Instruction::sub, v1, v2, bb);
}

BinaryInst *BinaryInst::create_mul(Value *v1, Value *v2, BasicBlock *bb, Module *m) {
    return new BinaryInst(Type::get_int32_type(m), Instruction::mul, v1, v2, bb);
}

BinaryInst *BinaryInst::create_mul64(Value *v1, Value *v2, BasicBlock *bb, Module *m) {
    return new BinaryInst(Type::get_int32_type(m), Instruction::mul64, v1, v2, bb);
}

BinaryInst *BinaryInst::create_sdiv(Value *v1, Value *v2, BasicBlock *bb, Module *m) {
    return new BinaryInst(Type::get_int32_type(m), Instruction::sdiv, v1, v2, bb);
}

BinaryInst *BinaryInst::create_srem(Value *v1, Value *v2, BasicBlock *bb, Module *m) {
    return new BinaryInst(Type::get_int32_type(m), Instruction::srem, v1, v2, bb);
}

BinaryInst *BinaryInst::create_fadd(Value *v1, Value *v2, BasicBlock *bb, Module *m) {
    return new BinaryInst(Type::get_float_type(m), Instruction::fadd, v1, v2, bb);
}

BinaryInst *BinaryInst::create_fsub(Value *v1, Value *v2, BasicBlock *bb, Module *m) {
    return new BinaryInst(Type::get_float_type(m), Instruction::fsub, v1, v2, bb);
}

BinaryInst *BinaryInst::create_fmul(Value *v1, Value *v2, BasicBlock *bb, Module *m) {
    return new BinaryInst(Type::get_float_type(m), Instruction::fmul, v1, v2, bb);
}

BinaryInst *BinaryInst::create_fdiv(Value *v1, Value *v2, BasicBlock *bb, Module *m) {
    return new BinaryInst(Type::get_float_type(m), Instruction::fdiv, v1, v2, bb);
}

BinaryInst *BinaryInst::create_and(Value *v1, Value *v2, BasicBlock *bb, Module *m) {
    return new BinaryInst(Type::get_int32_type(m), Instruction::land, v1, v2, bb);
}

BinaryInst *BinaryInst::create_or(Value *v1, Value *v2, BasicBlock *bb, Module *m) {
    return new BinaryInst(Type::get_int32_type(m), Instruction::lor, v1, v2, bb);
}

BinaryInst *BinaryInst::create_xor(Value *v1, Value *v2, BasicBlock *bb, Module *m) {
    return new BinaryInst(Type::get_int32_type(m), Instruction::lxor, v1, v2, bb);
}

BinaryInst *BinaryInst::create_asr(Value *v1, Value *v2, BasicBlock *bb, Module *m) {
    return new BinaryInst(Type::get_int32_type(m), Instruction::asr, v1, v2, bb);
}

BinaryInst *BinaryInst::create_lsl(Value *v1, Value *v2, BasicBlock *bb, Module *m) {
    return new BinaryInst(Type::get_int32_type(m), Instruction::shl, v1, v2, bb);
}

BinaryInst *BinaryInst::create_lsr(Value *v1, Value *v2, BasicBlock *bb, Module *m) {
    return new BinaryInst(Type::get_int32_type(m), Instruction::lsr, v1, v2, bb);
}

BinaryInst *BinaryInst::create_asr64(Value *v1, Value *v2, BasicBlock *bb, Module *m) {
    return new BinaryInst(Type::get_int32_type(m), Instruction::asr64, v1, v2, bb);
}

BinaryInst *BinaryInst::create_lsl64(Value *v1, Value *v2, BasicBlock *bb, Module *m) {
    return new BinaryInst(Type::get_int32_type(m), Instruction::shl64, v1, v2, bb);
}

BinaryInst *BinaryInst::create_lsr64(Value *v1, Value *v2, BasicBlock *bb, Module *m) {
    return new BinaryInst(Type::get_int32_type(m), Instruction::lsr64, v1, v2, bb);
}



std::string BinaryInst::print() {
    std::string instr_ir;
    instr_ir += "%";
    instr_ir += this->get_name();
    instr_ir += " = ";
    instr_ir += this->get_module()->get_instr_op_name(this->get_instr_type());
    instr_ir += " ";
    instr_ir += this->get_operand(0)->get_type()->print();
    instr_ir += " ";
    instr_ir += print_as_op(this->get_operand(0), false);
    instr_ir += ", ";
    if (Type::is_eq_type(this->get_operand(0)->get_type(), this->get_operand(1)->get_type())) {
        instr_ir += print_as_op(this->get_operand(1), false);
    } else {
        instr_ir += print_as_op(this->get_operand(1), true);
    }
    return instr_ir;
}

//& CmpInst
CmpInst::CmpInst(Type *ty, CmpOp op, Value *lhs, Value *rhs, BasicBlock *bb) 
    : Instruction(ty, Instruction::cmp, 2, bb), cmp_op_(op) {
    set_operand(0, lhs);
    set_operand(1, rhs); 
}

CmpInst *CmpInst::create_cmp(CmpOp op, Value *lhs, Value *rhs, BasicBlock *bb, Module *m) {
    return new CmpInst(m->get_int1_type(), op, lhs, rhs, bb);
}

void CmpInst::negation() {
    switch(get_cmp_op()) {
        case CmpOp::EQ:
            cmp_op_ = CmpOp::NE;
            break;
        case CmpOp::NE:
            cmp_op_ = CmpOp::EQ;
            break;
        case CmpOp::GT:
            cmp_op_ = CmpOp::LE;
            break;
        case CmpOp::GE:
            cmp_op_ = CmpOp::LT;
            break;
        case CmpOp::LT:
            cmp_op_ = CmpOp::GE;
            break;
        case CmpOp::LE:
            cmp_op_ = CmpOp::GT;
            break;
        default:
            break;
    }
    return ;
}

std::string CmpInst::print() {
    std::string instr_ir;
    instr_ir += "%";
    instr_ir += this->get_name();
    instr_ir += " = ";
    instr_ir += this->get_module()->get_instr_op_name(this->get_instr_type());
    instr_ir += " ";
    instr_ir += print_cmp_type(this->cmp_op_);
    instr_ir += " ";
    instr_ir += this->get_operand(0)->get_type()->print();
    instr_ir += " ";
    instr_ir += print_as_op(this->get_operand(0), false);
    instr_ir += ", ";
    if (Type::is_eq_type(this->get_operand(0)->get_type(), this->get_operand(1)->get_type())) {
        instr_ir += print_as_op(this->get_operand(1), false);
    } else {
        instr_ir += print_as_op(this->get_operand(1), true);
    }
    return instr_ir;
}


//& FCmpInst

FCmpInst::FCmpInst(Type *ty, CmpOp op, Value *lhs, Value *rhs, BasicBlock *bb)
    : Instruction(ty, Instruction::fcmp, 2, bb), cmp_op_(op) {
    set_operand(0, lhs);
    set_operand(1, rhs);
}

FCmpInst *FCmpInst::create_fcmp(CmpOp op, Value *lhs, Value *rhs, BasicBlock *bb, Module *m) {
    return new FCmpInst(m->get_int1_type(), op, lhs, rhs, bb);
}

void FCmpInst::negation() {
    switch(get_cmp_op()) {
        case CmpOp::EQ:
            cmp_op_ = CmpOp::NE;
            break;
        case CmpOp::NE:
            cmp_op_ = CmpOp::EQ;
            break;
        case CmpOp::GT:
            cmp_op_ = CmpOp::LE;
            break;
        case CmpOp::GE:
            cmp_op_ = CmpOp::LT;
            break;
        case CmpOp::LT:
            cmp_op_ = CmpOp::GE;
            break;
        case CmpOp::LE:
            cmp_op_ = CmpOp::GT;
            break;
        default:
            break;
    }
    return ;
}

std::string FCmpInst::print() {
    std::string instr_ir;
    instr_ir += "%";
    instr_ir += this->get_name();
    instr_ir += " = ";
    instr_ir += this->get_module()->get_instr_op_name(this->get_instr_type());
    instr_ir += " ";
    instr_ir += print_fcmp_type(this->cmp_op_);
    instr_ir += " ";
    instr_ir += this->get_operand(0)->get_type()->print();
    instr_ir += " ";
    instr_ir += print_as_op(this->get_operand(0), false);
    instr_ir += ",";
    if (Type::is_eq_type(this->get_operand(0)->get_type(), this->get_operand(1)->get_type())) {
        instr_ir += print_as_op(this->get_operand(1), false);
    } else {
        instr_ir += print_as_op(this->get_operand(1), true);
    }
    return instr_ir;
}

//& CallInst
CallInst::CallInst(Function *func, std::vector<Value *> args, BasicBlock *bb) 
    : Instruction(func->get_return_type(), Instruction::call, args.size()+1, bb) {
    assert(func->get_num_of_args() == args.size());
    int num_ops = args.size() + 1;
    set_operand(0, func);
    for (int i = 1; i < num_ops; i++) {
        set_operand(i, args[i - 1]);
    }
}

CallInst::CallInst(Type *ret_ty, std::vector<Value *> args, BasicBlock *bb)
    : Instruction(ret_ty, Instruction::call, args.size() + 1, bb) {
    int num_ops = args.size() + 1; 
    for (int i = 1; i < num_ops; i++) {
        set_operand(i, args[i-1]);
    }
}

CallInst *CallInst::create_call(Function *func, std::vector<Value *> args, BasicBlock *bb) {
    return new CallInst(func, args, bb);
}

std::string CallInst::print() {
    std::string instr_ir;
    if (!this->is_void()) {
        instr_ir += "%";
        instr_ir += this->get_name();
        instr_ir += " = ";
    }
    instr_ir += this->get_module()->get_instr_op_name(this->get_instr_type());
    instr_ir += " ";
    instr_ir += this->get_function_type()->get_return_type()->print();

    instr_ir += " ";
    assert(dynamic_cast<Function *>(this->get_operand(0)) && "Wrong call operand function");
    instr_ir += print_as_op(this->get_operand(0), false);
    instr_ir += "(";
    for (int i = 1; i < this->get_num_operands(); i++) {
        if (i > 1)
            instr_ir += ", ";
        instr_ir += this->get_operand(i)->get_type()->print();
        instr_ir += " ";
        instr_ir += print_as_op(this->get_operand(i), false);
    }
    instr_ir += ")";
    return instr_ir;
}

//& BranchInst
BranchInst::BranchInst(Value *cond, BasicBlock *if_true, BasicBlock *if_false, BasicBlock *bb) 
    : Instruction(Type::get_void_type(if_true->get_module()), Instruction::br, 3, bb) { 
    set_operand(0, cond);
    set_operand(1, if_true);
    set_operand(2, if_false); 
}

BranchInst::BranchInst(BasicBlock *if_true, BasicBlock *bb)
    : Instruction(Type::get_void_type(if_true->get_module()), Instruction::br, 1, bb) {
    set_operand(0, if_true);
}

BranchInst::BranchInst(Value *cond, BasicBlock *bb)
    : Instruction(Type::get_void_type(bb->get_module()), Instruction::br, 3, bb) {
    set_operand(0, cond);
}

BranchInst::BranchInst(BasicBlock *bb)
    : Instruction(Type::get_void_type(bb->get_module()), Instruction::br, 1, bb) {
    //nothing to do
}

BranchInst *BranchInst::create_cond_br(Value *cond, BasicBlock *if_true, BasicBlock *if_false, BasicBlock *bb) {
    if_true->add_pre_basic_block(bb);
    if_false->add_pre_basic_block(bb);
    bb->add_succ_basic_block(if_false);
    bb->add_succ_basic_block(if_true);

    return new BranchInst(cond, if_true, if_false, bb);
}

BranchInst *BranchInst::create_br(BasicBlock *if_true, BasicBlock *bb) {
    if_true->add_pre_basic_block(bb);
    bb->add_succ_basic_block(if_true);

    return new BranchInst(if_true, bb);
}

std::string BranchInst::print() {
    std::string instr_ir;
    instr_ir += this->get_module()->get_instr_op_name(this->get_instr_type());
    instr_ir += " ";
    //// instr_ir += this->get_operand(0)->get_type()->print();
    instr_ir += print_as_op(this->get_operand(0), true);
    if (is_cond_br()) {
        instr_ir += ", ";
        instr_ir += print_as_op(this->get_operand(1), true);
        instr_ir += ", ";
        instr_ir += print_as_op(this->get_operand(2), true);
    }
    return instr_ir;
}

//& ReturnInst

ReturnInst::ReturnInst(Value *val, BasicBlock *bb) 
    : Instruction(Type::get_void_type(bb->get_module()), Instruction::ret, 1, bb) {
    set_operand(0, val);
}

ReturnInst::ReturnInst(BasicBlock *bb) 
    : Instruction(Type::get_void_type(bb->get_module()), Instruction::ret, 0, bb) {

}

ReturnInst *ReturnInst::create_ret(Value *val, BasicBlock *bb) {
    return new ReturnInst(val, bb);
}

ReturnInst *ReturnInst::create_void_ret(BasicBlock *bb) {
    return new ReturnInst(bb);
}

std::string ReturnInst::print() {
    std::string instr_ir;
    instr_ir += this->get_module()->get_instr_op_name(this->get_instr_type());
    instr_ir += " ";
    if (!is_void_ret()) {
        instr_ir += this->get_operand(0)->get_type()->print();
        instr_ir += " ";
        instr_ir += print_as_op(this->get_operand(0), false);
    } else {
        instr_ir += "void";
    }

    return instr_ir;
}

//& GetElementPtrInst
GetElementPtrInst::GetElementPtrInst(Value *ptr, std::vector<Value *> idxs, BasicBlock *bb) 
    : Instruction(PointerType::get(get_element_type(ptr, idxs)), Instruction::getelementptr, 1+idxs.size(), bb) {
    set_operand(0, ptr);
    for (int i = 0; i < idxs.size(); i++) {
        set_operand(i + 1, idxs[i]);
    }
    element_ty_ = get_element_type(ptr, idxs);
}

Type *GetElementPtrInst::get_element_type(Value *ptr, std::vector<Value *> idxs) {
    Type *ty = ptr->get_type()->get_pointer_element_type();
    assert("GetElementPtrInst ptr is wrong type" &&
           (ty->is_array_type() || ty->is_integer_type() || ty->is_float_type()));
    if (ty->is_array_type()) {
        ArrayType *arr_ty = static_cast<ArrayType *>(ty);
        for (int i = 1; i < idxs.size(); i++) {
            ty = arr_ty->get_element_type();
            if (i < idxs.size() - 1) {
                assert(ty->is_array_type() && "Index error!");
            }
            if (ty->is_array_type()) {
                arr_ty = static_cast<ArrayType *>(ty);
            }
        }
    }
    return ty;
}

GetElementPtrInst *GetElementPtrInst::create_gep(Value *ptr, std::vector<Value *> idxs, BasicBlock *bb) {
    return new GetElementPtrInst(ptr, idxs, bb);
}

std::string GetElementPtrInst::print() {
    std::string instr_ir;
    instr_ir += "%";
    instr_ir += this->get_name();
    instr_ir += " = ";
    instr_ir += this->get_module()->get_instr_op_name(this->get_instr_type());
    instr_ir += " ";
    assert(this->get_operand(0)->get_type()->is_pointer_type());
    instr_ir += this->get_operand(0)->get_type()->get_pointer_element_type()->print();
    instr_ir += ", ";
    for (int i = 0; i < this->get_num_operands(); i++) {
        if (i > 0)
            instr_ir += ", ";
        instr_ir += this->get_operand(i)->get_type()->print();
        instr_ir += " ";
        instr_ir += print_as_op(this->get_operand(i), false);
    }
    return instr_ir;
}

//& StoreInst
StoreInst::StoreInst(Value *val, Value *ptr, BasicBlock *bb)
    : Instruction(Type::get_void_type(bb->get_module()), Instruction::store, 2, bb) {
    set_operand(0, val);
    set_operand(1, ptr);
}

StoreInst *StoreInst::create_store(Value *val, Value *ptr, BasicBlock *bb) {
    return new StoreInst(val, ptr, bb);
}

std::string StoreInst::print() {
    std::string instr_ir;
    instr_ir += this->get_module()->get_instr_op_name(this->get_instr_type());
    instr_ir += " ";
    instr_ir += this->get_operand(0)->get_type()->print();
    instr_ir += " ";
    instr_ir += print_as_op(this->get_operand(0), false);
    instr_ir += ", ";
    instr_ir += print_as_op(this->get_operand(1), true);
    return instr_ir;
}

//& MemsetInst
MemsetInst::MemsetInst(Value *ptr, BasicBlock *bb)
    : Instruction(Type::get_void_type(bb->get_module()), Instruction::memset, 1, bb) {
    set_operand(0, ptr);
}

MemsetInst *MemsetInst::create_memset(Value *ptr, BasicBlock *bb) {
    return new MemsetInst(ptr, bb);
}

std::string MemsetInst::print() {
    std::string instr_ir;
    instr_ir += this->get_module()->get_instr_op_name(this->get_instr_type());
    instr_ir += " ";
    instr_ir += print_as_op(this->get_operand(0), true);
    return instr_ir;
}

//& LoadInst
LoadInst::LoadInst(Type *ty, Value *ptr, BasicBlock *bb) 
    : Instruction(ty, Instruction::load, 1, bb) {
    assert(ptr->get_type()->is_pointer_type());
    assert(ty == static_cast<PointerType *>(ptr->get_type())->get_element_type());
    set_operand(0, ptr);
}

LoadInst *LoadInst::create_load(Type *ty, Value *ptr, BasicBlock *bb) {
    return new LoadInst(ty, ptr, bb);
}

std::string LoadInst::print() {
    std::string instr_ir;
    instr_ir += "%";
    instr_ir += this->get_name();
    instr_ir += " = ";
    instr_ir += this->get_module()->get_instr_op_name(this->get_instr_type());
    instr_ir += " ";
    assert(this->get_operand(0)->get_type()->is_pointer_type());
    instr_ir += this->get_operand(0)->get_type()->get_pointer_element_type()->print();
    instr_ir += ",";
    instr_ir += " ";
    instr_ir += print_as_op(this->get_operand(0), true);
    return instr_ir;
}

//& AllocInst
AllocaInst::AllocaInst(Type *ty, BasicBlock *bb) 
    : Instruction(PointerType::get(ty), Instruction::alloca, 0, bb), alloca_ty_(ty) {

}

AllocaInst *AllocaInst::create_alloca(Type *ty, BasicBlock *bb) {
    return new AllocaInst(ty, bb);
}

std::string AllocaInst::print() {
    std::string instr_ir;
    instr_ir += "%";
    instr_ir += this->get_name();
    instr_ir += " = ";
    instr_ir += this->get_module()->get_instr_op_name(this->get_instr_type());
    instr_ir += " ";
    instr_ir += get_alloca_type()->print();
    return instr_ir;
}

//& ZextInst
ZextInst::ZextInst(OpID op, Value *val, Type *ty, BasicBlock *bb)
    : Instruction(ty, op, 1, bb), dest_ty_(ty) {
    set_operand(0, val);
}

ZextInst *ZextInst::create_zext(Value *val, Type *ty, BasicBlock *bb) {
    return new ZextInst(Instruction::zext, val, ty, bb);
}

std::string ZextInst::print() {
    std::string instr_ir;
    instr_ir += "%";
    instr_ir += this->get_name();
    instr_ir += " = ";
    instr_ir += this->get_module()->get_instr_op_name(this->get_instr_type());
    instr_ir += " ";
    instr_ir += this->get_operand(0)->get_type()->print();
    instr_ir += " ";
    instr_ir += print_as_op(this->get_operand(0), false);
    instr_ir += " to ";
    instr_ir += this->get_dest_type()->print();
    return instr_ir;
}

//& FpToSiInst
FpToSiInst::FpToSiInst(OpID op, Value *val, Type *ty, BasicBlock *bb)
    : Instruction(ty, op, 1, bb), dest_ty_(ty) {
    set_operand(0, val);
}

FpToSiInst *FpToSiInst::create_fptosi(Value *val, Type *ty, BasicBlock *bb) {
    return new FpToSiInst(Instruction::fptosi, val, ty, bb);
}

std::string FpToSiInst::print() {
    std::string instr_ir;
    instr_ir += "%";
    instr_ir += this->get_name();
    instr_ir += " = ";
    instr_ir += this->get_module()->get_instr_op_name(this->get_instr_type());
    instr_ir += " ";
    instr_ir += this->get_operand(0)->get_type()->print();
    instr_ir += " ";
    instr_ir += print_as_op(this->get_operand(0), false);
    instr_ir += " to ";
    instr_ir += this->get_dest_type()->print();
    return instr_ir;
}

//& SiToFpInst
SiToFpInst::SiToFpInst(OpID op, Value *val, Type *ty, BasicBlock *bb)
    : Instruction(ty, op, 1, bb), dest_ty_(ty) {
    set_operand(0, val);
}

SiToFpInst *SiToFpInst::create_sitofp(Value *val, Type *ty, BasicBlock *bb) {
    return new SiToFpInst(Instruction::sitofp, val, ty, bb);
}

std::string SiToFpInst::print() {
    std::string instr_ir;
    instr_ir += "%";
    instr_ir += this->get_name();
    instr_ir += " = ";
    instr_ir += this->get_module()->get_instr_op_name(this->get_instr_type());
    instr_ir += " ";
    instr_ir += this->get_operand(0)->get_type()->print();
    instr_ir += " ";
    instr_ir += print_as_op(this->get_operand(0), false);
    instr_ir += " to ";
    instr_ir += this->get_dest_type()->print();
    return instr_ir;
}

//& PhiInst
PhiInst::PhiInst(OpID op, std::vector<Value *> vals, std::vector<BasicBlock *> val_bbs, Type *ty, BasicBlock *bb) 
    : Instruction(ty, op, 2*vals.size()) {
    for (int i = 0; i < vals.size(); i++) {
        set_operand(2 * i, vals[i]);
        set_operand(2 * i + 1, val_bbs[i]);
    }
    this->set_parent(bb);
}

PhiInst *PhiInst::create_phi(Type *ty, BasicBlock *bb) {
    std::vector<Value *> vals;
    std::vector<BasicBlock *> val_bbs;
    return new PhiInst(Instruction::phi, vals, val_bbs, ty, bb);
}

std::string PhiInst::print() {
    std::string instr_ir;
    instr_ir += "%";
    instr_ir += this->get_name();
    instr_ir += " = ";
    instr_ir += this->get_module()->get_instr_op_name(this->get_instr_type());
    instr_ir += " ";
    instr_ir += this->get_operand(0)->get_type()->print();
    instr_ir += " ";
    for (int i = 0; i < this->get_num_operands() / 2; i++) {
        if (i > 0)
            instr_ir += ", ";
        instr_ir += "[ ";
        instr_ir += print_as_op(this->get_operand(2 * i), false);
        instr_ir += ", ";
        instr_ir += print_as_op(this->get_operand(2 * i + 1), false);
        instr_ir += " ]";
    }
    if (this->get_num_operands() / 2 < this->get_parent()->get_pre_basic_blocks().size()) {
        for (auto pre_bb : this->get_parent()->get_pre_basic_blocks()) {
            if (std::find(this->get_operands().begin(), this->get_operands().end(), static_cast<Value *>(pre_bb)) ==
                this->get_operands().end()) {
                // find a pre_bb is not in phi
                instr_ir += ", [ undef, " + print_as_op(pre_bb, false) + " ]";
            }
        }
    }
    return instr_ir;
}

CmpBrInst::CmpBrInst(CmpOp op, Value *lhs, Value *rhs, BasicBlock *if_true, BasicBlock *if_false, BasicBlock *bb) 
        :Instruction(Type::get_void_type(if_true->get_module()), Instruction::cmpbr, 4, bb), cmp_op_(op) {
    set_operand(0, lhs);
    set_operand(1, rhs);
    set_operand(2, if_true);
    set_operand(3, if_false);
}


CmpBrInst::CmpBrInst(CmpOp op, Value *lhs, Value *rhs, 
            BasicBlock *bb)
    : Instruction(Type::get_void_type(bb->get_module()), Instruction::cmpbr, 4, bb), cmp_op_(op) {
    set_operand(0, lhs);
    set_operand(1, rhs);
}

CmpBrInst *CmpBrInst::create_cmpbr(CmpOp op, Value *lhs, Value *rhs, BasicBlock *if_true, BasicBlock *if_false, BasicBlock *bb, Module *m) {
    if_true->add_pre_basic_block(bb);
    if_false->add_pre_basic_block(bb);
    bb->add_succ_basic_block(if_false);
    bb->add_succ_basic_block(if_true);

    return new CmpBrInst(op, lhs, rhs, if_true, if_false, bb);
}

std::string CmpBrInst::print() {
    std::string instr_ir;
    instr_ir += this->get_module()->get_instr_op_name( this->get_instr_type() );
    instr_ir += " ";
    instr_ir += print_cmp_type(this->cmp_op_);
    instr_ir += " ";
    instr_ir += this->get_operand(0)->get_type()->print();
    instr_ir += " ";
    instr_ir += print_as_op(this->get_operand(0), false);
    instr_ir += ", ";
    if (Type::is_eq_type(this->get_operand(0)->get_type(), this->get_operand(1)->get_type())) {
        instr_ir += print_as_op(this->get_operand(1), false);
    } else {
        instr_ir += print_as_op(this->get_operand(1), true);
    }
    
    instr_ir += ", ";
    instr_ir += print_as_op(this->get_operand(2), true);
    instr_ir += ", ";
    instr_ir += print_as_op(this->get_operand(3), true);
    
    return instr_ir;
}

FCmpBrInst::FCmpBrInst(CmpOp op, Value *lhs, Value *rhs, BasicBlock *if_true, BasicBlock *if_false, BasicBlock *bb) 
        :Instruction(Type::get_void_type(if_true->get_module()), Instruction::fcmpbr, 4, bb), cmp_op_(op) {
    set_operand(0, lhs);
    set_operand(1, rhs);
    set_operand(2, if_true);
    set_operand(3, if_false);
}

FCmpBrInst::FCmpBrInst(CmpOp op, Value *lhs, Value *rhs, 
            BasicBlock *bb)
    : Instruction(Type::get_void_type(bb->get_module()), Instruction::fcmpbr, 4, bb), cmp_op_(op) {
    set_operand(0, lhs);
    set_operand(1, rhs);
}

FCmpBrInst *FCmpBrInst::create_fcmpbr(CmpOp op, Value *lhs, Value *rhs, BasicBlock *if_true, BasicBlock *if_false, BasicBlock *bb, Module *m) {
    if_true->add_pre_basic_block(bb);
    if_false->add_pre_basic_block(bb);
    bb->add_succ_basic_block(if_false);
    bb->add_succ_basic_block(if_true);

    return new FCmpBrInst(op, lhs, rhs, if_true, if_false, bb);
}

std::string FCmpBrInst::print() {
    std::string instr_ir;
    instr_ir += this->get_module()->get_instr_op_name( this->get_instr_type());
    instr_ir += " ";
    instr_ir += print_fcmp_type(this->cmp_op_);
    instr_ir += " ";
    instr_ir += this->get_operand(0)->get_type()->print();
    instr_ir += " ";
    instr_ir += print_as_op(this->get_operand(0), false);
    instr_ir += ", ";
    if (Type::is_eq_type(this->get_operand(0)->get_type(), this->get_operand(1)->get_type())) {
        instr_ir += print_as_op(this->get_operand(1), false);
    } else {
        instr_ir += print_as_op(this->get_operand(1), true);
    }
    
    instr_ir += ", ";
    instr_ir += print_as_op(this->get_operand(2), true);
    instr_ir += ", ";
    instr_ir += print_as_op(this->get_operand(3), true);
    
    return instr_ir;
}

LoadOffsetInst::LoadOffsetInst(Type *ty, Value *ptr, Value *offset, BasicBlock *bb)
    : Instruction(ty, Instruction::loadoffset, 2, bb) {
    set_operand(0, ptr);
    set_operand(1, offset);
}

LoadOffsetInst::LoadOffsetInst(Type *ty, Value *ptr, BasicBlock *bb)
    : Instruction(ty, Instruction::loadoffset, 2, bb) {
    set_operand(0, ptr);
}

LoadOffsetInst *LoadOffsetInst::create_loadoffset(Type *ty, Value *ptr, Value *offset, BasicBlock *bb) {
    return new LoadOffsetInst(ty, ptr, offset, bb);
}

Type *LoadOffsetInst::get_load_type() const {
    return static_cast<PointerType *>(get_operand(0)->get_type())->get_element_type();
}

std::string LoadOffsetInst::print() {
    std::string instr_ir;
    instr_ir += "%";
    instr_ir += this->get_name();
    instr_ir += " = ";
    instr_ir += this->get_module()->get_instr_op_name( this->get_instr_type() );
    instr_ir += " ";
    instr_ir += this->get_operand(0)->get_type()->get_pointer_element_type()->print();
    instr_ir += ",";
    instr_ir += " ";
    instr_ir += print_as_op(this->get_operand(0), true);
    instr_ir += ", ";
    instr_ir += print_as_op(this->get_operand(1), true);
    return instr_ir;
}

StoreOffsetInst::StoreOffsetInst(Value *val, Value *ptr, Value *offset, BasicBlock *bb)
    : Instruction(Type::get_void_type(bb->get_module()), Instruction::storeoffset, 3, bb) {
    set_operand(0, val);
    set_operand(1, ptr);
    set_operand(2, offset);
}

StoreOffsetInst::StoreOffsetInst(Value *val, Value *ptr, BasicBlock *bb)
    : Instruction(Type::get_void_type(bb->get_module()), Instruction::storeoffset, 3, bb) {
    set_operand(0, val);
    set_operand(1, ptr);
}

Type *StoreOffsetInst::get_store_type() const {
    return static_cast<PointerType *>(get_operand(1)->get_type())->get_element_type();
}

StoreOffsetInst *StoreOffsetInst::create_storeoffset(Value *val, Value *ptr, Value *offset, BasicBlock *bb) {
    return new StoreOffsetInst(val, ptr, offset, bb);
}

std::string StoreOffsetInst::print() {
    std::string instr_ir;
    instr_ir += this->get_module()->get_instr_op_name( this->get_instr_type() );
    instr_ir += " ";
    instr_ir += this->get_operand(0)->get_type()->print();
    instr_ir += " ";
    instr_ir += print_as_op(this->get_operand(0), false);
    instr_ir += ", ";
    instr_ir += print_as_op(this->get_operand(1), true);
    instr_ir += ", ";
    instr_ir += print_as_op(this->get_operand(2), true);
    return instr_ir;
}