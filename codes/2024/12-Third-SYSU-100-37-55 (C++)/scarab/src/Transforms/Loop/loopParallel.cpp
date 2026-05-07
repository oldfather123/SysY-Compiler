#include "loopParallel.h"

// 本文件中与并行化相关的部分代码，参考了2023年参赛队伍@CMMC的实现，详见README.md

int countIcmpInstructions(BasicBlockPtr bb){
    int cnt = 0;
    for(auto inst : bb->instructions){
        if(inst->type == Icmp) cnt++;
    }
    return cnt;
}

bool checkInstructionsUse(BasicBlockPtr bb, LoopPtr loop){
    auto exitBlock = *loop->exitBlocks.begin();
    for(auto inst : bb->instructions){
        if(inst->type == Br || inst->type == Store) continue; // 没有reg
        if(inst->type == Call) return false; // 顺便把这个判定做了
        if(inst.get() == loop->indexPhi) continue; // 与index有关的是可以用的
        auto useHead = inst->reg->useHead;
        int useCnt=0;
        while (useHead) {
            useCnt++;
            
            auto use = useHead->user;
            auto useBlock = use->basicblock;
            if(useBlock == exitBlock){
                bool flag = false;
                for(int i=0;i<exitBlock->instructions.size();i++){
                    if(exitBlock->instructions[i].get() == use){
                        auto useBinary = dynamic_cast<BinaryInstruction*>(use);
                        if(useBinary && useBinary->op == '+'){
                            auto nextInst = exitBlock->instructions[i+1];
                            if(nextInst->type == Store){
                                auto nextStoreInst = dynamic_cast<StoreInstruction*>(nextInst.get());
                                auto theGlobalVar = dynamic_cast<Variable*>(nextStoreInst->des.get());
                                if(theGlobalVar && theGlobalVar->isGlobal){
                                    flag = true;
                                }
                            }
                        }
                        break;
                    }
                }
                if(!flag && useCnt == 1) return false;
                useHead = useHead->next;
                continue;
            }
            if(!loop->contains(use->basicblock)) {
                if(loop->friendLoop && loop->friendLoop->contains(use->basicblock)){

                }
                else return false;
            }
            useHead = useHead->next;
        }
    }
    return true;
}

void findIndex(LoopPtr loop, vector<ValuePtr> &argv){
    // 因为partialUnrollLoop维护信息失败，所以这里要重新找
    for(auto inst : loop->header->instructions){
        if(inst->type == Icmp){
            auto icmpInst = dynamic_cast<IcmpInstruction*>(inst.get());
            if(icmpInst->a->I->type == Binary){
                // 因为做了部分循环展开，index提前+4预判了
                auto binaryInst = dynamic_cast<BinaryInstruction*>(icmpInst->a->I);
                if(binaryInst->a->I->type == Phi){
                    auto phiNode = dynamic_cast<PhiInstruction*>(binaryInst->a->I);
                    for(auto it:phiNode->from){
                        if(it.second == loop->preHeader){
                            argv.push_back(it.first);
                        }
                    }
                    loop->indexPhi = binaryInst->a->I;
                }
                else if(binaryInst->b->I->type == Phi){
                    auto phiNode = dynamic_cast<PhiInstruction*>(binaryInst->b->I);
                    for(auto it:phiNode->from){
                        if(it.second == loop->preHeader){
                            argv.push_back(it.first);
                        }
                    }
                    loop->indexPhi = binaryInst->b->I;
                }
                argv.push_back(icmpInst->b);
            }
            else if(icmpInst->b->I->type == Binary){
                // 因为做了部分循环展开，index提前+4预判了
                auto binaryInst = dynamic_cast<BinaryInstruction*>(icmpInst->b->I);
                if(binaryInst->a->I->type == Phi){
                    auto phiNode = dynamic_cast<PhiInstruction*>(binaryInst->a->I);
                    for(auto it:phiNode->from){
                        if(it.second == loop->preHeader){
                            argv.push_back(it.first);
                        }
                    }
                    loop->indexPhi = binaryInst->a->I;
                }
                else if(binaryInst->b->I->type == Phi){
                    auto phiNode = dynamic_cast<PhiInstruction*>(binaryInst->b->I);
                    for(auto it:phiNode->from){
                        if(it.second == loop->preHeader){
                            argv.push_back(it.first);
                        }
                    }
                    loop->indexPhi = binaryInst->b->I;
                }
                argv.push_back(icmpInst->a);
            }
        }
    }
}

// 将用到index的那些phi，全部换成循环的结束值
void resetIndexPhi(LoopPtr loop, vector<ValuePtr> &argv, FunctionPtr bodyFunc){
    auto indexPhi = loop->indexPhi;
    auto useHead = indexPhi->reg->useHead;
    while (useHead) {
        auto use = useHead->user;
        if(use->basicblock->parentFunction != bodyFunc){
            use->replaceOperand(indexPhi->reg, argv[1]);
        }
        useHead = useHead->next;
    }
    //replaceVarByVar(indexPhi->reg, argv[1]);
}


void findAllGEPIndex(BasicBlockPtr bb, unordered_set<ValuePtr> &indexVal){
    for(auto inst : bb->instructions){
        if(inst->type == GEP){
            auto gepInst = dynamic_cast<GetElementPtrInstruction*>(inst.get());
            for(auto it:gepInst->index){
                indexVal.insert(it);
                auto anotherInst = it->I;
                if(anotherInst && anotherInst->type == Ext){
                    auto extInst = dynamic_cast<ExtInstruction*>(anotherInst);
                    auto fromVal = extInst->from;
                    indexVal.insert(fromVal);
                }
            }
        }
    }
}

void findAllIndexExt(BasicBlockPtr bb, unordered_set<ValuePtr> &indexVal, unordered_set<ValuePtr> &indexExt){
    unordered_set<ValuePtr> indexBinary;
    for(auto inst : bb->instructions){
        if(inst->type == Binary){
            auto binaryInst = dynamic_cast<BinaryInstruction*>(inst.get());
            if(indexVal.count(binaryInst->a) || indexVal.count(binaryInst->b)){
                indexBinary.insert(binaryInst->reg);
            }
        }
        if(inst->type == Ext){
            auto extInst = dynamic_cast<ExtInstruction*>(inst.get());
            if(indexBinary.count(extInst->from)){
                indexExt.insert(extInst->reg);
            }
        }
    }
}



bool checkDependence(LoopPtr loop){
    // 如果说x[i] = x[i-1]这种，就是有依赖的
    auto header = loop->header;
    auto latch = loop->latchBlock;
    unordered_set<ValuePtr> indexVal;
    findAllGEPIndex(header, indexVal);
    findAllGEPIndex(latch, indexVal);
    unordered_set<ValuePtr> indexExt;
    findAllIndexExt(header, indexVal, indexExt);
    findAllIndexExt(latch, indexVal, indexExt);

    for(auto inst : header->instructions){
        if(inst->type == GEP){
            auto gepInst = dynamic_cast<GetElementPtrInstruction*>(inst.get());
            for(auto it:gepInst->index){
                if(indexExt.count(it)) return true;
            }
        }
    }
    for(auto inst : latch->instructions){
        if(inst->type == GEP){
            auto gepInst = dynamic_cast<GetElementPtrInstruction*>(inst.get());
            for(auto it:gepInst->index){
                if(indexExt.count(it)) return true;
            }
        }
    }
    return false;
}


bool makeParallelLoop(LoopPtr loop, FunctionPtr func, Module &ir, std::vector<FunctionPtr> &funcToInsert){
    if(!loop->hasPartiallyUnroll) return false;

    // 检查icmp指令
    if(countIcmpInstructions(loop->header) + countIcmpInstructions(loop->latchBlock) != 1) return false;

    // 提前一点先把indexPhi找出来
    vector<ValuePtr> argv;
    findIndex(loop, argv);

    // 检查两个块中的所有指令，他们的reg都不能被循环外的指令使用
    if(!checkInstructionsUse(loop->header, loop)) return false;
    if(!checkInstructionsUse(loop->latchBlock, loop)) return false;
    auto friendLoop = loop->friendLoop;
    if(!checkInstructionsUse(friendLoop->header, friendLoop)) return false;
    if(!checkInstructionsUse(friendLoop->latchBlock, friendLoop)) return false;
    if(checkDependence(friendLoop)) return false;

    auto bodyFunc = loopExtract(loop, ir);
    if(bodyFunc == nullptr) return false;
    funcToInsert.push_back(bodyFunc);

    resetIndexPhi(loop->friendLoop, argv, bodyFunc);

    // 准备好函数的参数
    auto preHeader = loop->preHeader;
    
    
    // auto phiNode = dynamic_cast<PhiInstruction*>(loop->indexPhi);
    // for(auto it : phiNode->from){
    //     if(it.second == preHeader){
    //         argv.push_back(it.first); // 循环的起始
    //     }
    // }
    
    //argv.push_back(loop->loopEnd);
    assert(argv.size() == 2 && "loopParallel:传入的函数不是2个参数的");

    // 取消preHeader向loop的跳转，改为call
    auto exitBlock = *loop->friendLoop->exitBlocks.begin();
    preHeader->instructions.pop_back();
    preHeader->terminator = nullptr;
    auto callInst = std::make_shared<CallInstruction>(bodyFunc, argv, preHeader);
    preHeader->appendInstruction(callInst);
    auto brInst = std::make_shared<BrInstruction>(exitBlock->label, preHeader);
    preHeader->appendInstruction(brInst);
    preHeader->setTerminator(brInst);
    
    //processAtomicAdd(loop->friendLoop, bodyFunc, );

    return true;
}


bool loopParallel(FunctionPtr func, Module &ir, std::vector<FunctionPtr> &funcToInsert) {
    queue<LoopPtr> workQueue;
    bool hasParallel = false;
    for (auto loop : func->loopList) {
        if (loop->subLoops.size() == 0) {
            workQueue.push(loop);
        }
    }

    while (!workQueue.empty()) {
        auto loop = workQueue.front();
        workQueue.pop();
        
        hasParallel |= makeParallelLoop(loop, func, ir, funcToInsert);
    }
    return hasParallel;
}