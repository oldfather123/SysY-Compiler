#include "riscv.h"

#define MYPAIR std::pair<MInst *, MInst *>
MYPAIR findOrderBetweenIaIb(MInst *Ia, MInst *Ib) {
    if(Ia->mb != Ib->mb || Ia == Ib) return MYPAIR(nullptr, nullptr);
    for(auto tmpIa = Ia; tmpIa; tmpIa = tmpIa->next) {
        if(tmpIa == Ib) return MYPAIR(Ia, Ib);
    }
    return MYPAIR(Ib, Ia);
}
#undef MYPAIR

bool isIssuable(MInst *Ia, MInst *Ib) {
    auto ordMI = findOrderBetweenIaIb(Ia, Ib);
    if(!ordMI.first || !ordMI.second) return false;
    // move first before second
    auto firStrI = dynamic_cast<MI_Store *>(ordMI.first);
    auto firLdrI = dynamic_cast<MI_Load *>(ordMI.first);
    auto firDefs = getDefs(ordMI.first);
    auto firOprs = firDefs + getUses(ordMI.first);
    for(auto midMI = ordMI.first->next; midMI != ordMI.second; midMI = midMI->next) {
        auto midStrI = dynamic_cast<MI_Store *>(midMI);
        auto midStrV = dynamic_cast<MI_FStore *>(midMI);
        auto midLdrI = dynamic_cast<MI_Load *>(midMI);
        auto midLdrV = dynamic_cast<MI_FLoad *>(midMI);
        auto midDefs = getDefs(midMI);
        auto midOprs = midDefs + getUses(midMI);
        for(auto &def : firDefs) {
            if(midOprs.find(def) != -1)
                goto moveSecondAfterFirst;
        }
        for(auto &def : midDefs) {
            if(firOprs.find(def) != -1)
                goto moveSecondAfterFirst;
        }
        if(firStrI && midStrI && firStrI->base == midStrI->base && firStrI->offset == midStrI->offset ||
           firStrI && midStrV && firStrI->base == midStrV->base && firStrI->offset == midStrV->offset ||
           firStrI && midLdrI && firStrI->base == midLdrI->base && firStrI->offset == midLdrI->offset ||
           firStrI && midLdrV && firStrI->base == midLdrV->base && firStrI->offset == midLdrV->offset ||
           firLdrI && midStrI && firLdrI->base == midStrI->base && firLdrI->offset == midStrI->offset ||
           firLdrI && midStrV && firLdrI->base == midStrV->base && firLdrI->offset == midStrV->offset) {
            goto moveSecondAfterFirst;
        }
    }
    ordMI.first->moveBefore(ordMI.second);
    return true;
    // move second after first
    moveSecondAfterFirst :
    auto secStrI = dynamic_cast<MI_Store *>(ordMI.second);
    auto secLdrI = dynamic_cast<MI_Load *>(ordMI.second);
    auto secDefs = getDefs(ordMI.second);
    auto secOprs = secDefs + getUses(ordMI.second);
    for(auto midMI = ordMI.first->next; midMI != ordMI.second; midMI = midMI->next) {
        auto midStrI = dynamic_cast<MI_Store *>(midMI);
        auto midStrV = dynamic_cast<MI_FStore *>(midMI);
        auto midLdrI = dynamic_cast<MI_Load *>(midMI);
        auto midLdrV = dynamic_cast<MI_FLoad *>(midMI);
        auto midDefs = getDefs(midMI);
        auto midOprs = midDefs + getUses(midMI);
        for(auto &def : secDefs) {
            if(midOprs.find(def) != -1)
                return false;
        }
        for(auto &def : midDefs) {
            if(secOprs.find(def) != -1)
                return false;
        }
        if(secStrI && midStrI && secStrI->base == midStrI->base && secStrI->offset == midStrI->offset ||
           secStrI && midStrV && secStrI->base == midStrV->base && secStrI->offset == midStrV->offset ||
           secStrI && midLdrI && secStrI->base == midLdrI->base && secStrI->offset == midLdrI->offset ||
           secStrI && midLdrV && secStrI->base == midLdrV->base && secStrI->offset == midLdrV->offset ||
           secLdrI && midStrI && secLdrI->base == midStrI->base && secLdrI->offset == midStrI->offset ||
           secLdrI && midStrV && secLdrI->base == midStrV->base && secLdrI->offset == midStrV->offset) {
            return false;
        }
    }
    ordMI.second->moveAfter(ordMI.first);
    return true;
}

void doubleBandwidth(MProg *mProj) {
    for(auto mFunc : mProj->mFuncs) {
        curFunc = mFunc;
        for(auto mb : curFunc->mBlocks) {
            map<MOperand, map<int, vector<MI_Load *>>> ldrBasesOffsMIs;
            map<MOperand, map<int, vector<MI_Store *>>> strBasesOffsMIs;
            for(auto I = mb->firInst; I; I = I->next) {
                if (auto ldrMI = dynamic_cast<MI_Load *>(I)) {
                    if (ldrMI->isRv64 || (ldrMI->base.tag == IREG && ldrMI->base.value == sp) || ldrMI->base.tag == GLO_ADR || ldrMI->base.tag == IMM) continue;
                    ldrBasesOffsMIs[ldrMI->base][ldrMI->offset.value].push_back(ldrMI);
                } else if (auto strMI = dynamic_cast<MI_Store *>(I)) {
                    if (strMI->isRv64 || (strMI->base.tag == IREG && strMI->base.value == sp) || strMI->base.tag == GLO_ADR || strMI->base.tag == IMM) continue;
                    strBasesOffsMIs[strMI->base][strMI->offset.value].push_back(strMI);
                }
            }
            for(auto &ldrBaseOffsMIs : ldrBasesOffsMIs) {
                auto ldrOffsMIs = ldrBaseOffsMIs.second;
                for(auto &ldrOffMIs : ldrOffsMIs) {
                    auto &ldrOff = ldrOffMIs.first;
                    if(ldrOff % 8 != 0 || ldrOffsMIs.find(ldrOff + 4) == ldrOffsMIs.end()) continue;
                    auto &ldrMIs = ldrOffMIs.second;
                    auto &nextLdrMIs = ldrOffsMIs[ldrOff + 4];
                    ldrMatch :
                    for(auto &ldrMI : ldrMIs) {
                        for(auto &nextLdrMI : nextLdrMIs) {
                            if(!isIssuable(ldrMI, nextLdrMI)) continue;
                            auto zxtDst = makeVIReg(curFunc->vIRegCount++);
                            auto srlDst = makeVIReg(curFunc->vIRegCount++);
                            ldrMI->reg.replaceAllUseWith(&zxtDst, curFunc);
                            nextLdrMI->reg.replaceAllUseWith(&srlDst, curFunc);
                            auto zxtMI = new MI_Zext(zxtDst, ldrMI->reg);
                            auto srlMI = new MI_Binary(BINARY_SRL, srlDst, ldrMI->reg, makeImm(32));
                            ldrMI->isRv64 = true;
                            zxtMI->isRv64 = true;
                            srlMI->isRv64 = true;
                            zxtMI->insertAfter(ldrMI);
                            srlMI->insertAfter(zxtMI);
                            nextLdrMI->deleteFromParent(curFunc);
                            ldrMIs.erase(std::find(ldrMIs.begin(), ldrMIs.end(), ldrMI));
                            nextLdrMIs.erase(std::find(nextLdrMIs.begin(), nextLdrMIs.end(), nextLdrMI));
                            stillOptimize = true;
                            goto ldrMatch;
                        }
                    }
                }
            }
            for(auto &strBaseOffsMIs : strBasesOffsMIs) {
                auto &strOffsMIs = strBaseOffsMIs.second;
                for(auto &strOffMIs : strOffsMIs) {
                    auto &strOff = strOffMIs.first;
                    if(strOff % 8 != 0 || strOffsMIs.find(strOff + 4) == strOffsMIs.end()) continue;
                    auto &strMIs = strOffMIs.second;
                    auto &nextStrMIs = strOffsMIs[strOff + 4];
                    strMatch :
                    for(auto &strMI : strMIs) {
                        for(auto &nextStrMI : nextStrMIs) {
                            if(!isIssuable(strMI, nextStrMI)) continue;
                            auto defMI = strMI->reg.getDefI(curFunc);
                            auto nextDefMI = nextStrMI->reg.getDefI(curFunc);
                            auto LiMI = dynamic_cast<MI_Move *>(defMI);
                            auto nextLiMI = dynamic_cast<MI_Move *>(nextDefMI);
                            if(LiMI && nextLiMI && LiMI->src.tag == IMM && nextLiMI->src.tag == IMM) {
                                auto mvDst = makeVIReg(curFunc->vIRegCount++);
                                auto mvSrc = makeImm(((uint64)(uint32)nextLiMI->src.value << 32) + (uint64)(uint32)LiMI->src.value);
                                auto mvMI = new MI_Move(mvDst, mvSrc);
                                auto newStrMI = new MI_Store(mvDst, strMI->base, strMI->offset);
                                mvMI->isRv64 = true;
                                newStrMI->isRv64 = true;
                                mvMI->insertBefore(strMI);
                                newStrMI->insertBefore(strMI);
                                strMI->deleteFromParent(curFunc);
                                nextStrMI->deleteFromParent(curFunc);
                                stillOptimize = true;
                            } else if(getDefs(defMI).size() == 1 && getDefs(nextDefMI).size() == 1) {
                                auto sllDst = makeVIReg(curFunc->vIRegCount++);
                                auto sllLft = getDefs(nextDefMI).front();
                                auto addDst = makeVIReg(curFunc->vIRegCount++);
                                auto addRhs = getDefs(defMI).front();
                                auto sllMI = new MI_Binary(BINARY_SLL, sllDst, sllLft, makeImm(32));
                                auto addMI = new MI_Binary(BINARY_ADD, addDst, sllDst, addRhs);
                                auto newStrMI = new MI_Store(addDst, strMI->base, strMI->offset);
                                sllMI->isRv64 = true;
                                addMI->isRv64 = true;
                                newStrMI->isRv64 = true;
                                sllMI->insertBefore(strMI);
                                addMI->insertBefore(strMI);
                                newStrMI->insertBefore(strMI);
                                strMI->deleteFromParent(curFunc);
                                nextStrMI->deleteFromParent(curFunc);
                                stillOptimize = true;
                            }
                            strMIs.erase(std::find(strMIs.begin(), strMIs.end(), strMI));
                            nextStrMIs.erase(std::find(nextStrMIs.begin(), nextStrMIs.end(), nextStrMI));
                            goto strMatch;
                        }
                    }
                }
            }
        }
    }
}
