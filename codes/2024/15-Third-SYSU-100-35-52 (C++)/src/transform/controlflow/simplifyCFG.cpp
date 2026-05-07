#include "simplifyCFG.h"
#include "Block.h"
#include "CFGAnalysis.h"
#include "Function.h"
// TODO : wjq to write
bool removeUnreachableBlocks(FunctionPtr func) {
    //    vector<BasicBlockPtr> newBB;
    bool change = false;
    for(int i = 0; i < func->basicBlocks.size(); i++) {
        auto bb = func->basicBlocks[i].get();
        if(bb == func->getEntryBlock() || !bb->getPredecessor().empty()) {
        } else {
            for(int j = 0; j < bb->succBasicBlocks.size(); j++) {
                //    succ->predBasicBlocks.erase(func->basicBlocks[i]);
                auto succ = bb->succBasicBlocks[j];
                succ->deletePredBasicBlock(bb);
                cout << "Removed unreachable bb:" << bb->getName() << endl;
            }
            bb->eraseFromParent();
            change = true;
            i--;
        }
    }
    // TODO: solve Phi instruction
    return change;
}

bool deleteOneFromOneToBB(FunctionPtr func) {
    bool change = false;
    for(int i = 0; i < func->basicBlocks.size(); i++) {
        auto bb = func->basicBlocks[i].get();
        if(bb->predBasicBlocks.size() == 1) {
            for(auto pred : bb->predBasicBlocks) {
                if(auto bI = dynamic_cast<BrInstruction*>(pred->getTerminator())) {
                    if(!bI->getCondition()) {
                        cout << "Merge bb: " << bb->getName() << " to " << pred->getName() << endl;
                        mergeBasicBlock(pred, bb);
                        change = true;
                        i--;
                    }
                }
            }
        }
    }
    return change;
}

bool replaceEmptyBB(FunctionPtr func) {
    bool changedBB = false;
    for(int i = 0; i < func->basicBlocks.size(); i++) {
        auto bb = func->basicBlocks[i].get();
        if(bb->getBlockSize() == 1 && bb->getPredecessor().size() == 1 && bb->getSuccessor().size() == 1) {
            auto bI = dynamic_cast<BrInstruction*>(bb->getTerminator());
            assert(!bI->getCondition());
            auto succ = bI->getTrueTarget();
            auto pred = bb->getPredecessor()[0];
            assert(succ);
            assert(pred);
            bool illegal = false;
            for(auto& inst : succ->instructionsRef()) {
                if(auto phi = dynamic_cast<PhiInstruction*>(inst.get())) {
                    if(phi->incomings().size() != 1) {
                        illegal = true;
                        break;
                    }
                }
            }
            if(illegal) {
                continue;
            }
            cout << "replace empty bb:"<< bb->getName() << " with " << succ->getName() << endl;
            auto predBI = dynamic_cast<BrInstruction*>(pred->getTerminator());
            assert(predBI);
            //if(predBI->getTrueTarget() == bb) {
            //    predBI->setTrueTarget(succ);
            //} else {
            //    predBI->setFalseTarget(succ);
            //}
            resetTarget(predBI, bb, succ);
            pred->deleteSuccBasicBlock(bb);
            pred->addSuccBasicBlock(succ);
            succ->replacePredecessor(bb, pred);
            if(predBI->getTrueTarget() == predBI->getFalseTarget()) {
               predBI->rmOperand(0);
               predBI->setFalseTarget(nullptr);
            }
            changedBB = true;
        } 
    }


    return changedBB;
}


bool runSimplifyCFG(FunctionPtr func) {
    bool change = false;
    bool optimized = false;
    runCFGAnalysis(func);
    int count = 0;
    do{
        count++;
        change = false;
        change |= removeUnreachableBlocks(func);
        runCFGAnalysis(func);
        change |= deleteOneFromOneToBB(func);
        runCFGAnalysis(func);
        change |= replaceEmptyBB(func);
        runCFGAnalysis(func);
        optimized |= change;
    } while (change);
    return optimized;
}
