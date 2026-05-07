#include <iostream>
#include "ir_instruction.hpp"
#include "ir_basic_block.hpp"
#include "ir_function.hpp"
#include "ir_module.hpp"
#include "ir_type.hpp"

// ------------------------------------------  构造函数  ------------------------------------------

Instruction::Instruction(Type* type, IRInstID iid, int num_ops, BasicBlock* parent, InsertPos position)
    : Value(type, ""), iid(iid) {
    operands.resize(num_ops);
    use_pos.resize(num_ops);
    switch(position) {
        case InsertPos::None:
            this->parent = parent;
            break;
        case InsertPos::Head:
            parent->add_inst_front(this);
            break;
        case InsertPos::Tail:
            parent->add_inst_back(this);
            break;
        default: {
            cerr << "[ERROR] Instruction::Instruction: Invalid InsertPos!" << endl;
            exit(1);
        }
    }
}

BinaryInst::BinaryInst(Type* type, IRInstID iid, Value* val1, Value* val2, BasicBlock* bb, InsertPos position, bool is_clone)
    : Instruction(type, iid, 2, bb, position) {
    if(!is_clone) {
        set_op(0, val1);
        set_op(1, val2);
    } else {
        operands[0] = val1;
        operands[1] = val2;
    }
}

ICmpInst::ICmpInst(ICmpOp icmp_op, Value* val1, Value* val2, BasicBlock* bb, InsertPos position, bool is_clone)
    : Instruction(bb->parent->parent->int1_type, IRInstID::ICmp, 2, bb, position), icmp_op(icmp_op) {
    if(!is_clone) {
        set_op(0, val1);
        set_op(1, val2);
    } else {
        operands[0] = val1;
        operands[1] = val2;
    }
}

FCmpInst::FCmpInst(FCmpOp fcmp_op, Value* val1, Value* val2, BasicBlock* bb, InsertPos position, bool is_clone)
    : Instruction(bb->parent->parent->int1_type, IRInstID::FCmp, 2, bb, position), fcmp_op(fcmp_op) {
    if(!is_clone) {
        set_op(0, val1);
        set_op(1, val2);
    } else {
        operands[0] = val1;
        operands[1] = val2;
    }
}

AllocaInst::AllocaInst(Type* type, BasicBlock* bb, InsertPos position, bool is_clone)
    : Instruction(bb->parent->parent->get_pointer_type(type), IRInstID::Alloca, 0, bb, position), alloca_type(type) {
}

GetElementPtrInst::GetElementPtrInst(Value* ptr, vector<Value*> idxs, BasicBlock* bb, InsertPos position, bool is_clone)
    : Instruction(bb->parent->parent->get_pointer_type(get_return_type(ptr, idxs.size())), IRInstID::GetElementPtr, idxs.size() + 1, bb, position),
      has_base(false), base(nullptr), offset(0) {
    if(!is_clone) {
        set_op(0, ptr);
        for (int i = 0; i < idxs.size(); ++i) {
            set_op(i + 1, idxs[i]);
        }
    } else {
        operands[0] = ptr;
        for (int i = 0; i < idxs.size(); ++i) {
            operands[i + 1] = idxs[i];
        }
    }
}

LoadInst::LoadInst(Value* ptr, BasicBlock* bb, InsertPos position, bool is_clone)
    : Instruction(static_cast<PointerType*>(ptr->type)->pointed_type, IRInstID::Load, 1, bb, position) {
    if(!is_clone) {
        set_op(0, ptr);
    } else {
        operands[0] = ptr;
    }
}

StoreInst::StoreInst(Value* val, Value* ptr, BasicBlock* bb, InsertPos position, bool is_clone)
    : Instruction(bb->parent->parent->void_type, IRInstID::Store, 2, bb, position) {
    if(!is_clone) {
        set_op(0, val);
        set_op(1, ptr);
    } else {
        operands[0] = val;
        operands[1] = ptr;
    }
}

UnaryInst::UnaryInst(Type* type, IRInstID iid, Value* val, BasicBlock* bb, InsertPos position, bool is_clone)
    : Instruction(type, iid, 1, bb, position), target_type(type) {
    if(!is_clone) {
        set_op(0, val);
    } else {
        operands[0] = val;
    }
}

CallInst::CallInst(Function* func, vector<Value*> args, BasicBlock* bb, InsertPos position, bool is_clone)
    : Instruction(static_cast<FunctionType*>(func->type)->ret_type, IRInstID::Call, args.size() + 1, bb, position) {
    if(!is_clone) {
        for (int i = 0; i < args.size(); ++i) {
            set_op(i, args[i]);
        }
        set_op(args.size(), func);
    } else {
        for (int i = 0; i < args.size(); ++i) {
            operands[i] = args[i];
        }
        operands[args.size()] = func;
    }
}

BranchInst::BranchInst(BasicBlock* target_bb, BasicBlock* bb, InsertPos position, bool is_clone)
    : Instruction(target_bb->parent->parent->void_type, IRInstID::Br, 1, bb, position) {
    if(!is_clone) {
        set_op(0, target_bb);
        target_bb->add_pre_basic_block(bb);
        bb->add_succ_basic_block(target_bb);
    } else {
        operands[0] = target_bb;
    }
}

BranchInst::BranchInst(Value* cond, BasicBlock* true_bb, BasicBlock* false_bb, BasicBlock* bb, InsertPos position, bool is_clone)
    : Instruction(true_bb->parent->parent->void_type, IRInstID::Br, 3, bb, position) {
    if(!is_clone) {
        set_op(0, cond);
        set_op(1, true_bb);
        set_op(2, false_bb);
        true_bb->add_pre_basic_block(bb);
        false_bb->add_pre_basic_block(bb);
        bb->add_succ_basic_block(true_bb);
        bb->add_succ_basic_block(false_bb);
    } else {
        operands[0] = cond;
        operands[1] = true_bb;
        operands[2] = false_bb;
    }
}

ReturnInst::ReturnInst(BasicBlock* bb, InsertPos position, bool is_clone)
    : Instruction(bb->parent->parent->void_type, IRInstID::Ret, 0, bb, position) {
}

ReturnInst::ReturnInst(Value* val, BasicBlock* bb, InsertPos position, bool is_clone)
    : Instruction(bb->parent->parent->void_type, IRInstID::Ret, 1, bb, position) {
    if(!is_clone) {
        set_op(0, val);
    } else {
        operands[0] = val;
    }
}

PhiInst::PhiInst(Type* type, vector<Value*> vals, vector<BasicBlock*> val_bbs, BasicBlock* bb, InsertPos position, bool is_clone)
    : Instruction(type, IRInstID::Phi, vals.size() + val_bbs.size(), bb, position) {
    if(!is_clone) {
        for (int i = 0; i < vals.size(); ++i) {
            set_op(i * 2, vals[i]);
            set_op(i * 2 + 1, val_bbs[i]);
        }
    } else {
        for (int i = 0; i < vals.size(); ++i) {
            operands[i * 2] = vals[i];
            operands[i * 2 + 1] = val_bbs[i];
        }
    }
}

// ------------------------------------------  功能函数  ------------------------------------------
Value* Instruction::get_op(int index) {
    if(index < 0 || index >= operands.size()) { // 索引越界
        cerr << "[ERROR] Instruction::get_op: index out of bounds!" << endl;
        exit(1);
    }
    return operands[index];
}

void Instruction::set_op(int index, Value* val) {
    if(index < 0 || index >= operands.size()) { // 索引越界
        cerr << "[ERROR] Instruction::set_op: index out of bounds!" << endl;
        exit(1);
    }
    if(val) {
        operands[index] = val;
        use_pos[index] = val->add_use(this, index);
    } else {
        cerr << "[ERROR] Instruction::set_op: cannot set null value!" << endl;
        exit(1);
    }
}

void Instruction::replace_op(int index, Value* new_val) {
    if(index < 0 || index >= operands.size()) { // 索引越界
        cerr << "[ERROR] Instruction::replace_op: index out of bounds!" << endl;
        exit(1);
    }
    if(new_val) {
        if(!dynamic_cast<Const*>(operands[index])) {
            operands[index]->remove_use(use_pos[index]); // 移除操作数的 Use 关系
        }
        operands[index] = new_val;
        use_pos[index] = new_val->add_use(this, index);
    } else {
        cerr << "[ERROR] Instruction::replace_op: cannot replace with null value!" << endl;
        exit(1);
    }
}

void Instruction::remove_use_of_ops() {
    for(int i = 0; i < operands.size(); ++i) {
        if(!dynamic_cast<Const*>(operands[i])) {
            operands[i]->remove_use(use_pos[i]); // 移除操作数的 Use 关系
        }
    }
}

Type* GetElementPtrInst::get_return_type(Value* ptr, int idxs_size) {
    Type* cur_type = static_cast<PointerType*>(ptr->type)->pointed_type;
    if(cur_type->tid == TypeID::ArrayTy) {
        auto arr_type = static_cast<ArrayType*>(cur_type);
        for(int i = 1; i < idxs_size; ++i) {
            cur_type = arr_type->element_type;
            if(cur_type->tid == TypeID::ArrayTy) {
                arr_type = static_cast<ArrayType*>(cur_type);
            }
        }
    }
    return cur_type;
}

void PhiInst::add_ops(Value* val, BasicBlock* bb) {
    if(val->type != type) {
        cerr << "[ERROR] PhiInst::add_operand: value type does not match!" << endl;
        exit(1);
    }
    operands.push_back(val);
    use_pos.emplace_back(val->add_use(this, operands.size() - 1));
    operands.push_back(bb);
    use_pos.emplace_back(bb->add_use(this, operands.size() - 1));
}

void PhiInst::remove_ops(int index_1, int index_2) {
    if(index_1 < 0 || index_1 >= operands.size() || index_2 < 0 || index_2 >= operands.size()) {
        cerr << "[ERROR] PhiInst::remove_ops: index out of bounds!" << endl;
        exit(1);
    }
    if(index_1 == index_2) {
        cerr << "[ERROR] PhiInst::remove_ops: cannot remove the same operand twice!" << endl;
        exit(1);
    }
    for(int i = index_1; i <= index_2; ++i) {
        if(!dynamic_cast<Const*>(operands[i])) {
            operands[i]->remove_use(use_pos[i]); // 移除操作数的 Use 关系
        }
    }
    for(int i = index_2 + 1; i < operands.size(); ++i) {
        for(auto& use: operands[i]->use_list) {
            if(use.val == this) {
                use.index -= index_2 - index_1 + 1; // 更新use的索引
                break; // 找到后就可以退出了
            }
        }
    }
    operands.erase(operands.begin() + index_1, operands.begin() + index_2 + 1);
    use_pos.erase(use_pos.begin() + index_1, use_pos.begin() + index_2 + 1);
}

// ------------------------------------------  克隆函数  ------------------------------------------
Instruction* BinaryInst::clone(BasicBlock* parent) {
    return new BinaryInst(type, iid, get_op(0), get_op(1), parent, InsertPos::None, true);
}

Instruction* ICmpInst::clone(BasicBlock* parent) {
    return new ICmpInst(icmp_op, get_op(0), get_op(1), parent, InsertPos::None, true);
}

Instruction* FCmpInst::clone(BasicBlock* parent) {
    return new FCmpInst(fcmp_op, get_op(0), get_op(1), parent, InsertPos::None, true);
}

Instruction* AllocaInst::clone(BasicBlock* parent) {
    return new AllocaInst(alloca_type, parent, InsertPos::None, true);
}

Instruction* GetElementPtrInst::clone(BasicBlock* parent) {
    Value* base = get_op(0);
    vector<Value*> vals(operands.begin() + 1, operands.end());
    auto new_gep = new GetElementPtrInst(base, vals, parent, InsertPos::None, true);
    new_gep->has_base = has_base;
    new_gep->base = this->base;
    new_gep->offset = offset;
    return new_gep;
}

Instruction* LoadInst::clone(BasicBlock* parent) {
    return new LoadInst(get_op(0), parent, InsertPos::None, true);
}

Instruction* StoreInst::clone(BasicBlock* parent) {
    return new StoreInst(get_op(0), get_op(1), parent, InsertPos::None, true);
}

Instruction* UnaryInst::clone(BasicBlock* parent) {
    return new UnaryInst(type, iid, get_op(0), parent, InsertPos::None, true);
}

Instruction* CallInst::clone(BasicBlock* parent) {
    // TODO: 理论上不会到这里，因为当前阶段统一认为函数调用会有副作用
    vector<Value*> args(operands.begin(), operands.end() - 1);
    Function* callee = static_cast<Function*>(get_op(operands.size() - 1));
    return new CallInst(callee, args, parent, InsertPos::None, true);
}

Instruction* BranchInst::clone(BasicBlock* parent) {
    if(operands.size() == 1) {
        return new BranchInst(static_cast<BasicBlock*>(get_op(0)), parent, InsertPos::None, true);
    } else if(operands.size() == 3) {
        return new BranchInst(get_op(0), static_cast<BasicBlock*>(get_op(1)), static_cast<BasicBlock*>(get_op(2)), parent, InsertPos::None, true);
    } else {
        cerr << "[ERROR] BranchInst::clone: invalid number of operands!" << endl;
        exit(1);
    }
}

Instruction* ReturnInst::clone(BasicBlock* parent) {
    if(operands.size() == 0) {
        return new ReturnInst(parent, InsertPos::None, true);
    } else {
        return new ReturnInst(get_op(0), parent, InsertPos::None, true);
    }
}

Instruction* PhiInst::clone(BasicBlock* parent) {
    vector<Value*> vals;
    vector<BasicBlock*> val_bbs;
    for(int i = 0; i < operands.size(); i += 2) {
        vals.push_back(get_op(i));
        val_bbs.push_back(static_cast<BasicBlock*>(get_op(i + 1)));
    }
    return new PhiInst(type, vals, val_bbs, parent, InsertPos::None, true);
}

// ------------------------------------------  快速类型判断  ------------------------------------------
bool Instruction::is_add() { return iid == IRInstID::Add; }
bool Instruction::is_sub() { return iid == IRInstID::Sub; }
bool Instruction::is_mul() { return iid == IRInstID::Mul; }
bool Instruction::is_div() { return iid == IRInstID::Div; }
bool Instruction::is_rem() { return iid == IRInstID::Rem; }
bool Instruction::is_fadd() { return iid == IRInstID::FAdd; }
bool Instruction::is_fsub() { return iid == IRInstID::FSub; }
bool Instruction::is_fmul() { return iid == IRInstID::FMul; }
bool Instruction::is_fdiv() { return iid == IRInstID::FDiv; }
bool Instruction::is_shl() { return iid == IRInstID::Shl; }
bool Instruction::is_ashr() { return iid == IRInstID::AShr; }
bool Instruction::is_lshr() { return iid == IRInstID::LShr; }
bool Instruction::is_icmp() { return iid == IRInstID::ICmp; }
bool Instruction::is_fcmp() { return iid == IRInstID::FCmp; }
bool Instruction::is_alloca() { return iid == IRInstID::Alloca; }
bool Instruction::is_gep() { return iid == IRInstID::GetElementPtr; }
bool Instruction::is_load() { return iid == IRInstID::Load; }
bool Instruction::is_store() { return iid == IRInstID::Store; }
bool Instruction::is_zext() { return iid == IRInstID::ZExt; }
bool Instruction::is_fptosi() { return iid == IRInstID::FpToSi; }
bool Instruction::is_sitofp() { return iid == IRInstID::SiToFp; }
bool Instruction::is_bitcast() { return iid == IRInstID::BitCast; }
bool Instruction::is_call() { return iid == IRInstID::Call; }
bool Instruction::is_br() { return iid == IRInstID::Br; }
bool Instruction::is_ret() { return iid == IRInstID::Ret; }
bool Instruction::is_phi() { return iid == IRInstID::Phi; }
bool Instruction::is_binary() {
    return is_add() || is_sub() || is_mul() || is_div() || is_rem() ||
           is_fadd() || is_fsub() || is_fmul() || is_fdiv() ||
           is_shl() || is_ashr() || is_lshr();
}
bool Instruction::is_terminator() {
    return is_br() || is_ret();
}