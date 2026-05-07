#include "PureFuncDect.hpp"

void PureFuncDect::run() {
    // 找到所有的非纯函数
    for (auto &func : m_->get_functions()) {
        auto func_ptr = &func;
        mark_pure_func(func_ptr);
        if (not is_pure_func[func_ptr]) {
            worklist.push_back(func_ptr);
        }
    }

    // 传播非纯函数
    while (not worklist.empty()) {
        auto func_ptr = worklist.front();
        worklist.pop_front();
        propagate_non_pure_func(func_ptr);
    }
}

void PureFuncDect::mark_pure_func(Function *func) {
    if (func->is_declaration() or func->get_name() == "main") {
        is_pure_func[func] = false;
        return;
    }
    // 只要传入数组，都作为非纯函数处理
    for (auto it = func->get_function_type()->param_begin(); it != func->get_function_type()->param_end(); ++it) {
        auto arg_type = *it;
        if (arg_type->is_integer_type() == false and arg_type->is_float_type() == false) {
            is_pure_func[func] = false;
            return;
        }
    }
    for (auto &bb : func->get_basic_blocks())
        for (auto &inst : bb.get_instructions()) {
            if (is_side_effect_inst(&inst)) {
                is_pure_func[func] = false;
                return;
            }
        }
    is_pure_func[func] = true;
}

void PureFuncDect::propagate_non_pure_func(Function *func) {
    for (auto &use : func->get_use_list()) {
        auto inst = dynamic_cast<Instruction *>(use.val_);
        if (inst) {
            auto func = (inst->get_parent()->get_parent());
            if (is_pure_func[func]) {
                is_pure_func[func] = false;
                worklist.push_back(func);
            }
        } else {
            //std::cout << "Value besides instruction uses a function: " << use.val_->print() << std::endl;
        }
    }
}

bool PureFuncDect::is_side_effect_inst(Instruction *inst) {
    if (inst->is_store()) {
        if (is_local_store(dynamic_cast<StoreInst *>(inst))) {
            return false;
        } else {
            return true;
        }
    } else if (inst->is_load()) {
        if (is_local_load(dynamic_cast<LoadInst *>(inst))) {
            return false;
        } else {
            return true;
        }
    } 
    return false;
}

bool PureFuncDect::is_local_load(LoadInst *inst) {
    auto addr = dynamic_cast<Instruction *>(get_first_addr(inst->get_operand(0)));
    if (addr and addr->is_alloca()) {
        return true;
    }
    return false;
}

bool PureFuncDect::is_local_store(StoreInst *inst) {
    auto addr = dynamic_cast<Instruction *>(get_first_addr(inst->get_lval()));
    if (addr and addr->is_alloca()) {
        return true;
    }
    return false;
}

Value *PureFuncDect::get_first_addr(Value *val) {
    if (auto inst = dynamic_cast<Instruction *>(val)) {
        if (inst->is_alloca()) {
            return inst;
        } else if (inst->is_gep()) {
            return get_first_addr(inst->get_operand(0));
        } else if (inst->is_load()) {
            return val;
        } else {
            for (auto op : inst->get_operands()) {
                if (op->get_type()->is_pointer_type()) {
                    return get_first_addr(op);
                }
            }
        }
    }
    return val;
}