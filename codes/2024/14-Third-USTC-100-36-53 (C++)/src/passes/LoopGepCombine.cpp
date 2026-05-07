#include "LoopGepCombine.hpp"
#include "BasicBlock.hpp"
#include "Constant.hpp"
#include "Dominators.hpp"
#include "Function.hpp"
#include "Instruction.hpp"
#include <PatternMatch.hpp>
#include <cassert>
#include <queue>

#include "LoopDetection.hpp"
#include "Value.hpp"

void LoopGepCombine::run_on_block(BasicBlock *bb) {
    using namespace PatternMatch;
    // classify gep
    std::unordered_map<int, std::vector<std::vector<Instruction *>>> match_gep;
    for (auto &inst1 : bb->get_instructions()) {
        auto inst = &inst1;
        if (not inst->is_gep())
            continue;
        int hash = 0;
        for (int idx = 0; idx < inst->get_num_operand() - 1; ++idx) {
            hash = hash * 131 + std::hash<Value *>{}(inst->get_operand(idx));
        }

        auto &vec = match_gep[hash];
        bool match = false;
        for (auto &geps : vec) {
            bool flag = true;
            auto gep = geps.front();
            if (gep->get_num_operand() != inst->get_num_operand())
                continue;
            for (auto idx = 0; idx < gep->get_num_operand() - 1; idx++) {
                if (gep->get_operand(idx) != inst->get_operand(idx)) {
                    flag = false;
                    break;
                }
            }
            if (flag) {
                geps.push_back(inst);
                match = true;
                break;
            }
        }
        if (!match) {
            vec.push_back({inst});
        }
    }
    // find
    for (auto &hashs : match_gep) {
        for (auto &vecs : hashs.second) {
            std::queue<Instruction *> matched;
            for (auto &inst : vecs) {
                while (!matched.empty()) {
                    auto prev = matched.front();

                    auto prevLast =
                        prev->get_operand(prev->get_num_operand() - 1);
                    auto curLast =
                        inst->get_operand(inst->get_num_operand() - 1);

                    Value *offset1, *offset2;
                    Value *base1, *base2;
                    if (match(curLast,
                              m_add(m_value(base1), m_value(offset1))) and
                        base1 == prevLast and
                        dynamic_cast<ConstantInt *>(offset1)) {
                        work_list[inst] = gep_pair{
                            prev,
                            dynamic_cast<ConstantInt *>(offset1)->get_value()};
                        break;
                    }
                    if (match(curLast,
                              m_add(m_value(base1), m_value(offset1))) and
                        match(prevLast,
                              m_add(m_value(base2), m_value(offset2))) and
                        base1 == base2 and
                        dynamic_cast<ConstantInt *>(offset1) and
                        dynamic_cast<ConstantInt *>(offset2)) {
                        work_list[inst] = gep_pair{
                            prev,
                            dynamic_cast<ConstantInt *>(offset1)->get_value() -
                                dynamic_cast<ConstantInt *>(offset2)
                                    ->get_value()};
                        break;
                    }
                    if (dynamic_cast<ConstantInt *>(curLast) != nullptr and
                        dynamic_cast<ConstantInt *>(prevLast) != nullptr) {
                        work_list[inst] = gep_pair{
                            prev,
                            dynamic_cast<ConstantInt *>(curLast)->get_value() -
                                dynamic_cast<ConstantInt *>(prevLast)
                                    ->get_value()};
                    }
                    matched.pop();
                }
                matched.push(inst);
            }
        }
    }
    if (work_list.empty())
        return;
    // combine
    for (auto iter = bb->get_instructions().begin();
         iter != bb->get_instructions().end(); ++iter) {
        auto &inst = *iter;
        auto it = work_list.find(&inst);
        if (it == work_list.end())
            continue;
        auto gep = dynamic_cast<GetElementPtrInst *>(&inst);
        auto size = gep->get_element_type()->get_size();
        inst.remove_all_operands();
        auto base = (*it).second.base;
        auto offset = (*it).second.idx;
        inst.add_operand(base);
        auto offset1 = ConstantInt::get(offset, bb->get_module());
        inst.add_operand(offset1);
    }
}

void LoopGepCombine::run(Function *func, AnalysisPassManager &APM) {
    for (auto &bb : func->get_basic_blocks()) {
        run_on_block(&bb);
    }
}