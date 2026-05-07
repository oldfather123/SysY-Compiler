#include "middleend/constant_embedding.hpp"
#include "middleend/constant_propagation.hpp"
#include <iostream>

namespace middleend {

void update_phi_in_ce(ir::BasicBlock* bb, int now_bb){
    for (auto iter = bb->get_instructions()->begin(); iter != bb->get_instructions()->end(); iter++) {
        if (auto phi = dynamic_cast<ir::instruction::Phi *>(*iter)) {
            std::vector<ir::BasicBlock *> bbs;
            std::vector<Temp *> values;
            for (int i = 0; i < phi->getbbs()->size(); i++) {
                if (now_bb != phi->getbbs()->at(i)->get_index()) {
                    bbs.push_back(phi->getbbs()->at(i));
                    values.push_back(phi->getvalues()->at(i));
                }
            }
            bb->get_instructions()->erase(iter);
            if (bbs.size() == 0) {
                assert(false);
            } else if (bbs.size() == 1) {
                bb->get_instructions()->insert(iter, new ir::instruction::Assign(phi->getdst(), values[0]));
            } else {
                bb->get_instructions()->insert(iter, new ir::instruction::Phi(phi->getdst(), values, bbs));
            }
            delete phi;
        }
        else break;
    }
}

bool isPowerOfTwo(int x) {
    return x > 0 && ((x & (x-1)) == 0);
}

int log2(int x) {
    int res = 0;
    while (x >>= 1)
        res++;
    return res;
}

bool couldDoMul(int x) {
    if (isPowerOfTwo(x) || isPowerOfTwo(x+1) || isPowerOfTwo(x-1))
        return true;
    int x_ = x;
    while (x_ % 2 == 0)
		x_ >>= 1;
	if (isPowerOfTwo(x_+1) || isPowerOfTwo(x_-1)) // (2^n - 1)*(2^m)
        return true;
    x_ = x + 1;
    while (x_ % 2 == 0)
		x_ >>= 1;
	if (isPowerOfTwo(x_+1) || isPowerOfTwo(x_-1)) // (2^n - 1)*(2^m) - 1
        return true;
    x_ = x - 1;
    while (x_ % 2 == 0)
		x_ >>= 1;
	if (isPowerOfTwo(x_+1) || isPowerOfTwo(x_-1)) // (2^n - 1)*(2^m) + 1
        return true;
	return false;
}

ir::instruction::BinaryImm* convertToBinaryImm(ir::instruction::Binary *binary, ConstValue imm, bool is_int) {
    if (is_int) {
        int32_t constant_value = imm.iv;
        if (binary->get_type() == BinaryOp::Mul && couldDoMul(constant_value)) {
            auto inst = new ir::instruction::BinaryImm(BinaryOp::Mul, binary->getdst(), binary->getlhs(), ConstValue(constant_value));
            return inst;
        }
        else if (binary->get_type() == BinaryOp::Div) {
            if (isPowerOfTwo(constant_value)) {
                int shift_amount = log2(constant_value);
                auto inst = new ir::instruction::BinaryImm(BinaryOp::Shr, binary->getdst(), binary->getlhs(), ConstValue(shift_amount));
                return inst;
            }
            else {
                auto inst = new ir::instruction::BinaryImm(BinaryOp::Div, binary->getdst(), binary->getlhs(), ConstValue(constant_value));
                return inst;
            }
        }
        else if (binary->get_type() == BinaryOp::Mod && isPowerOfTwo(constant_value)) {
            int mask = constant_value - 1;
            auto inst = new ir::instruction::BinaryImm(BinaryOp::Mod, binary->getdst(), binary->getlhs(), ConstValue(mask));
            return inst;
        }
        else if (binary->get_type() == BinaryOp::Sub) {
            auto inst = new ir::instruction::BinaryImm(BinaryOp::Add, binary->getdst(), binary->getlhs(), ConstValue(-constant_value));
            return inst;
        }
        switch (binary->get_type()) {
            case BinaryOp::Add:
            case BinaryOp::And:
            case BinaryOp::Or:
                auto inst = new ir::instruction::BinaryImm(binary->get_type(), binary->getdst(), binary->getlhs(), imm);
                return inst;
        }
    }
    else
        return nullptr;
    return nullptr;
}


void ConstantEmbedding::run() {
    bool flag = true;
    while(flag){
        flag = false;
        for (auto bb : *func_->get_basic_blocks()) {
            for (auto & inst : *bb->get_instructions()) {
                if (auto loadimm = dynamic_cast<ir::instruction::LoadImm4 *>(inst)) {
                    auto dst = loadimm->getdst();
                    if(!constant_map.count(dst->get_index())) flag = true;
                    constant_map[dst->get_index()] = loadimm->getimm();
                } else if (auto assign = dynamic_cast<ir::instruction::Assign *>(inst)) {
                    auto dst = assign->getdst();
                    auto src = assign->getsrc();
                    if (constant_map.count(src->get_index())) {
                        constant_map[dst->get_index()] = constant_map[src->get_index()];
                        inst = new ir::instruction::LoadImm4(dst, constant_map[dst->get_index()]);
                        flag = true;
                        delete assign;
                    }
                } else if (auto unary = dynamic_cast<ir::instruction::Unary *>(inst)) {
                    auto dst = unary->getdst();
                    auto src = unary->getsrc();
                    if (constant_map.count(src->get_index())) {
                        constant_map[dst->get_index()] = unary->to_const(constant_map[src->get_index()]);
                        inst = new ir::instruction::LoadImm4(dst, constant_map[dst->get_index()]);
                        flag = true;
                        delete unary;
                    }
                } else if (auto binary = dynamic_cast<ir::instruction::Binary *>(inst)) {
                    auto dst = binary->getdst();
                    auto lhs = binary->getlhs();
                    auto rhs = binary->getrhs();
                    if (constant_map.count(lhs->get_index()) && constant_map.count(rhs->get_index())) {
                        constant_map[dst->get_index()] = binary->to_const(constant_map[lhs->get_index()], constant_map[rhs->get_index()]);
                        inst = new ir::instruction::LoadImm4(dst, constant_map[dst->get_index()]);
                        flag = true;
                        delete binary;
                    }
                    else if (constant_map.count(rhs->get_index())) {
                        auto imm = constant_map[rhs->get_index()];
                        auto binary_imm_instr = convertToBinaryImm(binary, imm, rhs->get_type().base_type == ScalarType::Int);
                        if (binary_imm_instr) {
                            inst = binary_imm_instr;
                            flag = true;
                            delete binary;
                        }
                    }
                    else if (constant_map.count(lhs->get_index()) && (binary->get_type() == BinaryOp::Mul || binary->get_type() == BinaryOp::Add 
                        || binary->get_type() == BinaryOp::And || binary->get_type() == BinaryOp::Or)) {
                        auto imm = constant_map[lhs->get_index()];
                        binary->setlhs(rhs);
                        binary->setrhs(lhs);
                        auto binary_imm_instr = convertToBinaryImm(binary, imm, rhs->get_type().base_type == ScalarType::Int);
                        if (binary_imm_instr) {
                            inst = binary_imm_instr;
                            flag = true;
                            delete binary;
                        }
                    }
                } else if (auto binaryimm = dynamic_cast<ir::instruction::BinaryImm *>(inst)) {
                    if (binaryimm->get_type() != BinaryOp::Mod) {
                        auto dst = binaryimm->getdst();
                        auto src = binaryimm->getlhs();
                        if (constant_map.count(src->get_index())) {
                            constant_map[dst->get_index()] = binaryimm->to_const(constant_map[src->get_index()]);
                            inst = new ir::instruction::LoadImm4(dst, constant_map[dst->get_index()]);
                            flag = true;
                            delete binaryimm;
                        }
                    }
                } 
                else if (auto condbranch = dynamic_cast<ir::instruction::CondBranch *>(inst)) {
                    auto cond = condbranch->getcond();
                    if (constant_map.count(cond->get_index())) {
                        if (constant_map[cond->get_index()] == 0 || constant_map[cond->get_index()] == 0.0f) {
                            update_phi_in_ce(condbranch->get_true_bb(), bb->get_index());
                            inst = new ir::instruction::Branch(condbranch->get_false_bb());
                        } else {
                            update_phi_in_ce(condbranch->get_false_bb(), bb->get_index());
                            inst = new ir::instruction::Branch(condbranch->get_true_bb());
                        }
                        delete condbranch;
                        flag = true;
                    }
                }
            }
        }
    }
}

}