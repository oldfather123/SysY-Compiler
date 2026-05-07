#include "constantCombine.h"
#include "Block.h"
#include "Function.h"
#include "Instruction.h"
#include "Value.h"
#include "utils.h"
#include <cassert>

bool constantCombine(FunctionPtr func) {
    bool changedFunc = false;
    int count = 0;
    cout << YELLOW << "Begin constant combine" << RESET << endl;
    for(auto it = func->getBasicBlocks().begin(); it != func->getBasicBlocks().end(); it++) {
        auto& bb = *it;
        auto changedBB = false;
        for(int i = 0; i < bb->mInstructions.size(); i++) {
            auto& inst = bb->mInstructions[i];
            bool changed = false;
            if(inst->insId == InsID::Binary) {
                auto binInst = dynamic_cast<BinaryInstruction*>(inst.get());
                if(binInst->getLHS()->isConst) {
                    if(binInst->getRHS()->isConst) {
                        auto lhs = dynamic_cast<Const*>(binInst->getLHS());
                        auto rhs = dynamic_cast<Const*>(binInst->getRHS());
                        assert(lhs && rhs);
                        if(lhs->type->isBool()) {
                            if(rhs->type->isBool()) {
                                switch(binInst->op) {
                                    case BinaryInstructionOps::Xor:
                                        binInst->replaceWith(Const::getConst(Type::getBool(), lhs->boolVal ^ rhs->boolVal));
                                        changed = true;
                                        count++;
                                        break;
                                    default:
                                        break;
                                }
                            }
                            if(rhs->type->isInt()) {
                                switch(binInst->op) {
                                    case BinaryInstructionOps::Add:
                                        binInst->replaceWith(Const::getConst(Type::getBool(), lhs->boolVal + rhs->intVal));
                                        changed = true;
                                        count++;
                                        break;
                                    case BinaryInstructionOps::Sub:
                                        binInst->replaceWith(Const::getConst(Type::getBool(), lhs->boolVal - rhs->intVal));
                                        changed = true;
                                        count++;
                                        break;
                                    case BinaryInstructionOps::Mul:
                                        binInst->replaceWith(Const::getConst(Type::getBool(), lhs->boolVal * rhs->intVal));
                                        changed = true;
                                        count++;
                                        break;
                                    case BinaryInstructionOps::Div:
                                        binInst->replaceWith(Const::getConst(Type::getBool(), lhs->boolVal / rhs->intVal));
                                        changed = true;
                                        count++;
                                        break;
                                    case BinaryInstructionOps::Rem:
                                        binInst->replaceWith(Const::getConst(Type::getBool(), lhs->boolVal % rhs->intVal));
                                        changed = true;
                                        count++;
                                        break;
                                    default:
                                        break;
                                }
                            }
                        }
                        if(lhs->type->isFloat()) {
                            assert(rhs->type->isFloat());
                            switch(binInst->op) {
                                case BinaryInstructionOps::Add:
                                    binInst->replaceWith(Const::getConst(Type::getFloat(), lhs->floatVal + rhs->floatVal));
                                    changed = true;
                                    count++;
                                    break;
                                case BinaryInstructionOps::Sub:
                                    binInst->replaceWith(Const::getConst(Type::getFloat(), lhs->floatVal - rhs->floatVal));
                                    changed = true;
                                    count++;
                                    break;
                                case BinaryInstructionOps::Mul:
                                    binInst->replaceWith(Const::getConst(Type::getFloat(), lhs->floatVal * rhs->floatVal));
                                    changed = true;
                                    count++;
                                    break;
                                case BinaryInstructionOps::Div:
                                    binInst->replaceWith(Const::getConst(Type::getFloat(), lhs->floatVal / rhs->floatVal));
                                    changed = true;
                                    count++;
                                    break;
                                case BinaryInstructionOps::FAdd:
                                    binInst->replaceWith(Const::getConst(Type::getFloat(), lhs->floatVal + rhs->floatVal));
                                    changed = true;
                                    count++;
                                    break;
                                case BinaryInstructionOps::FSub:
                                    binInst->replaceWith(Const::getConst(Type::getFloat(), lhs->floatVal - rhs->floatVal));
                                    changed = true;
                                    count++;
                                    break;
                                case BinaryInstructionOps::FMul:
                                    binInst->replaceWith(Const::getConst(Type::getFloat(), lhs->floatVal * rhs->floatVal));
                                    changed = true;
                                    count++;
                                    break;
                                case BinaryInstructionOps::FDiv:
                                    binInst->replaceWith(Const::getConst(Type::getFloat(), lhs->floatVal / rhs->floatVal));
                                    changed = true;
                                    count++;
                                    break;
                                default:
                                    break;
                            }
                            i--;
                        }
                        if(lhs->type->isInt()) {
                            assert(rhs->type->isInt());
                            switch(binInst->op) {
                                case BinaryInstructionOps::Add:
                                    binInst->replaceWith(Const::getConst(Type::getInt(), lhs->intVal + rhs->intVal));
                                    changed = true;
                                    count++;
                                    break;
                                case BinaryInstructionOps::Sub:
                                    binInst->replaceWith(Const::getConst(Type::getInt(), lhs->intVal - rhs->intVal));
                                    changed = true;
                                    count++;
                                    break;
                                case BinaryInstructionOps::Mul:
                                    binInst->replaceWith(Const::getConst(Type::getInt(), lhs->intVal * rhs->intVal));
                                    changed = true;
                                    count++;
                                    break;
                                case BinaryInstructionOps::Div:
                                    binInst->replaceWith(Const::getConst(Type::getInt(), lhs->intVal / rhs->intVal));
                                    changed = true;
                                    count++;
                                    break;
                                case BinaryInstructionOps::Rem:
                                    binInst->replaceWith(Const::getConst(Type::getInt(), lhs->intVal % rhs->intVal));
                                    changed = true;
                                    count++;
                                    break;
                                default:
                                    break;
                            }
                        }
                    }
                }
            }
            if(inst->insId == InsID::Icmp) {
                auto cmpInst = dynamic_cast<IcmpInstruction*>(inst.get());
                if(cmpInst->getLHS()->isConst) {
                    if(cmpInst->getRHS()->isConst) {
                        auto lhs = dynamic_cast<Const*>(cmpInst->getLHS());
                        auto rhs = dynamic_cast<Const*>(cmpInst->getRHS());
                        assert(lhs && rhs);
                        // assert(!lhs->type->isBool() && !rhs->type->isBool());
                        assert(lhs->type->isInt() && rhs->type->isInt());
                        switch(cmpInst->kind) {
                            case IcmpKind::ICmpEQ:
                                cmpInst->replaceWith(Const::getConst(Type::getBool(), lhs->intVal == rhs->intVal));
                                changed = true;
                                count++;
                                break;
                            case IcmpKind::ICmpNE:
                                cmpInst->replaceWith(Const::getConst(Type::getBool(), lhs->intVal != rhs->intVal));
                                changed = true;
                                count++;
                                break;
                            case IcmpKind::ICmpSGT:
                                cmpInst->replaceWith(Const::getConst(Type::getBool(), lhs->intVal > rhs->intVal));
                                changed = true;
                                count++;
                                break;
                            case IcmpKind::ICmpSGE:
                                cmpInst->replaceWith(Const::getConst(Type::getBool(), lhs->intVal >= rhs->intVal));
                                changed = true;
                                count++;
                                break;
                            case IcmpKind::ICmpSLT:
                                cmpInst->replaceWith(Const::getConst(Type::getBool(), lhs->intVal < rhs->intVal));
                                changed = true;
                                count++;
                                break;
                            case IcmpKind::ICmpSLE:
                                cmpInst->replaceWith(Const::getConst(Type::getBool(), lhs->intVal <= rhs->intVal));
                                changed = true;
                                count++;
                                break;
                            default:
                                break;
                        }
                    }
                }
            }
            if(inst->insId == InsID::Fcmp) {
                auto cmpInst = dynamic_cast<FcmpInstruction*>(inst.get());
                if(cmpInst->getLHS()->isConst) {
                    if(cmpInst->getRHS()->isConst) {
                        auto lhs = dynamic_cast<Const*>(cmpInst->getLHS());
                        auto rhs = dynamic_cast<Const*>(cmpInst->getRHS());
                        assert(lhs && rhs);
                        assert(!lhs->type->isBool() && !rhs->type->isBool());
                        assert(lhs->type->isFloat() && rhs->type->isFloat());
                        switch(cmpInst->kind) {
                            case IcmpKind::ICmpEQ:
                                cmpInst->replaceWith(Const::getConst(Type::getBool(), lhs->floatVal == rhs->floatVal));
                                changed = true;
                                count++;
                                break;
                            case IcmpKind::ICmpSGT:
                                cmpInst->replaceWith(Const::getConst(Type::getBool(), lhs->floatVal > rhs->floatVal));
                                changed = true;
                                count++;
                                break;
                            case IcmpKind::ICmpSGE:
                                cmpInst->replaceWith(Const::getConst(Type::getBool(), lhs->floatVal >= rhs->floatVal));
                                changed = true;
                                count++;
                                break;
                            case IcmpKind::ICmpSLT:
                                cmpInst->replaceWith(Const::getConst(Type::getBool(), lhs->floatVal < rhs->floatVal));
                                changed = true;
                                count++;
                                break;
                            case IcmpKind::ICmpSLE:
                                cmpInst->replaceWith(Const::getConst(Type::getBool(), lhs->floatVal <= rhs->floatVal));
                                changed = true;
                                count++;
                                break;
                            case IcmpKind::ICmpNE:
                                cmpInst->replaceWith(Const::getConst(Type::getBool(), lhs->floatVal != rhs->floatVal));
                                changed = true;
                                count++;
                                break;
                            default:
                                break;
                        }
                    }
                }
            }
            if(inst->insId == InsID::Br) {
                auto brInst = dynamic_cast<BrInstruction*>(inst.get());
                if(brInst->getCondition()) {
                    if(brInst->getCondition()->isConst) {
                        auto cond = dynamic_cast<Const*>(brInst->getCondition());
                        assert(cond);
                        assert(cond->type->isBool());
                        if(cond->boolVal) {
                            brInst->rmOperand(0);
                            brInst->falseTarget->deletePredBasicBlock(bb.get());
                            brInst->falseTarget = nullptr;
                            // changed = true; count++;
                        } else {
                            brInst->rmOperand(0);
                            brInst->trueTarget->deletePredBasicBlock(bb.get());
                            brInst->trueTarget = brInst->falseTarget;
                            brInst->falseTarget = nullptr;
                            // changed = true; count++;
                        }
                    }
                }
            }
            if(inst->insId == InsID::Ext) {
                auto ext = dynamic_cast<ExtInstruction*>(inst.get());
                auto fromVal = ext->getFrom();
                if(fromVal->isConst) {
                    {
                        ValuePtr newConst;
                        if(ext->type == Type::getInt()) {
                            newConst = Const::getConst(Type::getInt(), dynamic_cast<Const*>(fromVal)->boolVal);
                        } else if(ext->type == Type::getInt64()) {
                            newConst = Const::getConst(Type::getInt64(), dynamic_cast<Const*>(fromVal)->intVal);
                        }
                        ext->replaceWith(newConst);
                        changed = true;
                    }
                }
            }
                if(changed) {
                    changedBB = true;
                    i--;
                    inst->eraseFromParent();
                    changedFunc = true;
                }
            }
            if(changedBB) {
                it--;
            }
        }
        cout << GREEN << "Constant combined " << count << " times" << RESET << endl;
        return changedFunc;
    }