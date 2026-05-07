#include "middleend/value_numbering.hpp"
#include <iostream>

namespace middleend {

string ValueNumbering::get_expr_str(ir::Instruction *inst) {
    string expr_str;
    TypeCase(binary, ir::instruction::Binary *, inst) {
        expr_str = "(" + binary->getlhs()->to_str() + ")" + binary->op_str() + "(" + binary->getrhs()->to_str() + ")";
    } else TypeCase(binaryimm, ir::instruction::BinaryImm *, inst) {
        expr_str = "(" + binaryimm->getlhs()->to_str() + ")" + binaryimm->op_str() + "(" + std::to_string(binaryimm->getimm()) + ")";
    } else TypeCase(unary, ir::instruction::Unary *, inst) {
        expr_str = unary->op_str() + "(" + unary->getsrc()->to_str() + ")";
    } else TypeCase(assign, ir::instruction::Assign *, inst) {
        expr_str = assign->getsrc()->to_str();
    } else TypeCase(li, ir::instruction::LoadImm4 *, inst) {
        expr_str = li->getimm().to_string();
    } else TypeCase(call, ir::instruction::Call *, inst) {
        expr_str = "call " + call->getfunc()->get_name();
        expr_str += "(";
        for (int i = 0; i < call->get_srcs().size(); ++i) {
            expr_str += call->get_srcs()[i]->to_str();
            if (i != call->get_srcs().size() - 1) expr_str += ", ";
        }
        expr_str += ")";
    } else TypeCase(branch, ir::instruction::Branch *, inst) {
        expr_str = "branch " + std::to_string(branch->get_bb()->get_index());
    } else TypeCase(cond, ir::instruction::CondBranch *, inst) {
        expr_str = "cond " + cond->getcond()->to_str();
        expr_str += " " + std::to_string(cond->get_true_bb()->get_index());
        expr_str += " " + std::to_string(cond->get_false_bb()->get_index());
    } else TypeCase(alloca, ir::instruction::Alloca *, inst) {
        expr_str = "alloca " + alloca->getaddr()->to_str();
    } else TypeCase(store, ir::instruction::Store *, inst) {
        expr_str = "store " + store->getaddr()->to_str();
    } else TypeCase(load, ir::instruction::Load *, inst) {
        expr_str = "load " + load->getaddr()->to_str();
    } else {
        assert(false);
    }
    return expr_str;
}

bool ValueNumbering::need_swap(ir::instruction::Binary *binary) {
    if (binary->get_type() == BinaryOp::Add ||
        binary->get_type() == BinaryOp::And ||
        binary->get_type() == BinaryOp::Eq  ||
        binary->get_type() == BinaryOp::Mul ||
        binary->get_type() == BinaryOp::Neq || 
        binary->get_type() == BinaryOp::Or  ) {
        if (binary->getlhs()->get_index() > binary->getrhs()->get_index()) {
            return true;
        }
    }
    return false;
}

bool ValueNumbering::name_compress() {
    std::unordered_map<string, Temp *> first_name;
    bool changed = false;
    for (auto func : *module_->get_functions()) {
        for (auto bb : *func->get_basic_blocks()) {
            for (auto &inst : *bb->get_instructions()) {
                TypeCase(assign, ir::instruction::Assign *, inst) {
                    if (!first_name.count(assign->getsrc()->to_str())) {
                        first_name[assign->getsrc()->to_str()] = assign->getsrc();
                    }
                    first_name[assign->getdst()->to_str()] = first_name[assign->getsrc()->to_str()];
                    if (first_name[assign->getsrc()->to_str()] != assign->getsrc()) {
                        changed = true;
                    }
                    inst = new ir::instruction::Assign(assign->getdst(), first_name[assign->getsrc()->to_str()]);
                } else TypeCase(unary, ir::instruction::Unary *, inst) {
                    if (first_name.count(unary->getsrc()->to_str())) {
                        if (first_name[unary->getsrc()->to_str()] != unary->getsrc()) {
                            changed = true;
                        }
                        inst = new ir::instruction::Unary(unary->get_type(), unary->getdst(), first_name[unary->getsrc()->to_str()]);
                    }
                } else TypeCase(binary, ir::instruction::Binary *, inst) {
                    if (first_name.count(binary->getlhs()->to_str())) {
                        if (first_name[binary->getlhs()->to_str()] != binary->getlhs()) {
                            changed = true;
                        }
                        binary->setlhs(first_name[binary->getlhs()->to_str()]);
                    }
                    if (first_name.count(binary->getrhs()->to_str())) {
                        if (first_name[binary->getrhs()->to_str()] != binary->getrhs()) {
                            changed = true;
                        }
                        binary->setrhs(first_name[binary->getrhs()->to_str()]);
                    }
                    if (need_swap(binary)) {
                        changed = true;
                        binary->swap_lhs_rhs();
                    }
                } else {
                    continue;
                }
            }
        }
    }
    return changed;
}

bool ValueNumbering::lvn() {
    std::unordered_map<string, Temp *> value_numbering;
    bool changed = false;
    for (auto func : *module_->get_functions()) {
        for (auto bb : *func->get_basic_blocks()) {
            for (auto &inst : *bb->get_instructions()) {
                TypeCase(assign, ir::instruction::Assign *, inst) {
                    string expr_str = get_expr_str(assign);
                    if (value_numbering.count(expr_str)) {
                        if (value_numbering[expr_str] != assign->getsrc()) {
                            changed = true;
                        }
                        inst = new ir::instruction::Assign(assign->getdst(), value_numbering[expr_str]);
                    } else {
                        value_numbering[expr_str] = assign->getdst();
                    }
                } else TypeCase(li, ir::instruction::LoadImm4 *, inst) {
                    continue;
                } else TypeCase(unary, ir::instruction::Unary *, inst) {
                    string expr_str = get_expr_str(unary);
                    if (value_numbering.count(expr_str)) {
                        changed = true;
                        inst = new ir::instruction::Assign(unary->getdst(), value_numbering[expr_str]);
                    } else {
                        value_numbering[expr_str] = unary->getdst();
                    }
                } else TypeCase(binary, ir::instruction::Binary *, inst) {
                    string expr_str = get_expr_str(binary);
                    if (value_numbering.count(expr_str)) {
                        changed = true;
                        inst = new ir::instruction::Assign(binary->getdst(), value_numbering[expr_str]);
                    } else {
                        value_numbering[expr_str] = binary->getdst();
                    }
                } else {
                    continue;
                }
            }
        }
    }
    return changed;
}

void ValueNumbering::vn() {
    std::unordered_map<Temp *, bool> main_safe_base;
    std::unordered_map<Temp *, Temp *> reg2base;
    auto cfg = new CFG(module_->get_func_map()["main"]);
    auto uda = new UseDefAnalysis(cfg);
    for (auto bb : *module_->get_func_map()["main"]->get_basic_blocks()) {
        for (auto &inst : *bb->get_instructions()) {
            TypeCase(alloc, ir::instruction::Alloca *, inst) {
                reg2base[alloc->getaddr()] = alloc->getaddr();
                main_safe_base[alloc->getaddr()] = true;
            } else TypeCase(elementptr, ir::instruction::ElementPtr *, inst) {
                if (reg2base.count(elementptr->get_base())) {
                    reg2base[elementptr->get_dst()] = reg2base[elementptr->get_base()];
                    for (auto index : elementptr->get_indices()) {
                        TypeCase(loadimm, ir::instruction::LoadImm4 *, (*uda->get_defset(index).begin())) {

                        } else {
                            main_safe_base[reg2base[elementptr->get_base()]] = false;
                        }
                    }
                }
            } else TypeCase(call, ir::instruction::Call *, inst) {
                for (auto src : call->get_srcs()) {
                    if (reg2base.count(src)) {
                        main_safe_base[reg2base[src]] = false;
                    }
                }
            }
        }
    }

    for (auto func : *module_->get_functions()) {
        std::unordered_map<string, Temp *> global_var_addr;
        for (auto bb : *func->get_basic_blocks()) {
            std::unordered_map<string, Temp *> global_var_numbering = global_var_addr; // 全局变量标号哈希表
            std::unordered_map<string, Temp *> value_numbering; // 值标号哈希表
            for (auto &inst : *bb->get_instructions()) {
                TypeCase(assign, ir::instruction::Assign *, inst) {
                    if (value_numbering.count(assign->getsrc()->to_str())) {
                        assign->setsrc(value_numbering[assign->getsrc()->to_str()]);
                    } else {
                        value_numbering[assign->getsrc()->to_str()] = assign->getsrc();
                    }
                    value_numbering[assign->getdst()->to_str()] = value_numbering[assign->getsrc()->to_str()];
                } else TypeCase(li, ir::instruction::LoadImm4 *, inst) {
                    string expr_str = get_expr_str(li);
                    if (value_numbering.count(expr_str)) {
                        inst = new ir::instruction::Assign(li->getdst(), value_numbering[expr_str]);
                        value_numbering[li->getdst()->to_str()] = value_numbering[expr_str];
                        delete li;
                    } else {
                        value_numbering[expr_str] = li->getdst();
                        value_numbering[li->getdst()->to_str()] = li->getdst();
                    }
                } else TypeCase(unary, ir::instruction::Unary *, inst) {
                    if (value_numbering.count(unary->getsrc()->to_str())) {
                        unary->setsrc(value_numbering[unary->getsrc()->to_str()]);
                    }
                    string expr_str = get_expr_str(unary);
                    if (value_numbering.count(expr_str)) {
                        inst = new ir::instruction::Assign(unary->getdst(), value_numbering[expr_str]);
                        value_numbering[unary->getdst()->to_str()] = value_numbering[expr_str];
                        delete unary;
                    } else {
                        value_numbering[expr_str] = unary->getdst();
                        value_numbering[unary->getdst()->to_str()] = unary->getdst();
                    }
                } else TypeCase(binary, ir::instruction::Binary *, inst) {
                    if (value_numbering.count(binary->getlhs()->to_str())) {
                        binary->setlhs(value_numbering[binary->getlhs()->to_str()]);
                    }
                    if (value_numbering.count(binary->getrhs()->to_str())) {
                        binary->setrhs(value_numbering[binary->getrhs()->to_str()]);
                    }
                    if (need_swap(binary)) {
                        binary->swap_lhs_rhs();
                    }
                    string expr_str = get_expr_str(binary);
                    if (value_numbering.count(expr_str)) {
                        inst = new ir::instruction::Assign(binary->getdst(), value_numbering[expr_str]);
                        value_numbering[binary->getdst()->to_str()] = value_numbering[expr_str];
                        delete binary;
                    } else {
                        value_numbering[expr_str] = binary->getdst();
                        value_numbering[binary->getdst()->to_str()] = binary->getdst();
                    }
                } else TypeCase(binaryimm, ir::instruction::BinaryImm *, inst) {
                    if (value_numbering.count(binaryimm->getlhs()->to_str())) {
                        binaryimm->setlhs(value_numbering[binaryimm->getlhs()->to_str()]);
                    }
                    if (value_numbering.count(std::to_string(binaryimm->getimm()))) {
                        // TODO: 变成bianry指令
                    }
                    string expr_str = get_expr_str(binaryimm);
                    if (value_numbering.count(expr_str)) {
                        inst = new ir::instruction::Assign(binaryimm->getdst(), value_numbering[expr_str]);
                        value_numbering[binaryimm->getdst()->to_str()] = value_numbering[expr_str];
                        delete binaryimm;
                    } else {
                        value_numbering[expr_str] = binaryimm->getdst();
                        value_numbering[binaryimm->getdst()->to_str()] = binaryimm->getdst();
                    }
                } else TypeCase(loadaddr, ir::instruction::LoadAddr *, inst) {
                    global_var_addr[loadaddr->get_addr()->to_str()] = nullptr;
                    global_var_numbering[loadaddr->get_addr()->to_str()] = nullptr;
                } else TypeCase(alloc, ir::instruction::Alloca *, inst) {
                    global_var_addr[alloc->getaddr()->to_str()] = nullptr;
                    global_var_numbering[alloc->getaddr()->to_str()] = nullptr;
                } else TypeCase(elementptr, ir::instruction::ElementPtr *, inst) {
                    global_var_addr[elementptr->get_dst()->to_str()] = nullptr;
                    global_var_numbering[elementptr->get_dst()->to_str()] = nullptr;
                } else TypeCase(store, ir::instruction::Store *, inst) {
                    if (value_numbering.count(store->getsrc()->to_str())) {
                        store->setsrc(value_numbering[store->getsrc()->to_str()]);
                    }
                    if (value_numbering.count(store->getaddr()->to_str())) {
                        store->setaddr(value_numbering[store->getaddr()->to_str()]);
                    }
                    if (store->getoffset() == 0) {
                        if (global_var_numbering.count(store->getaddr()->to_str())) {
                            if (global_var_numbering[store->getaddr()->to_str()] != store->getsrc()) {
                                global_var_numbering[store->getaddr()->to_str()] = store->getsrc();
                            }
                        }
                    }
                } else TypeCase(load, ir::instruction::Load *, inst) {
                    if (value_numbering.count(load->getaddr()->to_str())) {
                        load->setaddr(value_numbering[load->getaddr()->to_str()]);
                    }
                    if (load->getoffset() == 0) {
                        if (global_var_numbering.count(load->getaddr()->to_str())) {
                            string expr_str = get_expr_str(load);
                            if (global_var_numbering[load->getaddr()->to_str()] != nullptr) {
                                if (func->get_name() == "main") {
                                    if (reg2base.count(load->getaddr())) {
                                        if (main_safe_base[reg2base[load->getaddr()]]) {
                                            inst = new ir::instruction::Assign(load->getdst(), global_var_numbering[load->getaddr()->to_str()]);
                                            delete load;
                                        }
                                    }
                                }
                                // inst = new ir::instruction::Assign(load->getdst(), global_var_numbering[load->getaddr()->to_str()]);
                                // delete load;
                            } else if (value_numbering.count(expr_str)) {
                                if (func->get_name() == "main") {
                                    if (reg2base.count(load->getaddr())) {
                                        if (main_safe_base[reg2base[load->getaddr()]]) {
                                            inst = new ir::instruction::Assign(load->getdst(), value_numbering[expr_str]);
                                            value_numbering[load->getdst()->to_str()] = value_numbering[expr_str];
                                            delete load;
                                        }
                                    }
                                }
                                // inst = new ir::instruction::Assign(load->getdst(), value_numbering[expr_str]);
                                // value_numbering[load->getdst()->to_str()] = value_numbering[expr_str];
                                // delete load;
                            } else {
                                value_numbering[expr_str] = load->getdst();
                                value_numbering[load->getdst()->to_str()] = load->getdst();
                            }
                        }
                    }
                } else TypeCase(ret, ir::instruction::Return *, inst) {
                    if (ret->has_return_value()) {
                        if (value_numbering.count(ret->getvalue()->to_str())) {
                            ret->setvalue(value_numbering[ret->getvalue()->to_str()]);
                        }
                    }
                } else {
                    continue;
                }
            }
        }
    }
}

void ValueNumbering::run() {
    // name_compress(); bad try
    // lvn(); bad try
    vn();
}

} // namespace middleend