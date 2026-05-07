#include "simplifyInstruction.h"

ValuePtr simplifyInstructionBinary(Instruction* instr) {
    // qaq << "sim:23\n";
    // qaq << "sim:32\n";
    if(instr->insId == InsID::Binary) {
        auto bin = dynamic_cast<BinaryInstruction*>(instr);
        auto lhs = bin->getLHS();
        auto rhs = bin->getRHS();
        switch(bin->op) {
            case Add:
                // if(lhs->isConst) {
                //     bin->swapOperands();
                //     lhs = bin->a;
                //     rhs = bin->b;
                // }
                // X + 0 = 0
                if(rhs->isConst) {
                    auto crhs = dynamic_cast<Const*>(rhs);
                    if(crhs && crhs->intVal == 0)
                        return lhs;
                }
                // X + (0 - X) = 0, (0 - X) + X = 0
                if(auto slhs = dynamic_cast<BinaryInstruction*>(lhs); slhs && slhs->op == Sub) {
                    if(slhs->getLHS()->isConst) {
                        auto clhsofLhs = dynamic_cast<Const*>(slhs->getLHS());
                        if(clhsofLhs && clhsofLhs->intVal == 0 && slhs->getRHS() == rhs) {
                            auto newConst = Const::getConst(Type::getInt(), 0);
                            return newConst;
                        }
                    }
                } else if(auto srhs = dynamic_cast<BinaryInstruction*>(rhs); srhs && srhs->op == Sub) {
                    if(srhs->getLHS()->isConst) {
                        auto clhsofRhs = dynamic_cast<Const*>(srhs->getLHS());
                        if(clhsofRhs && clhsofRhs->intVal == 0 && srhs->getRHS() == lhs) {
                            auto newConst = Const::getConst(Type::getInt(), 0);
                            return newConst;
                        }
                    }
                }
                // (x - y) + (y - x) = 0
                if(auto slhs = dynamic_cast<BinaryInstruction*>(lhs); slhs && slhs->op == Sub) {
                    if(auto srhs = dynamic_cast<BinaryInstruction*>(rhs); srhs && srhs->op == Sub) {
                        if(slhs->getLHS() == srhs->getRHS() && slhs->getRHS() == srhs->getLHS()) {
                            auto newConst = Const::getConst(Type::getInt(), 0);
                            return newConst;
                        }
                    }
                }
                return bin;
            case Sub:
                // X - 0 = X
                if(rhs->isConst) {
                    if(auto crhs = dynamic_cast<Const*>(rhs); crhs && crhs->intVal == 0) {
                        return lhs;
                    }
                }
                // 0 - (0 - X) = X
                if(lhs->isConst) {
                    if(auto clhs = dynamic_cast<Const*>(lhs); clhs && clhs->intVal == 0) {
                        if(auto srhs = dynamic_cast<BinaryInstruction*>(rhs); srhs && srhs->op == Sub) {
                            if(srhs->getLHS()->isConst) {
                                if(auto clhsofRhs = dynamic_cast<Const*>(srhs->getLHS()); clhsofRhs && clhsofRhs->intVal == 0) {
                                    return srhs->getRHS();
                                }
                            }
                        }
                    }
                }
                // X - X = 0
                if(lhs == rhs) {
                    auto newConst = Const::getConst(Type::getInt(), 0);
                    return newConst;
                }
                return bin;
            case Mul:
                // X * 0 = 0 X * 1 = 1
                if(rhs->isConst) {
                    if(auto crhs = dynamic_cast<Const*>(rhs); crhs) {
                        if(crhs->intVal == 0) {
                            auto newConst = Const::getConst(Type::getInt(), 0);
                            return newConst;
                        } else if(crhs->intVal == 1) {
                            return lhs;
                        }
                    }
                }
                return bin;
            case Div:
                // X / 1 = X
                if(rhs->isConst) {
                    if(auto crhs = dynamic_cast<Const*>(rhs); crhs) {
                        if(crhs->intVal == 1) {
                            return lhs;
                        }
                    }
                }
                return bin;
                //        case FAdd:
                //            return bin->reg.get();
                //        case FSub:
                //            return bin->reg.get();
                //        case FMul:
                //            return bin->reg.get();
                //        case FDiv:
                //            return bin->reg.get();
            default:
                return bin;
        }
    } else if(instr->insId == InsID::Icmp) {
        auto bin = dynamic_cast<IcmpInstruction*>(instr);
        // qaq << "sim:177\n";
        auto lhs = bin->getLHS();
        auto rhs = bin->getRHS();
        if(bin->getOp() == "==") {
            if(lhs == rhs) {
                // cmp应当返回什么
                auto newConst = Const::getConst(Type::getBool(), true);
                return newConst;
            } else {
                return bin;
            }
        } else {
            return bin;
        }
    } else {
        return instr;
    }
}

ValuePtr simplifyGEP(Instruction* instr) {
    if(instr->insId != InsID::GEP)
        return instr;
    auto gep = dynamic_cast<GetElementPtrInstruction*>(instr);
    auto base = gep->getBase();
    if(gep->getIdx().size() == 1) {
        if(gep->getIdx()[0]->isConst) {
            auto idx = dynamic_cast<Const*>(gep->getIdx()[0]);
            if(idx->intVal == 0) {
                return base;
            }
        }
    }
    return instr;
}

bool simplifyInst(FunctionPtr func) {
    bool changed = false;
    for(auto& bb : func->getBasicBlocks()) {
        for(auto& inst : bb->instructions()) {
            if(inst->getInsID() == InsID::Binary || inst->getInsID() == InsID::Icmp) {
                auto newInst = simplifyInstructionBinary(inst);
                if(newInst != inst) {
                    // inst.reset(newInst);
                    inst->replaceWith(newInst);
                    changed = true;
                }
            } else if(inst->getInsID() == InsID::GEP) {
                auto gep = dynamic_cast<GetElementPtrInstruction*>(inst);
                auto base = gep->getBase();
                if(auto gepSource = dynamic_cast<GetElementPtrInstruction*>(gep->getBase())) {
                    bool flag = true;
                    auto srcIdx = gepSource->getIdx();
                    auto mIdx = gep->getIdx();
                    if(mIdx.size() > 3 || srcIdx.size() > 3) {
                        flag = false;
                    }
                    if(mIdx.size() >= srcIdx.size()) {
                        flag = false;
                    }
                    for(auto idx : mIdx) {
                        if(!idx->isConst) {
                            flag = false;
                            break;
                        }
                    }
                    for(auto idx : srcIdx) {
                        auto constIdx = dynamic_cast<Const*>(idx);
                        if(!constIdx || constIdx->intVal != 0) {
                            flag = false;
                            break;
                        }
                    }
                    if(flag) {
                        int srcIdxSize = srcIdx.size();
                        int idxSize = mIdx.size();
                        vector<ValuePtr> newIdx;
                        for(int i = 0; i < srcIdxSize - idxSize; i++) {
                            newIdx.push_back(srcIdx[i]);
                        }
                        for(int i = 0; i < idxSize; i++) {
                            newIdx.push_back(mIdx[i]);
                        }
                        for(int i = 1; i < gep->mOperands.size() + 1; i++) {
                            gep->rmOperand(i);
                        }
                        for(auto idx : newIdx) {
                            gep->addOperand(idx);
                        }
                        gep->setBase(gepSource->getBase());
                        changed = true;
                    }
                }
            } else if(inst->getInsID() == InsID::Ext) {
                auto ext = dynamic_cast<ExtInstruction*>(inst);
                auto src = ext->getFrom();
                if(src->isConst && src->type->isInt()) {
                    inst->replaceWith(Const::getConst(inst->type, dynamic_cast<Const*>(src)->intVal));
                    changed = true;
                }
            }
        }
    }
    return changed;
}

void runSimplifyInst(Module& mod) {
    for(auto& func : mod.getGlobalFunctions()) {
        if(func->isLib)
            continue;
        simplifyInst(func);
    }
}