#include "optimizeBranches.h"

void forwardBranch(FunctionPtr func, BasicBlockPtr block, ValuePtr cond, bool direction, BasicBlockPtr& target) {
    if (target == block) {
        return;
    }

    BasicBlockPtr nextTarget = nullptr;
    InstructionPtr phi = nullptr;
    
    if (target->instructions.size() == 1) {
        auto last = target->instructions.front();
        if (last->type != InsID::Br) {
            return;
        }
        auto brInst = dynamic_cast<BrInstruction*>(last.get());
        if (brInst->exp != cond) {
            return;
        }
        nextTarget = direction ? func->LabelToBBMap[brInst->label1] : func->LabelToBBMap[brInst->label2];
    }
    else if (target->instructions.size() == 2) {
        phi = target->instructions.front();
        if (phi->type != InsID::Phi) {
            return;
        }
        if (phi->reg->getUseCount() != 1) {
            return;
        }
        if (!phi->reg->type->isBool()) {
            return;            
        }
        auto phiInst = dynamic_cast<PhiInstruction*>(phi.get());
        auto it = std::find_if(phiInst->from.begin(), phiInst->from.end(), [&block](const auto& pair) {
            return pair.second == block;
        });
        if (it == phiInst->from.end()) {
            return;
        }
        auto val = it->first;
        std::optional<bool> dir;
        if (val == cond) {
            dir = direction;
        }
        else if (auto constInt = dynamic_cast<Const*>(val.get())) {
            dir = (constInt->intVal != 0);
        }
        if (!dir.has_value()) {
            return;
        }
        auto last = target->instructions.back();
        if (last->type != InsID::Br) {
            return;
        }
        if (last->getOperand(0) != phi->reg) {
            return;
        }
        auto brInst = dynamic_cast<BrInstruction*>(last.get());
        nextTarget = *dir ? func->LabelToBBMap[brInst->label1] : func->LabelToBBMap[brInst->label2];
    }
    else {
        return;
    }

    if (target == nextTarget) {
        return;
    }

    auto firstInst = nextTarget->instructions.front();
    if (firstInst->type == InsID::Phi) {
        auto phiInst = std::dynamic_pointer_cast<PhiInstruction>(firstInst);
        if (std::find_if(phiInst->from.begin(), phiInst->from.end(), [&block](const auto& pair) {
            return pair.second == block;
        }) != phiInst->from.end()) {
            for (auto& inst : nextTarget->instructions) {
                if (inst->type == InsID::Phi) {
                    auto phiInst = std::dynamic_pointer_cast<PhiInstruction>(inst);
                    auto it = std::find_if(phiInst->from.begin(), phiInst->from.end(), [&target](const auto& pair) {
                        return pair.second == target;
                    });
                    if (it != phiInst->from.end()) {
                        auto val = it->first;
                        if (val->I->basicblock == target)
                            val = std::dynamic_pointer_cast<PhiInstruction>(val)->from[std::distance(phiInst->from.begin(), std::find_if(phiInst->from.begin(), phiInst->from.end(), [&block](const auto& pair) {
                                return pair.second == block;
                            }))].first;
                        if (std::find_if(phiInst->from.begin(), phiInst->from.end(), [&block, &val](const auto& pair) {
                            return pair.second == block && pair.first == val;
                        }) == phiInst->from.end()) {
                            return;
                        }
                    }
                }
                else {
                    break;
                }
            }
        }
    }
    else {
        for (auto& inst : nextTarget->instructions) {
            if (inst->type == InsID::Phi) {
                auto phiInst = std::dynamic_pointer_cast<PhiInstruction>(inst);
                auto it = std::find_if(phiInst->from.begin(), phiInst->from.end(), [&target](const auto& pair) {
                    return pair.second == target;
                });
                if (it != phiInst->from.end()) {
                    auto val = it->first;
                    if (val->I->basicblock == target)
                        val = std::dynamic_pointer_cast<PhiInstruction>(val)->from[std::distance(phiInst->from.begin(), std::find_if(phiInst->from.begin(), phiInst->from.end(), [&block](const auto& pair) {
                            return pair.second == block;
                        }))].first;
                    phiInst->addFrom(val, block);
                }
            }
            else {
                break;
            }
        }
    }

    target = nextTarget;
    if (phi) {
        std::dynamic_pointer_cast<PhiInstruction>(phi)->removeIncomingByBB(block);
    }
}

void optimizeBranches(FunctionPtr func) {
    for (auto block : func->basicBlocks) {
        auto last = block->terminator;
        if (last->type != InsID::Br) {
            continue;
        }
        auto brInst = dynamic_cast<BrInstruction*>(last.get());
        if (!brInst->exp) {
            continue;
        }
        auto& trueTarget = func->LabelToBBMap[brInst->label1];
        auto& falseTarget = func->LabelToBBMap[brInst->label2];
        if (trueTarget == falseTarget) {
            continue;
        }

        forwardBranch(func, block, brInst->exp, true, trueTarget);
        forwardBranch(func, block, brInst->exp, false, falseTarget);
    }
}