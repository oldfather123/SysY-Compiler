#include "riscv.h"

using namespace std;

float2int f2i;
MFunc* curFunc = nullptr;
bool stillOptimize = true;

void MOperand::ISetUseHead(MIUse* use, MFunc* func) {
    if(isIntReg()) {
        func->IUseHeadMap[*this] = use;
    }
}

void MOperand::FSetUseHead(MIUse* use, MFunc* func) {
    if(isFloatReg()) {
        func->FUseHeadMap[*this] = use;
    }
}

void MOperand::setDefI(MInst* I, MFunc* func) {
    auto iter = func->defIMap.find(*this);
    if(iter != func->defIMap.end() && iter->second != I) {
        func->defIMap[*this] = nullptr;
    } else {
        func->defIMap[*this] = I;
    }
}

MIUse* MOperand::IGetUseHead(MFunc* func) {
    if(isIntReg()) {
        return func->IUseHeadMap[*this];
    } else {
        return nullptr;
    }
}

MIUse* MOperand::FGetUseHead(MFunc* func) {
    if(isFloatReg()) {
        return func->FUseHeadMap[*this];
    } else {
        return nullptr;
    }
}

MInst* MOperand::getDefI(MFunc* func) {
    auto iter = func->defIMap.find(*this);
    if(iter != func->defIMap.end()) {
        return func->defIMap[*this];
    } else {
        return nullptr;
    }
}

void MOperand::IAddUse(MIUse* u, MFunc* func) {
    u->prev = nullptr;
    auto useHead = IGetUseHead(func);
    if(useHead) {
        useHead->prev = u;
    }
    u->next = useHead;
    ISetUseHead(u, func);
}

void MOperand::FAddUse(MIUse* u, MFunc* func) {
    u->prev = nullptr;
    auto useHead = FGetUseHead(func);
    if(useHead) {
        useHead->prev = u;
    }
    u->next = useHead;
    FSetUseHead(u, func);
}

bool MOperand::isImm() {
    return tag == IMM;
}

bool MOperand::isInt() {
    return tag == IREG || tag == VIREG || tag == IMM;
}

bool MOperand::isIntReg() {
    return tag == IREG || tag == VIREG;
}

bool MOperand::isFloat() {
    return tag == FREG || tag == VFREG;
}

bool MOperand::isFloatReg() {
    return tag == FREG || tag == VFREG;
}

void MOperand::replaceAllUseWith(MOperand* newOpr, MFunc* func) {
    if(newOpr == nullptr)
        return;
    for(auto use = IGetUseHead(func); use; use = use->next) {
        assert(use->I != nullptr);
        use->I->IReplaceUses(this, newOpr, func);
    }
    for(auto use = FGetUseHead(func); use; use = use->next) {
        assert(use->I != nullptr);
        use->I->FReplaceUses(this, newOpr, func);
    }
}

bool MInst::isFcmpCond() {
    return cond == BR_LT || cond == BR_LE || cond == BR_EQ;
}

void MInst::insertBefore(MInst* before) {
    if(before->prev == nullptr) {
        before->mb->firInst = this;
    } else {
        before->prev->next = this;
    }
    this->mb = before->mb;
    this->prev = before->prev;
    this->next = before;
    before->prev = this;
}

void MInst::insertAfter(MInst* after) {
    if(after->next == nullptr) {
        after->mb->lastInst = this;
    } else {
        after->next->prev = this;
    }
    this->mb = after->mb;
    this->next = after->next;
    this->prev = after;
    after->next = this;
}

void MInst::moveBefore(MInst* before) {
    removeFromParent();
    insertBefore(before);
}

void MInst::moveAfter(MInst* after) {
    removeFromParent();
    insertAfter(after);
}

void MInst::IReplaceUses(MOperand* oldOpr, MOperand* newOpr, MFunc* func) {
    for(auto use = oldOpr->IGetUseHead(func); use; use = use->next) {
        if(use->I == this) {
            use->rmUse(func);
            auto newUse = new MIUse(newOpr, use->user, use->I);
            newOpr->IAddUse(newUse, func);
        }
    }
    for(auto used : IGetUsesPtr(this, func->hasReturnValue)) {
        if(used && *used == *oldOpr) {
            *used = *newOpr;
        }
    }
}

void MInst::FReplaceUses(MOperand* oldOpr, MOperand* newOpr, MFunc* func) {
    for(auto use = oldOpr->FGetUseHead(func); use; use = use->next) {
        if(use->I == this) {
            use->rmUse(func);
            auto newUse = new MIUse(newOpr, use->user, use->I);
            newOpr->FAddUse(newUse, func);
        }
    }
    for(auto used : FGetUsesPtr(this, func->hasReturnValue)) {
        if(used && *used == *oldOpr) {
            *used = *newOpr;
        }
    }
}

void MInst::removeFromParent() {
    if(mb != nullptr) {
        if(mb->firInst == this) {
            mb->firInst = next;
        }
        if(mb->lastInst == this) {
            mb->lastInst = prev;
        }
        if(mb->terminator == this) {
            mb->terminator = nullptr;
        }
    }
    if(prev) {
        prev->next = next;
    }
    if(next) {
        next->prev = prev;
    }
}

void MInst::deleteFromParent(MFunc* func) {
    removeFromParent();
    for(auto used : IGetUses(this, func->hasReturnValue)) {
        for(auto use = used.IGetUseHead(func); use; use = use->next) {
            if(use->I == this) {
                use->rmUse(func);
            }
        }
    }
    for(auto used : FGetUses(this, func->hasReturnValue)) {
        for(auto use = used.FGetUseHead(func); use; use = use->next) {
            if(use->I == this) {
                use->rmUse(func);
            }
        }
    }
}

void MInst::buildUse() {
    auto IDefs = IGetDefsPtr(this);
    auto IUses = IGetUsesPtr(this, curFunc->hasReturnValue);
    if(IDefs.empty()) {
        IDefs.push(nullptr);
    }
    for(auto IDef : IDefs) {
        for(auto IUse : IUses) {
            auto newUse = new MIUse(IUse, IDef, this);
            IUse->IAddUse(newUse, curFunc);
        }
        if(IDef != nullptr) {
            IDef->setDefI(this, curFunc);
        }
    }
    auto FDefs = FGetDefsPtr(this);
    auto FUses = FGetUsesPtr(this, curFunc->hasReturnValue);
    if(FDefs.empty()) {
        FDefs.push(nullptr);
    }
    for(auto FDef : FDefs) {
        for(auto FUse : FUses) {
            auto newUse = new MIUse(FUse, FDef, this);
            FUse->FAddUse(newUse, curFunc);
        }
        if(FDef != nullptr) {
            FDef->setDefI(this, curFunc);
        }
    }
}

void MBlock::push(MInst* mi) {
    mi->mb = this;
    mi->prev = mi->next = nullptr;
    if(terminator) {
        mi->insertBefore(terminator);
    } else if(lastInst) {
        lastInst->next = mi;
        mi->prev = lastInst;
        lastInst = mi;
    } else {
        lastInst = firInst = mi;
    }
}

void MBlock::cleanEveryButTerm() {
    firInst = lastInst = terminator;
    terminator->prev = terminator->next = nullptr;
}

void MFunc::clearUseDefMap() {
    IUseHeadMap.clear();
    FUseHeadMap.clear();
    defIMap.clear();
}

void MIUse::rmUse(MFunc* func) {
    if(prev == nullptr && next == nullptr) {
        used->ISetUseHead(nullptr, func);
    } else if(prev == nullptr && next != nullptr) {
        used->ISetUseHead(next, func);
        next->prev = nullptr;
    } else if(prev != nullptr && next == nullptr) {
        prev->next = nullptr;
    } else if(prev != nullptr && next != nullptr) {
        prev->next = next;
        next->prev = prev;
    }
}
