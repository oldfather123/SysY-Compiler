#include "instCombine.h"

void instCombine(Module& mod) {
    for(auto& func : mod.getGlobalFunctions()) {
        if(func->isLib)
            continue;
        for(auto& bb : func->getBasicBlocks()) {
            for(auto& inst : bb->instructions()) {
                // 如果是二元运算
                auto binInst = inst->as<BinaryInstruction>();
                if(binInst && binInst->getType()->isInt()) {
                    // 且右操作数是常数
                    auto constRht = binInst->getRHS()->as<Const>();
                    if(constRht && constRht->getType()->isInt()) {
                        // 如果 user 唯一
                        BinaryInstruction* user = nullptr;
                        if(binInst->hasOnlyOneUse() && (user = (*binInst->users().begin())->as<BinaryInstruction>())) {
                            // 且 user 右操作数也是常数
                            auto constRhtUser = user->getRHS()->as<Const>();
                            if(constRhtUser && constRhtUser->getType()->isInt()) {
                                // 且该指令和 user 的运算类型相同
                                if(binInst->getOp() == user->getOp()) {
                                    // 且运算符满足结合律, 则进行指令合并
                                    switch(user->op) {
                                        case Mul:
                                            user->setOperand(0, binInst->getOperand(0));
                                            user->setOperand(
                                                1, Const::getConst(Type::getInt(), constRht->intVal * constRhtUser->intVal));
                                            break;
                                        case Add:
                                            user->setOperand(0, binInst->getOperand(0));
                                            user->setOperand(
                                                1, Const::getConst(Type::getInt(), constRht->intVal + constRhtUser->intVal));
                                            break;
                                        case Xor:
                                            user->setOperand(0, binInst->getOperand(0));
                                            user->setOperand(
                                                1, Const::getConst(Type::getInt(), constRht->intVal ^ constRhtUser->intVal));
                                            break;
                                        default:
                                            break;
                                    }
                                }
                                // 或该指令和 user 的运算类型相容
                                else if(binInst->getOp() == "*" && user->getOp() == "/") {
                                    if( constRht->intVal % constRhtUser->intVal != 0){
                                        continue;
                                    }
                                    binInst->setOperand(1,
                                                        Const::getConst(Type::getInt(), constRht->intVal / constRhtUser->intVal));
                                    user->replaceWith(binInst);
                                } else if(binInst->getOp() == "+" && user->getOp() == "-") {
                                    binInst->setOperand(1,
                                                        Const::getConst(Type::getInt(), constRht->intVal - constRhtUser->intVal));
                                    user->replaceWith(binInst);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}