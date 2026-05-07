#include "riscv.h"

bool isPowerOf2(int n) {
    return !(n & (n - 1));
}

std::pair<int, int> magic(int d) {
    int p = 31;
    unsigned ad, anc, delta, q1, r1, q2, r2, t, two31 = 0x80000000;
    ad = abs(d);
    t = two31 + ((unsigned)d >> 31);
    anc = t - 1 - t % ad;
    q1 = two31 / anc;
    r1 = two31 - q1 * anc;
    q2 = two31 / ad;
    r2 = two31 - q2 * ad;
    do {
        p = p + 1;
        q1 = 2 * q1;
        r1 = 2 * r1;
        if(r1 >= anc) {
            q1 = q1 + 1;
            r1 = r1 - anc;
        }
        q2 = 2 * q2;
        r2 = 2 * r2;
        if(r2 >= ad) {
            q2 = q2 + 1;
            r2 = r2 - ad;
        }
        delta = ad - r2;
    } while(q1 < delta || (q1 == delta && q1 == 0));
    int M = q2 + 1;
    if(d < 0)
        M = -M;
    int s = p - 32;
    return { M, s };
}

void algebraicIdentity(MProg* mProg) {
    // all operands are SSA
    auto zeroReg = makeIReg(0);
    for(auto mFunc : mProg->mFuncs) {
        curFunc = mFunc;
        auto &intValMap = curFunc->intValMap;
        for(auto mb : curFunc->mBlocks) {
            for(auto inst = mb->firInst; inst; inst = inst->next) {
                if(auto biMI = dynamic_cast<MI_Binary*>(inst)) {
                    switch(biMI->op) {
                        case BINARY_ADD: {
                            // 0 + x
                            if(biMI->lhs.isImm() && biMI->lhs.value == 0 || biMI->lhs == zeroReg) {
                                biMI->dst.replaceAllUseWith(&biMI->rhs, curFunc);
                                biMI->deleteFromParent(curFunc);
                            }
                            // x + 0
                            else if(biMI->rhs.isImm() && biMI->rhs.value == 0 || biMI->rhs == zeroReg) {
                                biMI->dst.replaceAllUseWith(&biMI->lhs, curFunc);
                                biMI->deleteFromParent(curFunc);
                            }
                            // x + x
                            else if(biMI->lhs == biMI->rhs) {
                                auto sllMI = new MI_Binary(BINARY_SLL, biMI->dst, biMI->lhs, makeImm(1));
                                sllMI->insertBefore(biMI);
                                biMI->deleteFromParent(curFunc);
                            }
                            // x + imm12-constant
                            else if(intValMap.count(biMI->rhs) && canBeImm12(intValMap[biMI->rhs])) {
                                biMI->rhs = makeImm(intValMap[biMI->rhs]);
                            }
                        } break;
                        case BINARY_SUB: {
                            // x - 0
                            if(biMI->rhs.isImm() && biMI->rhs.value == 0 || biMI->rhs == zeroReg) {
                                biMI->dst.replaceAllUseWith(&biMI->lhs, curFunc);
                                biMI->deleteFromParent(curFunc);
                            }
                            // x - x
                            else if(biMI->lhs == biMI->rhs) {
                                biMI->dst.replaceAllUseWith(&zeroReg, curFunc);
                                biMI->deleteFromParent(curFunc);
                            }
                            // x - imm12-constant
                            else if(intValMap.count(biMI->rhs) && canBeImm12(intValMap[biMI->rhs])) {
                                biMI->rhs = makeImm(intValMap[biMI->rhs]);
                            }
                        } break;
                        case BINARY_MUL: {
                            // 0 * x
                            if(intValMap.count(biMI->lhs) && intValMap[biMI->lhs] == 0) {
                                biMI->dst.replaceAllUseWith(&zeroReg, curFunc);
                                biMI->deleteFromParent(curFunc);
                            }
                            // x * 0
                            else if(intValMap.count(biMI->rhs) && intValMap[biMI->rhs] == 0) {
                                biMI->dst.replaceAllUseWith(&zeroReg, curFunc);
                                biMI->deleteFromParent(curFunc);
                            }
                            // 1 * x
                            else if(intValMap.count(biMI->lhs) && intValMap[biMI->lhs] == 1) {
                                biMI->dst.replaceAllUseWith(&biMI->rhs, curFunc);
                                biMI->deleteFromParent(curFunc);
                            }
                            // x * 1
                            else if(intValMap.count(biMI->rhs) && intValMap[biMI->rhs] == 1) {
                                biMI->dst.replaceAllUseWith(&biMI->lhs, curFunc);
                                biMI->deleteFromParent(curFunc);
                            }
                            else if(intValMap.count(biMI->rhs)) {
                                int constRhs = intValMap[biMI->rhs];
                                if(constRhs <= 1)
                                    continue;
                                int log2v = __builtin_ctz(constRhs);
                                int v0 = 1 << log2v;
                                // x * 2^sh
                                if(constRhs == v0) {
                                    auto shmat = int(log2(constRhs));
                                    auto lsl = new MI_Binary(BINARY_SLL, biMI->dst, biMI->lhs, makeImm(shmat));
                                    lsl->insertBefore(biMI);
                                    biMI->deleteFromParent(curFunc);
                                }
                                // x * (2^sh1 + 2^sh2) = x << sh1 + x << sh2
                                else if(__builtin_popcount(constRhs - v0) == 1) {
                                    int s = __builtin_ctz(constRhs - v0);
                                    auto tmp = makeVIReg(curFunc->vIRegCount++);
                                    auto sllMI = new MI_Binary(BINARY_SLL, tmp, biMI->lhs, makeImm(s - log2v));
                                    auto addMI = new MI_Binary(BINARY_ADD, biMI->dst, tmp, biMI->lhs);
                                    sllMI->insertBefore(biMI);
                                    addMI->insertBefore(biMI);
                                    if(log2v) {
                                        tmp = makeVIReg(curFunc->vIRegCount++);
                                        sllMI = new MI_Binary(BINARY_SLL, biMI->dst, biMI->dst, makeImm(log2v));
                                        sllMI->insertBefore(biMI);
                                    }
                                    biMI->deleteFromParent(curFunc);
                                }
                                // x * (2^sh1 - 2^sh2) = x << sh1 - x << sh2
                                else if(__builtin_popcount(constRhs - v0) == 1) {
                                    int s = __builtin_ctz(constRhs - v0);
                                    auto tmp = makeVIReg(curFunc->vIRegCount++);
                                    auto sllMI = new MI_Binary(BINARY_SLL, tmp, biMI->lhs, makeImm(s - log2v));
                                    auto subMI = new MI_Binary(BINARY_SUB, biMI->dst, tmp, biMI->lhs);
                                    sllMI->insertBefore(biMI);
                                    subMI->insertBefore(biMI);
                                    if(log2v) {
                                        tmp = makeVIReg(curFunc->vIRegCount++);
                                        sllMI = new MI_Binary(BINARY_SLL, biMI->dst, biMI->dst, makeImm(log2v));
                                        sllMI->insertBefore(biMI);
                                    }
                                    biMI->deleteFromParent(curFunc);
                                }
                            }
                        } break;
                        case BINARY_DIV: {
                            if(intValMap.count(biMI->rhs) == 0)
                                continue;
                            int constRhs = intValMap[biMI->rhs];
                            if(constRhs <= 1)
                                continue;
                            int log2v = __builtin_ctz(constRhs);
                            int v0 = 1 << log2v;
                            // x / 2^sh
                            if(isPowerOf2(constRhs)) {
                                auto lsl_lhs = makeVIReg(curFunc->vIRegCount++);
                                auto r0 = makeVIReg(curFunc->vIRegCount++);
                                auto r1 = makeVIReg(curFunc->vIRegCount++);
                                auto r2 = makeVIReg(curFunc->vIRegCount++);
                                auto sllMI = new MI_Binary(BINARY_SLL, lsl_lhs, biMI->lhs, makeImm(1));
                                auto srlMI = new MI_Binary(BINARY_SRL, r0, lsl_lhs, makeImm(64 - log2v));
                                auto addMI = new MI_Binary(BINARY_ADD, r1, biMI->lhs, r0);
                                auto sraMI = new MI_Binary(BINARY_SRA, biMI->dst, r1, makeImm(log2v));
                                addMI->isRv64 = true;
                                sllMI->insertBefore(biMI);
                                srlMI->insertBefore(biMI);
                                addMI->insertBefore(biMI);
                                sraMI->insertBefore(biMI);
                                biMI->deleteFromParent(curFunc);
                            } else {
                                auto mag = magic(constRhs);
                                auto M = makeImm(mag.first);
                                auto s = makeImm(mag.second);
                                if(!canBeImm12(mag.first)) {
                                    M = makeVIReg(curFunc->vIRegCount++);
                                    emitMove(M, makeImm(mag.first), mb, biMI);
                                }
                                auto r0 = makeVIReg(curFunc->vIRegCount++);
                                auto r1 = makeVIReg(curFunc->vIRegCount++);
                                auto r2 = makeVIReg(curFunc->vIRegCount++);
                                auto r3 = makeVIReg(curFunc->vIRegCount++);
                                auto r4 = makeVIReg(curFunc->vIRegCount++);
                                auto r5 = makeVIReg(curFunc->vIRegCount++);
                                auto mulMI = new MI_Binary(BINARY_MUL, r0, biMI->lhs, M);
                                mulMI->isRv64 = true;
                                mulMI->insertBefore(biMI);
                                if(mag.first & (1LL << 31)) {
                                    s = makeImm(mag.second);
                                    auto srlMI1 = new MI_Binary(BINARY_SRL, r5, r0, makeImm(32));
                                    auto addMI = new MI_Binary(BINARY_ADD, r0, r5, biMI->lhs);
                                    auto srlMI2 = new MI_Binary(BINARY_SRL, r1, r0, makeImm(31));
                                    auto sraMI = new MI_Binary(BINARY_SRA, r2, r0, s);
                                    srlMI1->insertBefore(biMI);
                                    addMI->insertBefore(biMI);
                                    srlMI2->insertBefore(biMI);
                                    sraMI->insertBefore(biMI);
                                } else {
                                    s = makeImm(mag.second + 32);
                                    auto srlMI = new MI_Binary(BINARY_SRL, r1, r0, makeImm(63));
                                    auto sraMI = new MI_Binary(BINARY_SRA, r2, r0, s);
                                    srlMI->isRv64 = true;
                                    sraMI->isRv64 = true;
                                    srlMI->insertBefore(biMI);
                                    sraMI->insertBefore(biMI);
                                }
                                auto addMI = new MI_Binary(BINARY_ADD, biMI->dst, r1, r2);
                                addMI->isRv64 = true;
                                addMI->insertBefore(biMI);
                                biMI->deleteFromParent(curFunc);
                            }
                        } break;
                        case BINARY_MOD: {
                            if(biMI->isRv64)
                                continue;
                            if(intValMap.count(biMI->rhs) == 0)
                                continue;
                            int constRhs = intValMap[biMI->rhs];
                            if(constRhs <= 1)
                                continue;
                            int log2v = __builtin_ctz(constRhs);
                            int v0 = 1 << log2v;
                            if(isPowerOf2(constRhs)) {
                                auto lsl_lhs = makeVIReg(curFunc->vIRegCount++);
                                auto r0 = makeVIReg(curFunc->vIRegCount++);
                                auto r1 = makeVIReg(curFunc->vIRegCount++);
                                auto r2 = makeVIReg(curFunc->vIRegCount++);
                                auto constOpr = makeImm(-constRhs);
                                auto lsl = new MI_Binary(BINARY_SLL, lsl_lhs, biMI->lhs, makeImm(1));
                                auto lsr = new MI_Binary(BINARY_SRL, r0, lsl_lhs, makeImm(64 - log2v));
                                auto add = new MI_Binary(BINARY_ADD, r1, biMI->lhs, r0);
                                add->isRv64 = true;
                                lsl->insertBefore(biMI);
                                lsr->insertBefore(biMI);
                                add->insertBefore(biMI);
                                if(!canBeImm12(-constRhs)) {
                                    constOpr = makeVIReg(curFunc->vIRegCount++);
                                    emitMove(constOpr, makeImm(-constRhs), mb, biMI);
                                }
                                auto and_instr = new MI_Binary(BINARY_AND, r2, r1, constOpr);
                                auto sub = new MI_Binary(BINARY_SUB, biMI->dst, biMI->lhs, r2);
                                and_instr->insertBefore(biMI);
                                sub->insertBefore(biMI);
                                biMI->deleteFromParent(curFunc);
                            } else {
                                auto mag = magic(constRhs);
                                auto M = makeImm(mag.first);
                                auto s = makeImm(mag.second);
                                if(!canBeImm12(mag.first)) {
                                    M = makeVIReg(curFunc->vIRegCount++);
                                    emitMove(M, makeImm(mag.first), mb, biMI);
                                }
                                auto r0 = makeVIReg(curFunc->vIRegCount++);
                                auto r1 = makeVIReg(curFunc->vIRegCount++);
                                auto r2 = makeVIReg(curFunc->vIRegCount++);
                                auto r3 = makeVIReg(curFunc->vIRegCount++);
                                auto r4 = makeVIReg(curFunc->vIRegCount++);
                                auto r5 = makeVIReg(curFunc->vIRegCount++);
                                auto mul1 = new MI_Binary(BINARY_MUL, r0, biMI->lhs, M);
                                mul1->isRv64 = true;
                                mul1->insertBefore(biMI);
                                if(mag.first & (1LL << 31)) {
                                    s = makeImm(mag.second);
                                    auto srli1 = new MI_Binary(BINARY_SRL, r5, r0, makeImm(32));
                                    auto add2 = new MI_Binary(BINARY_ADD, r0, r5, biMI->lhs);
                                    auto srli2 = new MI_Binary(BINARY_SRL, r1, r0, makeImm(31));
                                    auto srai = new MI_Binary(BINARY_SRA, r2, r0, s);
                                    srli1->insertBefore(biMI);
                                    add2->insertBefore(biMI);
                                    srli2->insertBefore(biMI);
                                    srai->insertBefore(biMI);
                                } else {
                                    s = makeImm(mag.second + 32);
                                    auto srli = new MI_Binary(BINARY_SRL, r1, r0, makeImm(63));
                                    auto srai = new MI_Binary(BINARY_SRA, r2, r0, s);
                                    srli->isRv64 = true;
                                    srai->isRv64 = true;
                                    srli->insertBefore(biMI);
                                    srai->insertBefore(biMI);
                                }
                                auto add = new MI_Binary(BINARY_ADD, r3, r1, r2);
                                auto mul2 = new MI_Binary(BINARY_MUL, r4, r3, biMI->rhs);
                                auto sub = new MI_Binary(BINARY_SUB, biMI->dst, biMI->lhs, r4);
                                add->isRv64 = true;
                                add->insertBefore(biMI);
                                mul2->insertBefore(biMI);
                                sub->insertBefore(biMI);
                                biMI->deleteFromParent(curFunc);
                            }
                        } break;
                        default:
                            break;
                    }
                }
            }
        }
    }
}