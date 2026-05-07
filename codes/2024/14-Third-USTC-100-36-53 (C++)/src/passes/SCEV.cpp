#include "SCEV.hpp"
#include "Constant.hpp"
#include "LoopDetection.hpp"
#include <cassert>
#include <memory>
using namespace std;
using InstructionID = Instruction::OpID;
SCEVexp *SCEV::addSCEV(Value *val, std::unique_ptr<SCEVexp> scev) {
    auto ptr = scev.get();
    SCEVexps.push_back(std::move(scev));
    if (val)
        SCEVres.emplace(val, ptr);
    return ptr;
}

SCEVexp *SCEV::get_SCEVexp(Value *val) {
    auto iter = SCEVres.find(val);
    if (iter != SCEVres.end())
        return iter->second;
    if (dynamic_cast<ConstantInt *>(val) != nullptr) {
        auto newexp = make_unique<SCEVexp>();
        newexp->constant = dynamic_cast<ConstantInt *>(val)->get_value();
        newexp->instID = exptype::Constant;
        addSCEV(val, std::move(newexp));
    }
    return nullptr;
}
std::unique_ptr<SCEVexp> SCEV::foldAdd(SCEV &res, SCEVexp *lhs, SCEVexp *rhs) {
    if (!(lhs && rhs))
        return nullptr;
    if (lhs->instID == exptype::Constant && rhs->instID == exptype::Constant) {
        auto scev = make_unique<SCEVexp>();
        scev->instID = exptype::Constant;
        scev->constant = lhs->constant + rhs->constant;
        return scev;
    }
    if (lhs->instID == exptype::AddRec && rhs->instID == exptype::Constant) {
        auto base = lhs->operands.front();
        auto newBase = foldAdd(res, base, rhs);
        if (newBase) {
            auto ret = make_unique<SCEVexp>(*lhs);
            ret->operands.front() = newBase.get();
            res.addSCEV(nullptr, std::move(newBase));
            return ret;
        }
    }
    auto newexp = make_unique<SCEVexp>();
    newexp->instID = exptype::AddRec;
    return newexp;
}

void SCEV::run(Function *f, AnalysisPassManager &APM) {

    auto &&loops = APM.getResult<LoopDetection>(f);
    for (auto &loop : loops->get_loops()) {
        for (auto &bb : loop->get_blocks()) {
            for (auto &inst : bb->get_instructions()) {
                switch (inst.get_instr_type()) {
                case InstructionID::add: {
                    auto lhsSCEV = get_SCEVexp(inst.get_operand(0));
                    auto rhsSCEV = get_SCEVexp(inst.get_operand(1));
                    auto scev = foldAdd(*this, lhsSCEV, rhsSCEV);
                    if (scev) {
                        addSCEV(&inst, std::move(scev));
                    }
                    break;
                }
                case InstructionID::mul: {
                    // Similar logic for Mul instruction
                    break;
                }
                case InstructionID::sdiv: {
                    // Handling for SDiv instruction
                    break;
                }
                default:
                    break;
                }
            }
        }
    }
}