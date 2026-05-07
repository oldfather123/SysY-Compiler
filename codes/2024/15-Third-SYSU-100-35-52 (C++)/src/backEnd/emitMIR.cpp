#include "riscv.h"

using namespace std;

#define VECFIND(VEC, OBJ) std::find(VEC.begin(), VEC.end(), OBJ) != VEC.end()

int vIRegCount;
int vFRegCount;
static map<Value*, MOperand> IRV2MOpr;        // Mid Value -> back Opr
static map<BasicBlock*, MBlock*> IRB2MB;      // Mid Block -> back Block
static map<Value*, MOperand> ptr2Addr;        // ptr -> addr
static map<Value*, Value*> ptr2Base;          // ptr -> base
static map<Value*, int> ptr2Offs;             // ptr -> offset
static map<MOperand, int> MOpr2Const;         // opr -> const
static map<string, BinaryOpType> str2OPType;  // irop -> asmop

MOperand makeImm(int64 constant) {
    auto imm = MOperand(IMM, constant);
    MOpr2Const[imm] = constant;
    return imm;
}

MOperand makeIReg(uint8 IReg) {
    return MOperand(IREG, IReg);
}

MOperand makeVIReg(int32 vIRegIndex) {
    return MOperand(VIREG, vIRegIndex);
}

MOperand makeFReg(uint8 FReg) {
    return MOperand(FREG, FReg);
}

MOperand makeVFReg(int32 vFRegIndex) {
    return MOperand(VFREG, vFRegIndex);
}

// 获得操作数的api, 将你所用到的MB换为可操作的对象, 浮点 需要 load 和 转换
MOperand makeOperand(Value* irV, MBlock* mb, bool notImm = false) {
    auto MOpr = IRV2MOpr.find(irV);
    if(MOpr != IRV2MOpr.end()) {
        return MOpr->second;
    }
    if(auto constIrV = irV->as<Const>()) {
        int32 constant = constIrV->intVal;
        if(constIrV->getType()->isFloat()) {
            f2i.floatValue = constIrV->floatVal;
            constant = f2i.intValue;
            notImm = true;
        } else if(constIrV->getType()->isBool()) {
            constant = constIrV->boolVal;
        }
        if(notImm) {
            auto vIReg = makeVIReg(vIRegCount++);
            emitMove(vIReg, makeImm(constant), mb);
            if(constIrV->getType()->isFloat()) {
                auto vFReg = makeVFReg(vFRegCount++);
                auto fmvMI = new MI_FMove(vFReg, vIReg, true);
                assert(mb && "Not find mb");
                mb->push(fmvMI);
                return vFReg;
            } else {
                return vIReg;
            }
        } else {
            return makeLegalImm(constant, mb);
        }
    } else if(auto intIrV = irV->as<Int>()) {
        int32 constant = intIrV->intVal;
        if(notImm) {
            auto vIReg = makeVIReg(vIRegCount++);
            emitMove(vIReg, makeImm(constant), mb);
            return vIReg;
        } else {
            return makeLegalImm(constant, mb);
        }
    } else if(irV->is<Arr>() || irV->is<Ptr>()) {
        assert(notImm && "Must not be imm");
        auto vIReg = makeVIReg(vIRegCount++);
        IRV2MOpr[irV] = vIReg;
        return vIReg;
    } else {
        auto vReg = irV->getType()->isFloat() ? makeVFReg(vFRegCount++) : makeVIReg(vIRegCount++);
        IRV2MOpr[irV] = vReg;
        return vReg;
    }
}

MOperand makeLegalImm(int32 constant, MBlock* mb) {
    if(!canBeImm12(constant)) {
        auto vIReg = makeVIReg(vIRegCount++);
        emitMove(vIReg, makeImm(constant), mb);
        return vIReg;
    } else {
        return makeImm(constant);
    }
}

BranchCondition biOp2BrCond(BinaryOpType biOpType) {
    switch(biOpType) {
        case BINARY_LT: {
            return BR_LT;
        } break;
        case BINARY_GT: {
            return BR_GT;
        } break;
        case BINARY_LE: {
            return BR_LE;
        } break;
        case BINARY_GE: {
            return BR_GE;
        } break;
        case BINARY_NE: {
            return BR_NE;
        } break;
        case BINARY_EQ: {
            return BR_EQ;
        } break;
        case BINARY_FT: {
            return BR_FT;
        } break;
        default: {
            assert(false && biOpType);
        } break;
    }
    return BR_JU;
}

// 加载多余的参数
void emitLoadOfLaterArgs(Value* irV, MOperand vReg, MBlock* mb, int curOffset, int argSize, bool isFloat) {
    if(isFloat) {
        auto ldMI = new MI_FLoad(vReg, makeIReg(sp), makeImm(curOffset));
        ldMI->memTag = MEM_LOAD_ARG;
        if(argSize == 8) {
            ldMI->isRv64 = true;
        }
        mb->push(ldMI);
    } else {
        auto ldMI = new MI_Load(vReg, makeIReg(sp), makeImm(curOffset));
        ldMI->memTag = MEM_LOAD_ARG;
        if(argSize == 8) {
            ldMI->isRv64 = true;
        }
        mb->push(ldMI);
    }
    IRV2MOpr[irV] = vReg;
}

// la
void emitLoadOfGlobalRef(Value* irV, MOperand vIReg, MBlock* mb) {
    char* name = new char[irV->getName().size() + 1];
    strcpy(name, irV->getName().c_str());
    auto ldMI = new MI_Load(vIReg, MOperand(GLO_ADR, name));
    ldMI->memTag = MEM_LOAD_GLOBAL_REF;
    mb->push(ldMI);
    IRV2MOpr[irV] = vIReg;
}

// li, mv
void emitMove(MOperand mvDst, MOperand mvSrc, MBlock* mb, MInst* mi) {
    auto mvMI = new MI_Move(mvDst, mvSrc);
    if(MOpr2Const.count(mvSrc) && mvSrc.tag != IREG) {
        MOpr2Const[mvDst] = MOpr2Const[mvSrc];
    }
    assert(mb && "Not find mb");
    if(mi) {
        mvMI->insertBefore(mi);
    } else {
        mb->push(mvMI);
    }
}

// fmv.w.x (from s32), fmv.s, 操作数均为寄存器
void emitFmove(MOperand mvDst, MOperand mvSrc, MBlock* mb, MInst* mi) {
    if(mvSrc.tag == IMM) {
        auto newSrc = makeVIReg(vIRegCount++);
        emitMove(newSrc, makeImm(mvSrc.value), mb);
        mvSrc = newSrc;
    }
    auto mvMI = new MI_FMove(mvDst, mvSrc, mvSrc.isInt());
    assert(mb && "Not find mb");
    if(mi) {
        mvMI->insertBefore(mi);
    } else {
        mb->push(mvMI);
    }
}

// 浮点数没有比较并分支(e.g. blt), 只能先比较再分支
void emitBranch(BranchCondition brCond, BrInstruction* brIRI, MBlock* mb, MOperand brLhs = makeImm(0),
                MOperand brRhs = makeImm(0), bool isFloat = false) {
    if(isFloat) {
        // if ((lhs cond rhs) != 0), jump to true
        auto fcmpDst = makeVIReg(vIRegCount++);
        auto fcmpMI = new MI_FCmp(fcmpDst, brLhs, brRhs, brCond);
        mb->push(fcmpMI);
        brLhs = fcmpDst;
        brRhs = makeIReg(zero);
        brCond = BR_NE;
    }
    auto brMI = new MI_Branch(brCond, brLhs, brRhs, IRB2MB[brIRI->getTrueTarget()]);
    if(brIRI->getFalseTarget())
        brMI->FBlock = IRB2MB[brIRI->getFalseTarget()];
    mb->push(brMI);
    mb->terminator = brMI;
}

// 发射分支指令和二元指令
void emitInstruction(Instruction* irI, BasicBlock* irB, MBlock* mb, int32& i) {
    if(auto brIRI = irI->as<BrInstruction>()) {
        if(brIRI->getCondition()) {
            emitBranch(BR_NE, brIRI, mb, makeOperand(brIRI->getCondition(), mb, true), makeIReg(zero));
        } else {
            emitBranch(BR_JU, brIRI, mb);
        }
    } else if(auto fnegIRI = irI->as<FnegInstruction>()) {
        auto fnegDst = makeOperand(fnegIRI, mb, true);
        auto fnegSrc = makeOperand(fnegIRI->getSrc(), mb, true);
        auto fnegMI = new MI_FBinary(F_NEG, fnegDst, fnegSrc);
        fnegMI->fneg = 1;
        mb->push(fnegMI);
    } else if(auto biIRI = irI->as<BinaryInstruction>()) {
        auto opType = str2OPType[biIRI->getOp()];
        // not rd, rs = sub rd, 1, rs; assume rs can only be 0 or 1
        if(opType == UNARY_XOR) {
            assert(!biIRI->getLHS()->isConst);
            auto imm1 = makeVIReg(vIRegCount++);
            emitMove(imm1, makeImm(1), mb);
            auto subDst = makeOperand(biIRI, mb);
            auto subLhs = imm1;
            auto subRhs = makeOperand(biIRI->getLHS(), mb, true);
            auto subMI = new MI_Binary(BINARY_SUB, subDst, subLhs, subRhs);
            mb->push(subMI);
        } else if(biIRI->getLHS()->getType()->isFloat() || biIRI->getRHS()->getType()->isFloat()) {
            auto biLhs = makeOperand(biIRI->getLHS(), mb, true);
            auto biRhs = makeOperand(biIRI->getRHS(), mb, true);
            assert(biLhs.isFloat() || biRhs.isFloat());
            if(!biLhs.isFloat()) {
                auto fcvtDst = makeVFReg(vFRegCount++);
                auto fcvtMI = new MI_FCvt(F32, S32, fcvtDst, biLhs);
                mb->push(fcvtMI);
                biLhs = fcvtDst;
            } else if(!biRhs.isFloat()) {
                auto fcvtDst = makeVFReg(vFRegCount++);
                auto fcvtMI = new MI_FCvt(F32, S32, fcvtDst, biRhs);
                mb->push(fcvtMI);
                biRhs = fcvtDst;
            }
            auto biDst = makeOperand(biIRI, mb, true);
            auto biMI = new MI_FBinary(opType, biDst, biLhs, biRhs);
            biMI->isRv64 = biIRI->isrv64;
            mb->push(biMI);
        } else {
            auto biLhs = makeOperand(biIRI->getLHS(), mb, true);
            auto biRhs = (biIRI->op == Mul || biIRI->op == Div || biIRI->op == Rem) ? makeOperand(biIRI->getRHS(), mb, true) :
                                                                                      makeOperand(biIRI->getRHS(), mb);
            auto biDst = makeOperand(biIRI, mb, true);
            auto biMI = new MI_Binary(opType, biDst, biLhs, biRhs);
            biMI->isRv64 = biIRI->isrv64;
            mb->push(biMI);
        }
    } else if(auto icmpIRI = irI->as<IcmpInstruction>()) {
        // assume only branch instr after Icmp
        assert(i + 1 < irB->getBlockSize());
        auto nextIRI = irB->instructions()[i + 1];
        if(auto brIRI = nextIRI->as<BrInstruction>()) {
            i++;
            auto lhs = makeOperand(icmpIRI->getLHS(), mb, true);
            auto rhs = makeOperand(icmpIRI->getRHS(), mb, true);
            emitBranch(biOp2BrCond(str2OPType[icmpIRI->getOp()]), brIRI, mb, lhs, rhs);
        } else {
            // nonbool -> bool, e.g. if (a) -> if (a ne 0), type = ne
            assert(str2OPType[icmpIRI->getOp()] == BINARY_NE);
            auto subDst = makeVIReg(vIRegCount++);
            auto subLhs = makeOperand(icmpIRI->getLHS(), mb, true);
            auto subRhs = makeOperand(icmpIRI->getRHS(), mb);
            auto subMI = new MI_Binary(BINARY_SUB, subDst, subLhs, subRhs);
            auto mvDst = makeOperand(icmpIRI, mb, true);
            auto mvMI = new MI_Move(mvDst, makeIReg(zero));
            auto sltMI = new MI_Slt(mvDst, makeIReg(zero), subDst, SLT_U);
            mb->push(subMI);
            mb->push(mvMI);
            mb->push(sltMI);
        }
    } else if(auto fcmpIRI = irI->as<FcmpInstruction>()) {
        assert(i + 1 < irB->getBlockSize());
        auto nextIRI = irB->instructions()[i + 1];
        if(auto brIRI = nextIRI->as<BrInstruction>()) {
            i++;
            auto fcvtLhs = makeOperand(fcmpIRI->getLHS(), mb, true);
            auto fcvtRhs = makeOperand(fcmpIRI->getRHS(), mb, true);
            assert(fcvtLhs.isFloat() || fcvtRhs.isFloat());
            if(!fcvtLhs.isFloat()) {
                auto fcvtDst = makeVFReg(vFRegCount++);
                auto fcvtMI = new MI_FCvt(F32, S32, fcvtDst, fcvtLhs);
                mb->push(fcvtMI);
                // IRV2MOpr[fcmpIRI->getLHS()] = fcvtMIDst;
                fcvtLhs = fcvtDst;
            } else if(!fcvtRhs.isFloat()) {
                auto fcvtDst = makeVFReg(vFRegCount++);
                auto fcvtMI = new MI_FCvt(F32, S32, fcvtDst, fcvtRhs);
                mb->push(fcvtMI);
                // IRV2MOpr[fcmpIRI->getRHS()] = fcvtMIDst;
                fcvtRhs = fcvtDst;
            }
            emitBranch(biOp2BrCond(str2OPType[fcmpIRI->getOp()]), brIRI, mb, fcvtLhs, fcvtRhs, true);
        } else {
            auto fcmpLhs = makeOperand(fcmpIRI->getLHS(), mb, true);
            auto fcmpRhs = makeOperand(fcmpIRI->getRHS(), mb, true);
            assert(fcmpLhs.isFloat() || fcmpRhs.isFloat());
            if(!fcmpLhs.isFloat()) {
                auto fcvtDst = makeVFReg(vFRegCount++);
                auto fcvtMI = new MI_FCvt(F32, S32, fcvtDst, fcmpLhs);
                mb->push(fcvtMI);
                fcmpLhs = fcvtDst;
            } else if(!fcmpRhs.isFloat()) {
                auto fcvtDst = makeVFReg(vFRegCount++);
                auto fcvtMI = new MI_FCvt(F32, S32, fcvtDst, fcmpRhs);
                mb->push(fcvtMI);
                fcmpRhs = fcvtDst;
            }
            auto fcmpDst = makeOperand(fcmpIRI, mb, true);
            auto fcmpMI = new MI_FCmp(fcmpDst, fcmpLhs, fcmpRhs, biOp2BrCond(str2OPType[fcmpIRI->getOp()]));
            mb->push(fcmpMI);
        }
    } else {
        assert(false && "Wrong op type");
    }
}

// 发射函数 MFunc
void emitFunction(Function* irF, int idx, vector<VariablePtr>& globalValues) {
    vIRegCount = 0;
    vFRegCount = 0;

    IRV2MOpr.clear();
    IRB2MB.clear();
    ptr2Offs.clear();
    MOpr2Const.clear();

    curFunc = new MFunc;
    curFunc->index = idx;
    curFunc->stackSize = 0;
    curFunc->name = irF->getName();

    auto bbidx = 0;
    auto curOffs = 0;

    // 创建 machine blocks
    for(auto& irB : irF->getBasicBlocks()) {
        auto mB = new MBlock(bbidx++, irB->loopDepth, curFunc);
        curFunc->mBlocks.push(mB);
        IRB2MB[irB.get()] = mB;
    }
    // 获取 machine block 对应的 succ 和 pred
    for(auto& irB : irF->getBasicBlocks()) {
        for(auto succBB : irB->getSuccessor()) {
            IRB2MB[irB.get()]->succs.push(IRB2MB[succBB]);
        }
        for(auto predBB : irB->getPredecessor()) {
            IRB2MB[irB.get()]->preds.push(IRB2MB[predBB]);
        }
    }
    // 函数参数分配虚拟寄存器 a0-a3
    for(int i = 0; i < 4 && i < irF->getArgCount(); i++) {
        auto argIRV = irF->getArg(i);
        if(argIRV->getType()->isFloat()) {
            auto vFReg = makeVFReg(vFRegCount++);
            IRV2MOpr[argIRV] = vFReg;
            emitFmove(vFReg, makeFReg(i + fa0), curFunc->mBlocks[0]);
        } else {
            auto vIReg = makeVIReg(vIRegCount++);
            IRV2MOpr[argIRV] = vIReg;
            emitMove(vIReg, makeIReg(i + a0), curFunc->mBlocks[0]);
        }
    }
    // 函数参数分配虚拟寄存器 a4-
    for(int i = 4; i < irF->getArgCount(); i++) {
        auto argIRV = irF->getArg(i);
        auto ifFloat = argIRV->getType()->isFloat();
        auto argMR = ifFloat ? makeVFReg(vFRegCount++) : makeVIReg(vIRegCount++);
        // load pointer(64 bits) or int/float(32 bits)
        auto argSize = (argIRV->getType()->is64Bit()) ? 8 : 4;
        emitLoadOfLaterArgs(argIRV, argMR, curFunc->mBlocks[0], curOffs, argSize, ifFloat);
        curOffs += argSize;
    }
    // 翻译指令
    for(auto& irB : irF->getBasicBlocks()) {
        auto mb = IRB2MB[irB.get()];
        for(int i = 0; i < irB->getBlockSize(); i++) {
            auto irI = irB->instructions()[i];
            switch(irI->getInsID()) {
                case InsID::Br:
                case InsID::Fneg:
                case InsID::Binary:
                case InsID::Icmp:
                case InsID::Fcmp: {
                    emitInstruction(irI, irB.get(), mb, i);
                } break;
                case InsID::Sitofp: {
                    auto fcvtIRI = irI->as<SitofpInstruction>();
                    auto fcvtDst = makeOperand(fcvtIRI, mb, true);
                    auto fcvtSrc = makeOperand(fcvtIRI->getFrom(), mb, true);
                    auto fcvtMI = new MI_FCvt(F32, S32, fcvtDst, fcvtSrc);
                    mb->push(fcvtMI);
                } break;
                case InsID::Fptosi: {
                    auto fcvtIRI = irI->as<FptosiInstruction>();
                    auto fcvtDst = makeOperand(fcvtIRI, mb, true);
                    auto fcvtSrc = makeOperand(fcvtIRI->getFrom(), mb, true);
                    auto fcvtMI = new MI_FCvt(S32, F32, fcvtDst, fcvtSrc);
                    mb->push(fcvtMI);
                } break;
                case InsID::GEP: {
                    auto GEPIRI = irI->as<GetElementPtrInstruction>();
                    auto notConst = 0;  // 索引不是常量
                    auto offset = 0;    // 常量偏移
                    auto cnt = 0;
                    Type* curArr = GEPIRI->getBase()->getType();
                    MOperand mulRegs[50];
                    vector<MInst*> mulInsts(50, nullptr);
                    // 记录基地址
                    if(ptr2Base.find(GEPIRI->getBase()) == ptr2Base.end()) {
                        ptr2Base[GEPIRI] = GEPIRI->getBase();
                    } else {
                        ptr2Base[GEPIRI] = ptr2Base[GEPIRI->getBase()];
                    }
                    // 处理 loop-combine GEP
                    if(auto loopGEP = GEPIRI->loopGEP) {
                        auto offsMV = makeLegalImm(loopGEP->getStepOffset() * 4, mb);
                        auto baseMV = makeOperand(GEPIRI, mb, true);
                        auto addMI = new MI_Binary(BINARY_ADD, baseMV, baseMV, offsMV);
                        addMI->isRv64 = true;
                        mb->push(addMI);
                        break;
                    }
                    // GEP 作为 load | store 的基地址，必须是整数寄存器，但是 GEP 指令本身可能是浮点数
                    IRV2MOpr[GEPIRI] = makeVIReg(vIRegCount++);
                    // 全局数组
                    if(VECFIND(globalValues, GEPIRI->getBase())) {
                        auto gloReg = makeVIReg(vIRegCount++);
                        emitLoadOfGlobalRef(GEPIRI->getBase(), gloReg, mb);
                    }
                    // 非常量
                    for(auto x : GEPIRI->getIdx()) {
                        if(!x->isConst) {
                            notConst = 1;
                            break;
                        }
                    }
                    // 索引数组
                    auto indices = GEPIRI->getIdx();
                    // 是指针
                    if(indices.size() == 1) {
                        cnt++;
                        auto x = indices[0];
                        auto constSize = 1;
                        if(auto tmp = GEPIRI->getBase()->getType()->as<PtrType>()) {
                            if(auto inner = tmp->inner->as<ArrType>()) {
                                constSize = inner->getSize();
                            }
                        }
                        if(!notConst) {
                            auto constVal = x->as<Const>()->intVal;
                            offset += constSize * constVal;
                        } else if(constSize != 1) {
                            auto tmpIrV = new Int("tmp", false, false, constSize);
                            auto mulDst = makeVIReg(vIRegCount++);
                            auto mulLhs = makeOperand(x, mb, true);
                            auto mulRhs = makeOperand(tmpIrV, mb, true);
                            auto mulMI = new MI_Binary(BINARY_MUL, mulDst, mulLhs, mulRhs);
                            mulRegs[cnt] = mulMI->dst;
                            mulInsts[cnt] = mulMI;
                        } else {
                            mulRegs[cnt] = makeOperand(x, mb, true);
                        }
                    } else {
                        for(int i = 1; i < indices.size(); i++) {
                            cnt++;
                            auto x = indices[i];
                            auto constSize = 1;
                            if(auto tmp = curArr->as<ArrType>()) {
                                if(auto inner = tmp->inner->as<ArrType>()) {
                                    constSize = inner->getSize();
                                }
                                curArr = tmp->inner;
                            }
                            if(!notConst) {
                                auto constVal = x->as<Const>()->intVal;
                                offset += constSize * constVal;
                            } else if(constSize != 1) {
                                auto tmpIrV = new Int("tmp", false, false, constSize);
                                auto mulDst = makeVIReg(vIRegCount++);
                                auto mulLhs = makeOperand(x, mb, true);
                                auto mulRhs = makeOperand(tmpIrV, mb, true);
                                auto mulMI = new MI_Binary(BINARY_MUL, mulDst, mulLhs, mulRhs);
                                mulRegs[cnt] = mulMI->dst;
                                mulInsts[cnt] = mulMI;
                            } else {
                                mulRegs[cnt] = makeOperand(x, mb, true);
                            }
                        }
                    }
                    if(!notConst) {
                        ptr2Offs[GEPIRI] = offset;
                        if(ptr2Offs.count(GEPIRI->getBase()) != 0) {
                            ptr2Offs[GEPIRI] += ptr2Offs[GEPIRI->getBase()];
                        }
                        IRV2MOpr[GEPIRI] = IRV2MOpr[GEPIRI->getBase()];
                        GEPIRI->isConstIdx = true;
                        // 是指针
                    } else if(cnt == 1) {
                        auto baseMV = makeOperand(GEPIRI->getBase(), mb, true);
                        if(ptr2Offs.count(GEPIRI->getBase()) != 0) {
                            auto addRhs = makeLegalImm(ptr2Offs[GEPIRI->getBase()] * 4, mb);
                            auto addDst = makeVIReg(vIRegCount++);
                            auto addMI = new MI_Binary(BINARY_ADD, addDst, baseMV, addRhs);
                            addMI->isRv64 = true;
                            mb->push(addMI);
                            baseMV = addDst;
                        }
                        if(mulInsts[1] != nullptr) {
                            mb->push(mulInsts[1]);
                        }
                        auto sh2addMI = new MI_Binary(BINARY_SLL2ADD, makeOperand(GEPIRI, mb, true), mulRegs[1], baseMV);
                        mb->push(sh2addMI);
                    } else if(cnt > 1) {
                        auto lastReg = mulRegs[1];
                        if(mulInsts[1] != nullptr) {
                            mb->push(mulInsts[1]);
                        }
                        for(int i = 2; i <= cnt; i++) {
                            if(mulInsts[i] != nullptr) {
                                mb->push(mulInsts[i]);
                            }
                            auto addDst = makeVIReg(vIRegCount++);
                            auto addMI = new MI_Binary(BINARY_ADD, addDst, lastReg, mulRegs[i]);
                            addMI->isRv64 = true;
                            mb->push(addMI);
                            lastReg = addMI->dst;
                        }
                        // calculate pointer address
                        auto offsMV = lastReg;
                        auto baseMV = makeOperand(GEPIRI->getBase(), mb);
                        auto destMV = makeOperand(GEPIRI, mb, true);
                        // clear former constant offset
                        if(ptr2Offs.count(GEPIRI->getBase()) != 0) {
                            auto addRhs = makeLegalImm(ptr2Offs[GEPIRI->getBase()] * 4, mb);
                            auto addDst = makeVIReg(vIRegCount++);
                            auto addMI = new MI_Binary(BINARY_ADD, addDst, baseMV, addRhs);
                            addMI->isRv64 = true;
                            mb->push(addMI);
                            baseMV = addDst;
                        }
                        auto sh2addMI = new MI_Binary(BINARY_SLL2ADD, destMV, offsMV, baseMV);
                        mb->push(sh2addMI);
                    }
                } break;
                case InsID::Return: {
                    auto retIRV = irI->as<ReturnInstruction>()->getRetVal();
                    if(!retIRV->getType()->isVoid()) {
                        if(retIRV->isConst) {
                            auto retInt = retIRV->as<Const>()->intVal;
                            emitMove(makeIReg(a0), makeLegalImm(retInt, mb), mb);
                        } else if(retIRV->getType()->isFloat()) {
                            auto retMV = makeOperand(retIRV, mb);
                            emitFmove(makeFReg(fa0), retMV, mb);
                        } else {
                            auto retMV = makeOperand(retIRV, mb);
                            emitMove(makeIReg(a0), retMV, mb);
                        }
                    }
                    auto retMI = new MI_Return;
                    mb->push(retMI);
                    mb->terminator = retMI;
                } break;
                case InsID::Ext: {
                    auto extIRI = irI->as<ExtInstruction>();
                    auto extFrom = makeOperand(extIRI->getFrom(), mb, true);
                    IRV2MOpr[extIRI] = extFrom;
                } break;
                case InsID::Bitcast: {
                    auto bcIRI = irI->as<BitCastInstruction>();
                    auto bcFrom = makeOperand(bcIRI->getFrom(), mb);
                    IRV2MOpr[bcIRI] = bcFrom;
                } break;
                case InsID::Call: {
                    auto callIRI = irI->as<CallInstruction>();
                    auto funcIRF = callIRI->getFunc();
                    // 内置函数
                    if(funcIRF->getName().size() > 15 && funcIRF->getName().find("memset") != string::npos) {
                        auto callMI = new MI_Func_Call("memset");
                        callMI->argCount = funcIRF->args().size() - 1;
                        for(int i = 0; i < callMI->argCount; i++) {
                            auto argIRV = callIRI->getArg(i);
                            auto argMR = makeIReg(i + a0);
                            emitMove(argMR, makeOperand(argIRV, mb), mb);
                        }
                        mb->push(callMI);
                        // 如果有返回值必须存放到a0的寄存器
                        if(!funcIRF->getType()->isVoid()) {
                            auto retMV = makeOperand(callIRI, mb, true);
                            if(funcIRF->getType()->isFloat()) {
                                emitFmove(retMV, makeFReg(fa0), mb);
                            } else {
                                emitMove(retMV, makeIReg(a0), mb);
                            }
                        }
                        break;
                    }
                    // 其他函数
                    auto callMI = new MI_Func_Call(funcIRF->getName());
                    callMI->argCount = callIRI->getNumArg();
                    auto offset = 0;
                    for(int i = 0; i < callMI->argCount; i++) {
                        auto argIRV = callIRI->getArg(i);
                        if(i < 4) {
                            auto argMR = makeIReg(i + a0);
                            auto argMV = makeOperand(argIRV, mb);
                            // array(pointer) was calculated, avoid redundant address calculation
                            if(ptr2Addr.find(argIRV) != ptr2Addr.end()) {
                                argMV = ptr2Addr[argIRV];
                                emitMove(argMR, argMV, mb);
                                // array(pointer) calculation
                            } else if(ptr2Base.find(argIRV) != ptr2Base.end()) {
                                // GEP with constant offset
                                if(ptr2Offs.find(argIRV) != ptr2Offs.end()) {
                                    auto addRhs = makeLegalImm(ptr2Offs[argIRV] * 4, mb);
                                    auto addLhs = makeOperand(argIRV, mb);
                                    argMV = makeVIReg(vIRegCount++);
                                    auto addMI = new MI_Binary(BINARY_ADD, argMV, addLhs, addRhs);
                                    addMI->isRv64 = true;
                                    mb->push(addMI);
                                } else {
                                    // GEP with offset variable
                                    argMV = makeOperand(argIRV, mb, true);
                                }
                                // add map for calculated array(pointer) address
                                ptr2Addr[argIRV] = argMV;
                                emitMove(argMR, argMV, mb);
                            } else if(argIRV->getType()->isFloat()) {
                                argMR = makeFReg(i + fa0);
                                emitFmove(argMR, argMV, mb);
                            } else {
                                emitMove(argMR, argMV, mb);
                            }
                        } else {
                            // for cases i >= 4
                            if(ptr2Offs.find(argIRV) != ptr2Offs.end()) {
                                auto addDst = makeVIReg(vIRegCount++);
                                auto addLhs = makeOperand(argIRV, mb);
                                auto addRhs = makeLegalImm(ptr2Offs[argIRV] * 4, mb);
                                auto addMI = new MI_Binary(BINARY_ADD, addDst, addLhs, addRhs);
                                addMI->isRv64 = true;
                                mb->push(addMI);
                                auto strBase = makeIReg(sp);
                                auto strOffs = makeImm(offset);
                                if(!canBeImm12(offset)) {
                                    auto addDst = makeVIReg(vIRegCount++);
                                    auto addRhs = makeVIReg(vIRegCount++);
                                    emitMove(addRhs, makeImm(offset), mb);
                                    auto addMI = new MI_Binary(BINARY_ADD, addDst, makeIReg(sp), addRhs);
                                    addMI->isRv64 = true;
                                    mb->push(addMI);
                                    strBase = addDst;
                                    strOffs = makeImm(0);
                                }
                                auto strMI = new MI_Store(addMI->dst, strBase, strOffs);
                                strMI->memTag = MEM_PREP_ARG;
                                strMI->isRv64 = true;
                                mb->push(strMI);
                                offset += 8;
                            }
                            // move argument to the stack
                            else if(argIRV->getType()->isFloat()) {
                                auto vstrBase = makeIReg(sp);
                                auto vstrOffs = makeImm(offset);
                                if(!canBeImm12(offset)) {
                                    auto addDst = makeVIReg(vIRegCount++);
                                    auto addRhs = makeVIReg(vIRegCount++);
                                    emitMove(addRhs, makeImm(offset), mb);
                                    auto addMI = new MI_Binary(BINARY_ADD, addDst, makeIReg(sp), addRhs);
                                    addMI->isRv64 = true;
                                    mb->push(addMI);
                                    vstrBase = addDst;
                                    vstrOffs = makeImm(0);
                                }
                                auto vstrMI = new MI_FStore(makeOperand(argIRV, mb, true), vstrBase, vstrOffs);
                                vstrMI->memTag = MEM_PREP_ARG;
                                if(argIRV->getType()->is64Bit()) {
                                    vstrMI->isRv64 = true;
                                    offset += 8;
                                } else {
                                    offset += 4;
                                }
                                mb->push(vstrMI);
                            } else {
                                auto strBase = makeIReg(sp);
                                auto strOffs = makeImm(offset);
                                if(!canBeImm12(offset)) {
                                    auto addDst = makeVIReg(vIRegCount++);
                                    auto addRhs = makeVIReg(vIRegCount++);
                                    emitMove(addRhs, makeImm(offset), mb);
                                    auto addMI = new MI_Binary(BINARY_ADD, addDst, makeIReg(sp), addRhs);
                                    addMI->isRv64 = true;
                                    mb->push(addMI);
                                    strBase = addDst;
                                    strOffs = makeImm(0);
                                }
                                auto strMI = new MI_Store(makeOperand(argIRV, mb, true), strBase, strOffs);
                                strMI->memTag = MEM_PREP_ARG;
                                if(argIRV->getType()->is64Bit()) {
                                    strMI->isRv64 = true;
                                    offset += 8;
                                } else {
                                    offset += 4;
                                }
                                mb->push(strMI);
                            }
                        }
                    }
                    // 栈大小
                    callMI->argStackSize = offset;
                    mb->push(callMI);
                    // 返回值
                    if(!funcIRF->getType()->isVoid()) {
                        auto retMV = makeOperand(callIRI, mb, true);
                        if(funcIRF->getType()->isFloat()) {
                            emitFmove(retMV, makeFReg(fa0), mb);
                        } else {
                            emitMove(retMV, makeIReg(a0), mb);
                        }
                    }
                } break;
                case InsID::Alloca: {
                    auto allocIRI = irI->as<AllocaInstruction>();
                    auto allocSize = 0;
                    auto tp = allocIRI->getType()->getID();
                    switch(tp) {
                        case TypeID::Arr: {
                            allocSize = allocIRI->getType()->as<ArrType>()->getSize() * 4;
                        } break;
                        case TypeID::Ptr:
                        case TypeID::Int:
                        case TypeID::Float: {
                            allocSize = 4;
                        } break;
                        default: {
                            assert(false && "Wrong value type");
                        } break;
                    }
                    curFunc->stackSize += allocSize;
                    if(tp == TypeID::Int)
                        break;
                    // 16B 对齐
                    if(curFunc->stackSize % 16 != 0) {
                        curFunc->stackSize += 16 - (curFunc->stackSize % 16);
                    }
                    auto subDst = makeOperand(allocIRI, mb, true);
                    auto subLhs = makeIReg(sp);
                    auto subRhs = makeImm(curFunc->stackSize);
                    auto biMI = new MI_Binary(BINARY_SUB, subDst, subLhs, subRhs);
                    biMI->isRv64 = true;
                    mb->push(biMI);
                    curFunc->localArrayBases.push(biMI);
                } break;
                // load instruction
                // 1: array type, has const offset
                // 2: array type, no const offset
                // 3: reg, simple
                // 4: reg, global
                // 5: reg, store in stack
                case InsID::Load: {
                    auto loadIRI = irI->as<LoadInstruction>();
                    // 不会 load 指针
                    auto type = loadIRI->getPtr()->getType();
                    while(auto tmp = type->as<PtrType>()) {
                        type = tmp->inner;
                    }
                    if(type->isFloat()) {
                        auto fLoadDst = makeOperand(loadIRI, mb, true);
                        auto fLoadBase = makeOperand(loadIRI->getPtr(), mb);
                        auto fLoadOffs = makeImm(0);
                        // array(pointer)
                        if(ptr2Base.find(loadIRI->getPtr()) != ptr2Base.end()) {
                            // GEP with constant offset
                            if(ptr2Offs.find(loadIRI->getPtr()) != ptr2Offs.end()) {
                                auto offset = ptr2Offs[loadIRI->getPtr()] * 4;
                                fLoadOffs = makeImm(offset);
                                if(!canBeImm12(offset)) {
                                    auto addDst = makeVIReg(vIRegCount++);
                                    auto addRhs = makeVIReg(vIRegCount++);
                                    emitMove(addRhs, makeImm(offset), mb);
                                    auto addMI = new MI_Binary(BINARY_ADD, addDst, fLoadBase, addRhs);
                                    addMI->isRv64 = true;
                                    mb->push(addMI);
                                    fLoadBase = addDst;
                                    fLoadOffs = makeImm(0);
                                }
                            }
                            // global variable
                        } else if(VECFIND(globalValues, loadIRI->getPtr())) {
                            auto gloReg = makeVIReg(vIRegCount++);
                            emitLoadOfGlobalRef(loadIRI->getPtr(), gloReg, mb);
                            fLoadBase = gloReg;
                        }
                        auto fLoadMI = new MI_FLoad(fLoadDst, fLoadBase, fLoadOffs);
                        mb->push(fLoadMI);
                    } else {
                        auto loadDst = makeOperand(loadIRI, mb, true);
                        auto loadBase = makeOperand(loadIRI->getPtr(), mb);
                        auto loadOffs = makeImm(0);
                        // array(pointer)
                        if(ptr2Base.find(loadIRI->getPtr()) != ptr2Base.end()) {
                            // GEP with constant offset
                            if(ptr2Offs.find(loadIRI->getPtr()) != ptr2Offs.end()) {
                                auto offset = ptr2Offs[loadIRI->getPtr()] * 4;
                                loadOffs = makeImm(offset);
                                if(!canBeImm12(offset)) {
                                    auto addDst = makeVIReg(vIRegCount++);
                                    auto addRhs = makeVIReg(vIRegCount++);
                                    emitMove(addRhs, makeImm(offset), mb);
                                    auto addMI = new MI_Binary(BINARY_ADD, addDst, loadBase, addRhs);
                                    addMI->isRv64 = true;
                                    mb->push(addMI);
                                    loadBase = addDst;
                                    loadOffs = makeImm(0);
                                }
                            }
                            // global variable
                        } else if(VECFIND(globalValues, loadIRI->getPtr())) {
                            auto gloReg = makeVIReg(vIRegCount++);
                            emitLoadOfGlobalRef(loadIRI->getPtr(), gloReg, mb);
                            loadBase = gloReg;
                        }
                        auto loadMI = new MI_Load(loadDst, loadBase, loadOffs);
                        mb->push(loadMI);
                    }
                } break;
                // store instruction
                // 1: array type, has const offset
                // 2: array type, no const offset
                // 3: reg, simple
                // 4: reg, global
                // 5: reg, store in stack
                case InsID::Store: {
                    auto storeIRI = irI->as<StoreInstruction>();
                    auto type = storeIRI->getValue()->getType();
                    // store float type
                    if(type->isFloat()) {
                        auto fStoreFrom = makeOperand(storeIRI->getValue(), mb, true);
                        auto fStoreBase = makeOperand(storeIRI->getPtr(), mb);
                        auto fStoreOffs = makeImm(0);
                        // array(pointer)
                        if(ptr2Base.find(storeIRI->getPtr()) != ptr2Base.end()) {
                            // GEP with constant offset
                            if(ptr2Offs.find(storeIRI->getPtr()) != ptr2Offs.end()) {
                                auto offset = ptr2Offs[storeIRI->getPtr()] * 4;
                                fStoreOffs = makeImm(offset);
                                if(!canBeImm12(offset)) {
                                    auto addDst = makeVIReg(vIRegCount++);
                                    auto addRhs = makeVIReg(vIRegCount++);
                                    emitMove(addRhs, makeImm(offset), mb);
                                    auto addMI = new MI_Binary(BINARY_ADD, addDst, fStoreBase, addRhs);
                                    addMI->isRv64 = true;
                                    mb->push(addMI);
                                    fStoreBase = addDst;
                                    fStoreOffs = makeImm(0);
                                }
                            }
                            // global variable
                        } else if(VECFIND(globalValues, storeIRI->getPtr())) {
                            auto gloReg = makeVIReg(vIRegCount++);
                            emitLoadOfGlobalRef(storeIRI->getPtr(), gloReg, mb);
                            fStoreBase = gloReg;
                        }
                        auto fStoreMI = new MI_FStore(fStoreFrom, fStoreBase, fStoreOffs);
                        mb->push(fStoreMI);
                        break;
                    } else {
                        auto storeFrom = makeOperand(storeIRI->getValue(), mb, true);
                        auto storeBase = makeOperand(storeIRI->getPtr(), mb);
                        auto storeOffs = makeImm(0);
                        // array(pointer)
                        if(ptr2Base.find(storeIRI->getPtr()) != ptr2Base.end()) {
                            // GEP with constant offset
                            if(ptr2Offs.find(storeIRI->getPtr()) != ptr2Offs.end()) {
                                auto offset = ptr2Offs[storeIRI->getPtr()] * 4;
                                storeOffs = makeImm(offset);
                                if(!canBeImm12(offset)) {
                                    auto addDst = makeVIReg(vIRegCount++);
                                    auto addRhs = makeVIReg(vIRegCount++);
                                    emitMove(addRhs, makeImm(offset), mb);
                                    auto addMI = new MI_Binary(BINARY_ADD, addDst, storeBase, addRhs);
                                    addMI->isRv64 = true;
                                    mb->push(addMI);
                                    storeBase = addDst;
                                    storeOffs = makeImm(0);
                                }
                            }
                            // global variable
                        } else if(VECFIND(globalValues, storeIRI->getPtr())) {
                            auto gloReg = makeVIReg(vIRegCount++);
                            emitLoadOfGlobalRef(storeIRI->getPtr(), gloReg, mb);
                            storeBase = gloReg;
                            if(MOpr2Const.count(storeFrom)) {
                                MOpr2Const[storeBase] = MOpr2Const[storeFrom];
                            }
                        }
                        auto storeMI = new MI_Store(storeFrom, storeBase, storeOffs);
                        mb->push(storeMI);
                    }
                } break;
                case InsID::Phi:
                    break;
                case InsID::LoopGEP: {
                    // assert that GEP is defined in loop preheader
                    auto loopGEPIRI = irI->as<LoopGEPInstruction>();
                    auto GEPIRI = loopGEPIRI->gep;
                    auto notConst = 0;
                    auto offset = 0;
                    auto cnt = 0;
                    Type* curArr = GEPIRI->getBase()->getType();
                    MOperand mulRegs[50];
                    vector<MInst*> mulInsts(50, nullptr);
                    auto matchedIdx = 0;
                    // 基地址
                    if(ptr2Base.find(GEPIRI->getBase()) == ptr2Base.end()) {
                        ptr2Base[GEPIRI] = GEPIRI->getBase();
                    } else {
                        ptr2Base[GEPIRI] = ptr2Base[GEPIRI->getBase()];
                    }
                    // GEP
                    IRV2MOpr[GEPIRI] = makeVIReg(vIRegCount++);
                    if(std::find(globalValues.begin(), globalValues.end(), GEPIRI->getBase()) != globalValues.end()) {
                        auto gloReg = makeVIReg(vIRegCount++);
                        emitLoadOfGlobalRef(GEPIRI->getBase(), gloReg, mb);
                    }
                    // 非常量
                    for(auto x : GEPIRI->getIdx()) {
                        if(!x->isConst) {
                            notConst = 1;
                            break;
                        }
                    }
                    // 索引数组
                    auto indices = GEPIRI->getIdx();
                    // 是指针
                    if(indices.size() == 1) {
                        cnt++;
                        auto x = indices[0];
                        if(x == loopGEPIRI->index) {
                            matchedIdx = 0;
                            x = Const::getConst(Type::getInt(), 0, "");
                        }
                        auto constSize = 1;
                        if(auto tmp = GEPIRI->getBase()->getType()->as<PtrType>()) {
                            if(auto inner = tmp->inner->as<ArrType>()) {
                                constSize = inner->getSize();
                            }
                        }
                        if(!notConst) {
                            auto constVal = x->as<Const>()->intVal;
                            offset += constSize * constVal;
                        } else if(constSize != 1) {
                            auto tmpIrV = new Int("tmp", false, false, constSize);
                            auto mulDst = makeVIReg(vIRegCount++);
                            auto mulLhs = makeOperand(x, mb, true);
                            auto mulRhs = makeOperand(tmpIrV, mb, true);
                            auto mulMI = new MI_Binary(BINARY_MUL, mulDst, mulLhs, mulRhs);
                            mulRegs[cnt] = mulMI->dst;
                            mulInsts[cnt] = mulMI;
                        } else {
                            mulRegs[cnt] = makeOperand(x, mb, true);
                        }
                    } else {
                        for(int i = 1; i < indices.size(); i++) {
                            cnt++;
                            auto x = indices[i];
                            if(x == loopGEPIRI->index) {
                                matchedIdx = i;
                                x = Const::getConst(Type::getInt(), 0, "");
                            }
                            auto constSize = 1;
                            if(auto tmp = curArr->as<ArrType>()) {
                                if(auto inner = tmp->inner->as<ArrType>()) {
                                    constSize = inner->getSize();
                                }
                                curArr = tmp->inner;
                            }
                            if(!notConst) {
                                auto constVal = x->as<Const>()->intVal;
                                offset += constSize * constVal;
                            } else if(constSize != 1) {
                                auto tmpIrV = new Int("tmp", false, false, constSize);
                                auto mulDst = makeVIReg(vIRegCount++);
                                auto mulLhs = makeOperand(x, mb, true);
                                auto mulRhs = makeOperand(tmpIrV, mb, true);
                                auto mulMI = new MI_Binary(BINARY_MUL, mulDst, mulLhs, mulRhs);
                                mulRegs[cnt] = mulMI->dst;
                                mulInsts[cnt] = mulMI;
                            } else {
                                mulRegs[cnt] = makeOperand(x, mb, true);
                            }
                        }
                    }
                    if(!notConst) {
                        ptr2Offs[GEPIRI] = offset;
                        if(ptr2Offs.count(GEPIRI->getBase()) != 0) {
                            ptr2Offs[GEPIRI] += ptr2Offs[GEPIRI->getBase()];
                        }
                        IRV2MOpr[GEPIRI] = IRV2MOpr[GEPIRI->getBase()];
                        GEPIRI->isConstIdx = true;
                    // 是指针
                    } else if(cnt == 1) {
                        IRV2MOpr[GEPIRI] = IRV2MOpr[GEPIRI->getBase()];
                    } else if(cnt > 1) {
                        auto lastReg = mulRegs[1];
                        if(mulInsts[1] != nullptr && matchedIdx != 1) {
                            mb->push(mulInsts[1]);
                        } else if(matchedIdx == 1) {
                            lastReg = mulRegs[2];
                        }
                        for(int i = 2; i < cnt; i++) {
                            if(i - 1 == matchedIdx) {
                                continue;
                            }
                            if(mulInsts[i] != nullptr) {
                                mb->push(mulInsts[i]);
                            }
                            auto dst = makeVIReg(vIRegCount++);
                            auto addMI = new MI_Binary(BINARY_ADD, dst, lastReg, mulRegs[i]);
                            addMI->isRv64 = true;
                            mb->push(addMI);
                            lastReg = addMI->dst;
                        }
                        // calculate pointer address
                        auto offsMV = lastReg;
                        auto baseMV = makeOperand(GEPIRI->getBase(), mb);
                        auto destMV = makeOperand(GEPIRI, mb, true);
                        // clear former constant offset
                        if(ptr2Offs.count(GEPIRI->getBase()) != 0) {
                            auto addRhs = makeLegalImm(ptr2Offs[GEPIRI->getBase()] * 4, mb);
                            auto addDst = makeVIReg(vIRegCount++);
                            auto addMI = new MI_Binary(BINARY_ADD, addDst, baseMV, addRhs);
                            addMI->isRv64 = true;
                            mb->push(addMI);
                            baseMV = addDst;
                        }
                        auto sh2addMI = new MI_Binary(BINARY_SLL2ADD, destMV, offsMV, baseMV);
                        mb->push(sh2addMI);
                    }
                    auto tmp = makeVIReg(vIRegCount++);
                    auto base = makeOperand(GEPIRI, mb, true);
                    auto initOffs = loopGEPIRI->getInitOffset();
                    if(ptr2Offs.count(GEPIRI->getBase())) {
                        initOffs += ptr2Offs[GEPIRI->getBase()];
                    }
                    auto initOffsOpr = makeLegalImm(initOffs * 4, mb);
                    auto addMI = new MI_Binary(BINARY_ADD, tmp, base, initOffsOpr);
                    addMI->isRv64 = true;
                    mb->push(addMI);
                    IRV2MOpr[GEPIRI] = tmp;
                } break;
                default: {
                    assert(false && "Wrong IR");
                } break;
            }
        }
    }
    // phi instrutions
    for(auto& irB : irF->getBasicBlocks()) {
        auto mb = IRB2MB[irB.get()];
        for(auto irI : irB->instructions()) {
            if(auto phiIRI = irI->as<PhiInstruction>()) {
                if(phiIRI->getType()->isFloat()) {
                    auto phiDst = makeOperand(phiIRI, mb);
                    auto phiMid = makeVFReg(vFRegCount++);
                    emitFmove(phiDst, phiMid, mb, mb->firInst);
                    for(auto p : phiIRI->incomings()) {
                        auto phiBlo = IRB2MB[p.first];
                        auto phiSrc = p.second->val;
                        emitFmove(phiMid, makeOperand(phiSrc, phiBlo), phiBlo);
                    }
                } else {
                    auto phiDst = makeOperand(phiIRI, mb);
                    auto phiMid = makeVIReg(vIRegCount++);
                    emitMove(phiDst, phiMid, mb, mb->firInst);
                    for(auto p : phiIRI->incomings()) {
                        auto phiBlo = IRB2MB[p.first];
                        auto phiSrc = p.second->val;
                        if(phiSrc->getType()->isFloat()) {
                            auto fcvtSrc = makeOperand(phiSrc, phiBlo);
                            auto fcvtDst = phiMid;
                            auto fcvtMI = new MI_FCvt(S32, F32, fcvtDst, fcvtSrc);
                            phiBlo->push(fcvtMI);
                        // int -> int
                        } else {
                            emitMove(phiMid, makeOperand(phiSrc, phiBlo), phiBlo);
                        }
                    }
                }
            }
        }
    }
    map<MOperand, int> intValMap;
    for(auto p : MOpr2Const) {
        if(p.first.tag != IREG && p.first.tag != FREG)
            intValMap.emplace(p.first, p.second);
    }
    curFunc->vIRegCount = vIRegCount;
    curFunc->vFRegCount = vFRegCount;
    curFunc->intValMap = intValMap;
}

MProg* emitProg(Module& programModule) {
    auto programAsm = new MProg;
    // add some map
    str2OPType["+"] = BINARY_ADD;
    str2OPType["-"] = BINARY_SUB;
    str2OPType["*"] = BINARY_MUL;
    str2OPType["/"] = BINARY_DIV;
    str2OPType["%"] = BINARY_MOD;
    str2OPType["!"] = UNARY_XOR;
    str2OPType["<"] = BINARY_LT;
    str2OPType[">"] = BINARY_GT;
    str2OPType["!="] = BINARY_NE;
    str2OPType[">="] = BINARY_GE;
    str2OPType["<="] = BINARY_LE;
    str2OPType["=="] = BINARY_EQ;
    str2OPType[","] = BINARY_SLL;
    str2OPType["."] = BINARY_SRA;
    // do only one for now
    int i = 0;
    for(auto& func : programModule.getGlobalFunctions()) {
        if(func->isLib)
            continue;
        auto globalVars = programModule.getGlobalVariables();
        emitFunction(func, ++i, globalVars);
        programAsm->mFuncs.push(curFunc);
    }
    return programAsm;
}
