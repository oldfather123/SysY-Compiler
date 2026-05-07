#include "loopGepCombine.h"
#include "Function.h"
#include "IRbuilder.h"
#include "Instruction.h"
#include <queue>
#include "Type.h"
#include "Value.h"
#include "patternMatch.h"

using std::queue;

// reference: https://github.com/dtcxzyw/cmmc/blob/3888660547f82af579fb9013cdfefea52fd923f8/cmmc/Transforms/Loop/LoopGEPCombine.cpp#L4

static bool runBlock(BasicBlockPtr block) {
    std::unordered_map<size_t, std::vector<std::vector<Instruction*>>> continuousGEP;

    for(auto& inst : block->instructionsRef()) {
        if(inst->insId != InsID::GEP)
            continue;
        size_t hash = 0;
        for(uint32_t idx = 0; idx < inst->getNumOperands(); ++idx) {
            if(idx == inst->getNumOperands() - 2U)
                continue;
            hash = hash * 131 + std::hash<Value*>{}(inst->getOperand(idx));
        }

        auto& sets = continuousGEP[hash];
        bool inserted = false;
        for(auto& set : sets) {
            const auto lhs = set.front();
            if(lhs->getNumOperands() != inst->getNumOperands())
                continue;
            bool match = true;
            for(uint32_t idx = 0; idx < inst->getNumOperands(); ++idx) {
                if(idx == inst->getNumOperands() - 2U)
                    continue;

                if(lhs->getOperand(idx) != inst->getOperand(idx)) {
                    match = false;
                    break;
                }
            }
            if(match) {
                inserted = true;
                set.push_back(inst.get());
                break;
            }
        }
        if(!inserted) {
            sets.push_back({ inst.get() });
        }
    }

    struct OffsetPair final {
        Instruction* baseGEP;
        int offset;
    };

    std::unordered_map<Instruction*, OffsetPair> todo;

    for(auto& sets : continuousGEP) {
        for(auto& geps : sets.second) {
            std::queue<Instruction*> prevs;
            for(auto gep : geps) {
                while(!prevs.empty()) {
                    const auto prev = prevs.front();

                    const auto prevLast = prev->getOperand(static_cast<uint32_t>(prev->getNumOperands() - 2));
                    const auto curLast = gep->getOperand(static_cast<uint32_t>(gep->getNumOperands() - 2));

                    MatchContext<Value> matchCtx{ curLast };
                    int offset1, offset2;
                    Value *base1, *base2;
                    if(add(any(base1), int_(offset1))(matchCtx) && base1 == prevLast) {
                        todo.emplace(gep, OffsetPair{ prev, offset1 });
                        break;
                    }
                    if(add(any(base1), int_(offset1))(matchCtx) &&
                       add(any(base2), int_(offset2))(MatchContext<Value>{ prevLast }) && base1 == base2) {
                        todo.emplace(gep, OffsetPair{ prev, offset1 - offset2 });
                        break;
                    }

                    prevs.pop();
                }
                prevs.push(gep);
            }
        }
    }

    if(todo.empty())
        return false;

    IRbuilder builder(block);

    bool modified = false;
    for(auto iter = block->instructionsRef().begin(); iter != block->instructionsRef().end(); ++iter) {
        auto& inst = *iter;
        const auto it = todo.find(inst.get());
        if(it == todo.cend())
            continue;
        builder.setInsertPoint(block ,iter);
        //const auto newInst = builder.makeOp<GetElementPtrInst>(
        //    it->second.baseGEP, std::vector<Value*>{ ConstantInteger::get(builder.getIndexType(), it->second.offset) });
        auto newInst = builder.createGEP(it->second.baseGEP, std::vector<ValuePtr>{ Const::getConst(Type::getInt64(),it->second.offset) });
        modified |= inst->replaceWith(newInst);
    }
    return modified;
}

bool runLoopGepCombine(FunctionPtr func) {
    bool modified = false;
    for(auto& block : func->basicBlocks)
        modified |= runBlock(block.get());
    return modified;
}