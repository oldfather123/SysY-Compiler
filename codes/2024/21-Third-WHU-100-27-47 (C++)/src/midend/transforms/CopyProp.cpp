#include "CopyProp.h"
#include "DFA.h"

using namespace ir;

bool ir::copyProp(FuncPtr func) {
    bool changed = false;

    dbgout << std::endl
           << "CopyProp pass started (" << func->name() << ")." << std::endl;

    Map<RegPtr, Value> copyMap{};
    for (auto bb : func->bbSet()) {
        for (auto inst : bb->getInstTopoList()) {
            if (auto moveInst = castPtr<MoveInst>(inst)) {
                auto destReg = moveInst->destReg();
                auto srcVal = moveInst->srcVal();
                // 复制指令的源值传播到目的寄存器
                copyMap[moveInst->destReg()] = srcVal;
                // 目的寄存器有名称，且源值是匿名寄存器
                // 则重命名源寄存器提高可读性
                if (moveInst->srcVal().isRegister() && !destReg->isAnonymous() && !destReg->isRet()) {
                    auto srcReg = srcVal.getRegister();
                    if (srcReg->isAnonymous()) {
                        auto name = moveInst->destReg()->name();
                        srcReg->rename(name, false);
                        srcReg->isAnonymous() = false;
                    }
                    if (srcReg->isDiscard()) {
                        srcReg->isDiscard() = false;
                    }
                }
            } else if (auto loadInst = castPtr<LoadInst>(inst)) {
                // 全局常量也可传播
                auto srcAddrReg = loadInst->srcAddrReg();
                if (srcAddrReg->isGlobal() && srcAddrReg->dataType().isConst()) {
                    copyMap[loadInst->destReg()] = srcAddrReg->constVal();
                }
            }
        }
    }

    FixedPoint{
        [&](bool &_changed) {
            for (auto bb : func->bbSet()) {
                for (auto inst : bb->getInstSet()) {
                    for (auto useRef : inst->regsReadRefs()) {
                        auto use = useRef.get();
                        if (copyMap.find(use) != copyMap.end()) {
                            auto copyVal = copyMap[use];
                            if (copyVal == use) {
                                continue;
                            }
                            inst->replaceReg(useRef, copyVal);
                            _changed = true;
                        }
                    }
                    inst->recompute();
                }
            }
            changed |= _changed;
        },
        true,
        "Copy Propagation",
        func->name(),
    };

    dbgout << "└── CopyProp pass done." << std::endl;
    return changed;
}
