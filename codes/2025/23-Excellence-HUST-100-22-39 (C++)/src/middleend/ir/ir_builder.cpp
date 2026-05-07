#include "ir_builder.hpp"
#include "ir_instruction.hpp"
#include "ir_basic_block.hpp"
#include "ir_module.hpp"
#include "ir_type.hpp"

IRBuilder::IRBuilder(Module* module, BasicBlock* cur_bb)
    : module(module), cur_bb(cur_bb) {}

BinaryInst* IRBuilder::create_add(Value* val1, Value* val2) {
    return new BinaryInst(module->int32_type ,IRInstID::Add, val1, val2, cur_bb);
}

BinaryInst* IRBuilder::create_sub(Value* val1, Value* val2) {
    return new BinaryInst(module->int32_type, IRInstID::Sub, val1, val2, cur_bb);
}

BinaryInst* IRBuilder::create_mul(Value* val1, Value* val2) {
    return new BinaryInst(module->int32_type, IRInstID::Mul, val1, val2, cur_bb);
}

BinaryInst* IRBuilder::create_div(Value* val1, Value* val2) {
    return new BinaryInst(module->int32_type, IRInstID::Div, val1, val2, cur_bb);
}

BinaryInst* IRBuilder::create_rem(Value* val1, Value* val2) {
    return new BinaryInst(module->int32_type, IRInstID::Rem, val1, val2, cur_bb);
}

BinaryInst* IRBuilder::create_fadd(Value* val1, Value* val2) {
    return new BinaryInst(module->float_type, IRInstID::FAdd, val1, val2, cur_bb);
}

BinaryInst* IRBuilder::create_fsub(Value* val1, Value* val2) {
    return new BinaryInst(module->float_type, IRInstID::FSub, val1, val2, cur_bb);
}

BinaryInst* IRBuilder::create_fmul(Value* val1, Value* val2) {
    return new BinaryInst(module->float_type, IRInstID::FMul, val1, val2, cur_bb);
}

BinaryInst* IRBuilder::create_fdiv(Value* val1, Value* val2) {
    return new BinaryInst(module->float_type, IRInstID::FDiv, val1, val2, cur_bb);
}

BinaryInst* IRBuilder::create_shl(Value* val1, Value* val2) {
    return new BinaryInst(module->int32_type, IRInstID::Shl, val1, val2, cur_bb);
}

BinaryInst* IRBuilder::create_ashr(Value* val1, Value* val2) {
    return new BinaryInst(module->int32_type, IRInstID::AShr, val1, val2, cur_bb);
}

BinaryInst* IRBuilder::create_lshr(Value* val1, Value* val2) {
    return new BinaryInst(module->int32_type, IRInstID::LShr, val1, val2, cur_bb);
}

ICmpInst* IRBuilder::create_icmp_eq(Value* val1, Value* val2) {
    return new ICmpInst(ICmpOp::EQ, val1, val2, cur_bb);
}

ICmpInst* IRBuilder::create_icmp_ne(Value* val1, Value* val2) {
    return new ICmpInst(ICmpOp::NE, val1, val2, cur_bb);
}

ICmpInst* IRBuilder::create_icmp_gt(Value* val1, Value* val2) {
    return new ICmpInst(ICmpOp::GT, val1, val2, cur_bb);
}

ICmpInst* IRBuilder::create_icmp_ge(Value* val1, Value* val2) {
    return new ICmpInst(ICmpOp::GE, val1, val2, cur_bb);
}

ICmpInst* IRBuilder::create_icmp_lt(Value* val1, Value* val2) {
    return new ICmpInst(ICmpOp::LT, val1, val2, cur_bb);
}

ICmpInst* IRBuilder::create_icmp_le(Value* val1, Value* val2) {
    return new ICmpInst(ICmpOp::LE, val1, val2, cur_bb);
}

FCmpInst* IRBuilder::create_fcmp_eq(Value* val1, Value* val2) {
    return new FCmpInst(FCmpOp::FEQ, val1, val2, cur_bb);
}

FCmpInst* IRBuilder::create_fcmp_ne(Value* val1, Value* val2) {
    return new FCmpInst(FCmpOp::FNE, val1, val2, cur_bb);
}

FCmpInst* IRBuilder::create_fcmp_gt(Value* val1, Value* val2) {
    return new FCmpInst(FCmpOp::FGT, val1, val2, cur_bb);
}

FCmpInst* IRBuilder::create_fcmp_ge(Value* val1, Value* val2) {
    return new FCmpInst(FCmpOp::FGE, val1, val2, cur_bb);
}

FCmpInst* IRBuilder::create_fcmp_lt(Value* val1, Value* val2) {
    return new FCmpInst(FCmpOp::FLT, val1, val2, cur_bb);
}

FCmpInst* IRBuilder::create_fcmp_le(Value* val1, Value* val2) {
    return new FCmpInst(FCmpOp::FLE, val1, val2, cur_bb);
}

AllocaInst* IRBuilder::create_alloca(Type* type) {
    return new AllocaInst(type, cur_bb);
}

GetElementPtrInst* IRBuilder::create_gep(Value* ptr, vector<Value*> idxs) {
    return new GetElementPtrInst(ptr, idxs, cur_bb);
}

LoadInst* IRBuilder::create_load(Value* ptr) {
    return new LoadInst(ptr, cur_bb);
}

StoreInst* IRBuilder::create_store(Value* val, Value* ptr) {
    return new StoreInst(val, ptr, cur_bb);
}

UnaryInst* IRBuilder::create_zext(Value* val, Type* target_type) {
    return new UnaryInst(target_type, IRInstID::ZExt, val, cur_bb);
}

UnaryInst* IRBuilder::create_fptosi(Value* val, Type* target_type) {
    return new UnaryInst(target_type, IRInstID::FpToSi, val, cur_bb);
}

UnaryInst* IRBuilder::create_sitofp(Value* val, Type* target_type) {
    return new UnaryInst(target_type, IRInstID::SiToFp, val, cur_bb);
}

UnaryInst* IRBuilder::create_bitcast(Value* val, Type* target_type) {
    return new UnaryInst(target_type, IRInstID::BitCast, val, cur_bb);
}

CallInst* IRBuilder::create_call(Function* func, vector<Value*> args) {
    return new CallInst(func, args, cur_bb);
}

BranchInst* IRBuilder::create_uncond_br(BasicBlock* target_bb) {
    return new BranchInst(target_bb, cur_bb);
}

BranchInst* IRBuilder::create_cond_br(Value* cond, BasicBlock* true_bb, BasicBlock* false_bb) {
    return new BranchInst(cond, true_bb, false_bb, cur_bb);
}

ReturnInst* IRBuilder::create_ret(Value* val) {
    if(val) {
        return new ReturnInst(val, cur_bb);
    } else {
        return new ReturnInst(cur_bb);
    }
}