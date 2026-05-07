#include "riscv.h"

struct MyBiInst {
    BinaryOpType op;
    MOperand lhs, rhs;
    MyBiInst() {}
    MyBiInst(MOperand lhs, MOperand rhs, BinaryOpType op) : lhs(lhs), rhs(rhs), op(op) {}
    bool operator==(const MyBiInst& bm) const {
        return lhs == bm.lhs && rhs == bm.rhs && op == bm.op;
    }
    bool operator<(const MyBiInst& bm) const {
        return op < bm.op || lhs < bm.lhs || rhs < bm.rhs;
    }
};

void peepholePass(MProg* mProg) {
    for(auto mFunc : mProg->mFuncs) {
        curFunc = mFunc;
        for(auto mb : curFunc->mBlocks) {
            // 删除相邻的 store 和 load
            for(auto mi = mb->firInst; mi; mi = mi->next) {
                auto strMI = dynamic_cast<MI_Store*>(mi);
                auto ldrMI = dynamic_cast<MI_Load*>(mi->next);
                if(strMI && ldrMI && strMI->base == ldrMI->base && strMI->offset == ldrMI->offset) {
                    if(!(strMI->reg == ldrMI->reg)) {
                        auto mvMI = new MI_Move(makeVIReg(curFunc->vIRegCount++), strMI->reg);
                        mvMI->insertBefore(ldrMI);
                        ldrMI->reg.replaceAllUseWith(&mvMI->dst, curFunc);
                    }
                    ldrMI->deleteFromParent(curFunc);
                    stillOptimize = true;
                }
            }
            // 删除连续 la 指令
            std::map<MOperand, MI_Load*> laMIs;
            std::map<MyBiInst, MI_Binary*> biMIs;
            for(auto mi = mb->firInst; mi; mi = mi->next) {
                if(auto ldrMI = dynamic_cast<MI_Load*>(mi)) {
                    auto ldrBase = ldrMI->base;
                    if(ldrBase.tag != GLO_ADR)
                        continue;
                    if(laMIs.find(ldrBase) != laMIs.end()) {
                        ldrMI->reg.replaceAllUseWith(&laMIs[ldrBase]->reg, curFunc);
                        ldrMI->deleteFromParent(curFunc);
                        stillOptimize = true;
                    } else {
                        laMIs[ldrBase] = ldrMI;
                    }
                } else if(auto biMI = dynamic_cast<MI_Binary*>(mi)) {
                    MyBiInst myBi(biMI->lhs, biMI->rhs, biMI->op);
                    if(biMIs.count(myBi)) {
                        biMI->dst.replaceAllUseWith(&biMIs[myBi]->dst, curFunc);
                        biMI->deleteFromParent(curFunc);
                        stillOptimize = true;
                    } else {
                        biMIs[myBi] = biMI;
                    }
                }
            }
        }
    }
}