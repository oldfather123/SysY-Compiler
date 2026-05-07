#include "middleend/global_value_numbering.hpp"
#include <iostream>

namespace middleend {

string GlobalValueNumbering::get_expr_str(ir::Instruction *inst) {
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
    } else TypeCase(alloc, ir::instruction::Alloca *, inst) {
        expr_str = "alloca " + alloc->getaddr()->to_str();
    } else TypeCase(store, ir::instruction::Store *, inst) {
        expr_str = "store " + std::to_string(store->getoffset()) + "(" + store->getaddr()->to_str() + ")";
    } else TypeCase(load, ir::instruction::Load *, inst) {
        expr_str = "load " + std::to_string(load->getoffset()) + "(" + load->getaddr()->to_str() + ")";
    } else TypeCase(loadaddr, ir::instruction::LoadAddr *, inst) {
        expr_str = "loadaddr " + loadaddr->get_name();
    } else TypeCase(elementptr, ir::instruction::ElementPtr *, inst) {
        expr_str = "elementptr " + elementptr->get_base()->to_str() + " ";
        for (auto &index : elementptr->get_indices()) {
            expr_str += index->to_str() + " ";
        }
    } else TypeCase(arrayload, ir::instruction::ArrayLoad *, inst) {
        expr_str = "arrayload " + arrayload->getdep()->to_str() + " " + arrayload->getaddr()->to_str() + " " + std::to_string(arrayload->getoffset());
    } else TypeCase(arraystore, ir::instruction::ArrayStore *, inst) {
        
    } else {
        // TODO: 其他类型的指令
    }
    return expr_str;
}

void GlobalValueNumbering::set_srcs(ir::Instruction *inst, Temp *oldsrc, Temp *newsrc) {
    for (auto &src : *inst->get_src()) {
        if (src == oldsrc) {
            src = newsrc;
        }
    }
}

bool GlobalValueNumbering::need_swap(ir::instruction::Binary *binary) {
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

void GlobalValueNumbering::vn_changeuses(ir::Instruction *&inst) {
    TypeCase(assign, ir::instruction::Assign *, inst) {
        // if (uda_->get_defset(assign->getsrc()).size() == 1){
        //     TypeCase(phi, ir::instruction::Phi *, *(uda_->get_defset(assign->getsrc()).begin())) { // 通过phi得到的值可能会变，这里不进行传播
        //         return ; // TODO: 对phi进一步分析，更精细化处理
        //     }
        // }
        string expr_str = get_expr_str(inst);
        if (value_numbering.count(expr_str)) {
            assign->setsrc(value_numbering.at(expr_str));
        } else {
            value_numbering[expr_str] = assign->getsrc();
        }
        value_numbering[assign->getdst()->to_str()] = value_numbering.at(expr_str);
        for (auto use_inst : uda_->get_useset(assign->getdst())) {
            set_srcs(use_inst, assign->getdst(), assign->getsrc());
        }
    } else TypeCase(binary, ir::instruction::Binary *, inst) {
        auto lhs = binary->getlhs()->to_str();
        auto rhs = binary->getrhs()->to_str();
        if (value_numbering.count(lhs)) {
            binary->setlhs(value_numbering.at(lhs));
        }
        if (value_numbering.count(rhs)) {
            binary->setrhs(value_numbering.at(rhs));
        }
        if (need_swap(binary)) {
            binary->swap_lhs_rhs();
        }
        string expr_str = get_expr_str(inst);
        if (value_numbering.count(expr_str)) {
            inst = new ir::instruction::Assign(binary->getdst(), value_numbering.at(expr_str));
            for (auto use_inst : uda_->get_useset(binary->getdst())) {
                set_srcs(use_inst, binary->getdst(), value_numbering.at(expr_str));
            }
            delete binary;
        } else {
            value_numbering[expr_str] = binary->getdst();
        }
        value_numbering[inst->get_dst()->at(0)->to_str()] = value_numbering.at(expr_str);
    } else TypeCase(binaryimm, ir::instruction::BinaryImm *, inst) {
        auto lhs = binaryimm->getlhs()->to_str();
        if (value_numbering.count(lhs)) {
            binaryimm->setlhs(value_numbering.at(lhs));
        }
        string expr_str = get_expr_str(inst);
        if (value_numbering.count(expr_str)) {
            inst = new ir::instruction::Assign(binaryimm->getdst(), value_numbering.at(expr_str));
            for (auto use_inst : uda_->get_useset(binaryimm->getdst())) {
                set_srcs(use_inst, binaryimm->getdst(), value_numbering.at(expr_str));
            }
            delete binaryimm;
        } else {
            value_numbering[expr_str] = binaryimm->getdst();
        }
        value_numbering[inst->get_dst()->at(0)->to_str()] = value_numbering.at(expr_str);
    } else TypeCase(unary, ir::instruction::Unary *, inst) {
        auto src = unary->getsrc()->to_str();
        if (value_numbering.count(src)) {
            unary->setsrc(value_numbering.at(src));
        }
        string expr_str = get_expr_str(inst);
        if (value_numbering.count(expr_str)) {
            inst = new ir::instruction::Assign(unary->getdst(), value_numbering.at(expr_str));
            for (auto use_inst : uda_->get_useset(unary->getdst())) {
                set_srcs(use_inst, unary->getdst(), value_numbering.at(expr_str));
            }
            delete unary;
        } else {
            value_numbering[expr_str] = unary->getdst();
        }
        value_numbering[inst->get_dst()->at(0)->to_str()] = value_numbering.at(expr_str);
    } else TypeCase(li, ir::instruction::LoadImm4 *, inst) {
        string expr_str = get_expr_str(inst);
        if (value_numbering.count(expr_str)) {
            inst = new ir::instruction::Assign(li->getdst(), value_numbering.at(expr_str));
            for (auto use_inst : uda_->get_useset(li->getdst())) {
                set_srcs(use_inst, li->getdst(), value_numbering.at(expr_str));
            }
            delete li;
        } else {
            value_numbering[expr_str] = li->getdst();
        }
        value_numbering[inst->get_dst()->at(0)->to_str()] = value_numbering.at(expr_str);
    } else TypeCase(loadaddr, ir::instruction::LoadAddr *, inst) {
        string expr_str = get_expr_str(inst);
        if (value_numbering.count(expr_str)) {
            inst = new ir::instruction::Assign(loadaddr->get_addr(), value_numbering.at(expr_str));
            for (auto use_inst : uda_->get_useset(loadaddr->get_addr())) {
                set_srcs(use_inst, loadaddr->get_addr(), value_numbering.at(expr_str));
            }
            delete loadaddr;
        } else {
            value_numbering[expr_str] = loadaddr->get_addr();
        }
        value_numbering[inst->get_dst()->at(0)->to_str()] = value_numbering.at(expr_str);
    } else TypeCase(call, ir::instruction::Call *, inst) {
        // TODO: 优化没有外部依赖的函数调用
    } else TypeCase(branch, ir::instruction::Branch *, inst) {
        // nothing to do
    } else TypeCase(cond, ir::instruction::CondBranch *, inst) {
        // nothing to do
    } else TypeCase(alloc, ir::instruction::Alloca *, inst) {
        // nothing to do
    } else TypeCase(store, ir::instruction::Store *, inst) {
        // auto addr = store->getaddr()->to_str();
        // if (value_numbering.count(addr)) {
        //     store->setaddr(value_numbering.at(addr));
        // }
        // auto src = store->getsrc()->to_str();
        // if (value_numbering.count(src)) {
        //     store->setsrc(value_numbering.at(src));
        // }
        // string expr_str = get_expr_str(inst);
        // mem_numbering.clear();
        // mem_numbering[expr_str] = store->getsrc();
    } else TypeCase(load, ir::instruction::Load *, inst) {
        // string expr_str = get_expr_str(inst);
        // if (mem_numbering.count(expr_str)) {
        //     inst = new ir::instruction::Assign(load->getdst(), mem_numbering.at(expr_str));
        //     for (auto use_inst : uda_->get_useset(load->getdst())) {
        //         set_srcs(use_inst, load->getdst(), mem_numbering.at(expr_str));
        //     }
        //     delete load;
        // } else {
        //     mem_numbering[expr_str] = load->getdst();
        // }
    } else TypeCase(arrayload, ir::instruction::ArrayLoad *, inst) {
        auto dep = arrayload->getdep()->to_str();
        auto addr = arrayload->getaddr()->to_str();
        if (value_numbering.count(dep)) {
            arrayload->setdep(value_numbering.at(dep));
        }
        if (value_numbering.count(addr)) {
            arrayload->setaddr(value_numbering.at(addr));
        }
        string expr_str = get_expr_str(inst);
        if (value_numbering.count(expr_str)) {
            inst = new ir::instruction::Assign(arrayload->getdst(), value_numbering.at(expr_str));
            for (auto use_inst : uda_->get_useset(arrayload->getdst())) {
                set_srcs(use_inst, arrayload->getdst(), value_numbering.at(expr_str));
            }
            delete arrayload;
        } else {
            value_numbering[expr_str] = arrayload->getdst();
        }
    } else TypeCase(elementptr, ir::instruction::ElementPtr *, inst) {
        auto base = elementptr->get_base()->to_str();
        if (value_numbering.count(base)) {
            elementptr->set_base(value_numbering.at(base));
        }
        for (auto &src : *elementptr->get_src()) {
            if (value_numbering.count(src->to_str())) {
                src = value_numbering.at(src->to_str());
            }
        }
        string expr_str = get_expr_str(inst);
        if (value_numbering.count(expr_str)) {
            inst = new ir::instruction::Assign(elementptr->get_dst(), value_numbering.at(expr_str));
            for (auto use_inst : uda_->get_useset(elementptr->get_dst())) {
                set_srcs(use_inst, elementptr->get_dst(), value_numbering.at(expr_str));
            }
            delete elementptr;
        } else {
            value_numbering[expr_str] = elementptr->get_dst();
        }
    } else TypeCase(arraystore, ir::instruction::ArrayStore *, inst) {

    } else {
        // TODO: 其他类型的指令
    }
}

void GlobalValueNumbering::run() {
    for (auto bb : rpo_->order) {
        mem_numbering.clear();
        for (auto &inst : *cfg_->get_bb(bb)->get_instructions()) {
            vn_changeuses(inst);
        }
    }
}

} // namespace middleend
