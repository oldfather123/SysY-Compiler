#include "riscv.h"
#include "cacheLookUp.h"
using namespace std;

BranchCondition invertBranchCond(BranchCondition cond) {
    if(cond == BR_JU || cond == BR_FT)
        return cond;
    if(cond >= 1 && cond <= 3) {
        return (BranchCondition)(cond + 3);
    } else if(cond >= 4 && cond <= 6) {
        return (BranchCondition)(cond - 3);
    }
    assert(false && cond);
    return BR_JU;
}

BranchCondition invertCmpCond(BranchCondition cond) {
    if(cond == BR_JU || cond == BR_FT)
        return cond;
    if(cond == BR_LT)
        return BR_GT;
    else if(cond == BR_GT)
        return BR_LT;
    else if(cond == BR_GE)
        return BR_LE;
    else if(cond == BR_LE)
        return BR_GE;
    assert(false && cond);
    return BR_JU;
}

void buildCvtType(StringBuilder* s, FcvtType t) {
    switch(t) {
        case U32: {
            s->append("wu");
        } break;
        case S32: {
            s->append("w");
        } break;
        case F32: {
            s->append("s");
        } break;
        default: {
            assert(false && t);
        } break;
    }
}

const char* buildBranchSuffix(BranchCondition condition) {
    switch(condition) {
        case BR_JU: {
            return "";
        } break;
        case BR_LT: {
            return "lt";
        } break;
        case BR_GT: {
            return "gt";
        } break;
        case BR_LE: {
            return "le";
        } break;
        case BR_GE: {
            return "ge";
        } break;
        case BR_NE: {
            return "ne";
        } break;
        case BR_EQ: {
            return "eq";
        } break;
        default: {
            assert(false && condition);
            return "";
        } break;
    }
}

void buildOperand(StringBuilder* s, MOperand op) {
    switch(op.tag) {
        case IREG: {
            if(op.value == fp)
                s->append("s0");  // s0 = fp
            else if(op.value == sp)
                s->append("sp");
            else if(op.value == ra)
                s->append("ra");
            else if(op.value == pc)
                s->append("pc");
            else if(op.value == zero)
                s->append("zero");
            else if(op.value == x3)
                s->append("gp");
            else if(op.value == x4)
                s->append("tp");
            else if(op.value >= x5 && op.value <= x7)
                s->append("t%d", op.value - x5);
            else if(op.value == x9)
                s->append("s1");
            else if(op.value >= x10 && op.value <= x17)
                s->append("a%d", op.value - x10);
            else if(op.value >= x18 && op.value <= x27)
                s->append("s%d", op.value - x18 + 2);
            else if(op.value >= x28 && op.value <= x31)
                s->append("t%d", op.value - x28 + 3);
            else {
                assert(false && op.value);
            }
        } break;
        case FREG: {
            if(op.value >= f10 && op.value <= f17)
                s->append("fa%d", op.value - f10);
            else if(op.value >= f0 && op.value <= f7)
                s->append("ft%d", op.value);
            else if(op.value >= f8 && op.value <= f9)
                s->append("fs%d", op.value - f8);
            else if(op.value >= f18 && op.value <= f27)
                s->append("fs%d", op.value - f18 + 2);
            else if(op.value >= f28 && op.value <= f31)
                s->append("ft%d", op.value - f28 + 8);
            else {
                s->append("f%d\n", op.value);
            }
        } break;
        case VIREG: {
            s->append("vr%d", op.value);
        } break;
        case VFREG: {
            s->append("vfr%d", op.value);
        } break;
        case IMM: {
            s->append("%ld", op.value);
        } break;
        case GLO_ADR: {
            s->append("%s", op.addr);
        } break;
        case ERRORTYPE:
        default: {
            assert(false && op.tag);
        } break;
    }
}

void buildInstruction(StringBuilder* s, MInst* I, MBlock* nextBb, MFunc* func) {
    s->append("    ");
    switch(I->tag) {
        case MI_BINARY: {
            auto biMI = (MI_Binary*)I;
            switch(biMI->op) {
                case BINARY_ADD: {
                    biMI->rhs.tag == IMM ? s->append("addi") : s->append("add");
                } break;
                case BINARY_SUB: {
                    biMI->rhs.tag == IMM ? biMI->rhs.value = -biMI->rhs.value, s->append("addi") : s->append("sub");
                } break;
                case BINARY_MUL: {
                    s->append("mul");
                } break;
                case BINARY_DIV: {
                    s->append("div");
                } break;
                case BINARY_MOD: {
                    s->append("rem");
                } break;
                case BINARY_SLL2ADD: {
                    s->append("sh2add");
                } break;
                case BINARY_SLL: {
                    biMI->rhs.tag == IMM ? s->append("slli") : s->append("sll");
                } break;
                case BINARY_SRL: {
                    biMI->rhs.tag == IMM ? s->append("srli") : s->append("srl");
                } break;
                case BINARY_SRA: {
                    biMI->rhs.tag == IMM ? s->append("srai") : s->append("sra");
                } break;
                case BINARY_AND: {
                    biMI->rhs.tag == IMM ? s->append("andi") : s->append("and");
                } break;
                case BINARY_OR: {
                    biMI->rhs.tag == IMM ? s->append("ori") : s->append("or");
                }
                case BINARY_XOR: {
                    biMI->rhs.tag == IMM ? s->append("xori") : s->append("xor");
                } break;
                default: {
                    assert(false && "Wrong binary inst.");
                } break;
            }
            // arithmetic opeartion
            if(biMI->op >= BINARY_ADD && biMI->op <= BINARY_MOD) {
                s->append("%s", biMI->isRv64 ? "" : "w");
            }
            s->append(" ");
            buildOperand(s, biMI->dst);
            s->append(",");
            buildOperand(s, biMI->lhs);
            s->append(",");
            buildOperand(s, biMI->rhs);
            if(biMI->op == BINARY_ADD)
                s->append(" # BINARY_ADD");
            else if(biMI->op == BINARY_SUB)
                s->append(" # BINARY_SUBTRACT");
        } break;
        case MI_BRANCH: {
            auto brMI = (MI_Branch*)I;
            if(brMI->cond == BR_JU) {
                if(brMI->TBlock != nextBb) {
                    s->append("j .L%d_%d", func->index, brMI->TBlock->id);
                }
            } else if(brMI->TBlock == nextBb) {
                // invert the branch cond
                s->append("b%s ", buildBranchSuffix(invertBranchCond(brMI->cond)));
                buildOperand(s, brMI->lhs);
                s->append(",");
                buildOperand(s, brMI->rhs);
                s->append(",.L%d_%d # jump to true\n", func->index, brMI->FBlock->id);
            } else if(brMI->FBlock == nextBb) {
                s->append("b%s ", buildBranchSuffix(brMI->cond));
                buildOperand(s, brMI->lhs);
                s->append(",");
                buildOperand(s, brMI->rhs);
                s->append(",.L%d_%d # jump to true\n", func->index, brMI->TBlock->id);
            } else {
                s->append("b%s ", buildBranchSuffix(brMI->cond));
                buildOperand(s, brMI->lhs);
                s->append(",");
                buildOperand(s, brMI->rhs);
                s->append(",.L%d_%d # jump to true\n", func->index, brMI->TBlock->id);
                s->append("    ");
                s->append("j .L%d_%d # jump to false", func->index, brMI->FBlock->id);
            }
        } break;
        case MI_SLT: {
            auto sltMI = (MI_Slt*)I;
            switch(sltMI->sltTag) {
                case SLT: {
                    s->append("slt ");
                } break;
                case SLT_U: {
                    s->append("sltu ");
                } break;
                case SLT_I: {
                    s->append("slti ");
                } break;
                case SLT_IU: {
                    s->append("sltiu ");
                } break;
                default: {
                    assert(false && "Wrong slt tag");
                } break;
            }
            buildOperand(s, sltMI->dst);
            s->append(",");
            buildOperand(s, sltMI->lhs);
            s->append(",");
            buildOperand(s, sltMI->rhs);
        } break;
        case MI_LOAD:
        case MI_STORE: {
            auto loadOrStore = (MI_Load*)I;
            auto instrType = loadOrStore->isRv64 ? "d" : "w";
            if(loadOrStore->base.tag == GLO_ADR) {
                s->append("la ");
                buildOperand(s, loadOrStore->reg);
                s->append(",");
                buildOperand(s, loadOrStore->base);
                s->append(" # load global address");
            } else {
                s->append(I->tag == MI_LOAD ? "l" : "s");
                s->append("%s ", instrType);
                buildOperand(s, loadOrStore->reg);
                s->append(",");
                buildOperand(s, loadOrStore->offset);
                s->append("(");
                buildOperand(s, loadOrStore->base);
                s->append(")");
            }
        } break;
        case MI_PUSH: {
            auto pushMI = (MI_Push*)I;
            auto numReg = pushMI->operands.size();
            auto sizeReg = numReg * 8;
            // according to ARM, we first order the regs
            std::sort(pushMI->operands.begin(), pushMI->operands.end());
            s->append("addi sp,sp,%d # push\n", -sizeReg);
            // store highest-numbered reg into higher address
            for(int i = numReg - 1; i >= 0; i--) {
                s->append("    sd ");
                buildOperand(s, pushMI->operands[i]);
                s->append(",%d(sp)", 8 * i);
                if(i != 0)
                    s->append("\n");
            }
        } break;
        case MI_POP: {
            auto popMI = (MI_Push*)I;
            auto numReg = popMI->operands.size();
            auto sizeReg = numReg * 8;
            // according to ARM, we first order the regs
            std::sort(popMI->operands.begin(), popMI->operands.end());
            for(int i = 0; i < numReg; i++) {
                if(i > 0)
                    s->append("    ");
                s->append("ld ");
                buildOperand(s, popMI->operands[i]);
                s->append(",%d(sp)\n", 8 * i);
            }
            s->append("    addi sp,sp,%d # pop", sizeReg);
        } break;
        case MI_ZEXT: {
            auto zextMI = (MI_Zext*)I;
            s->append("zext.w ");
            buildOperand(s, zextMI->dst);
            s->append(",");
            buildOperand(s, zextMI->src);
        } break;
        case MI_FUNC_CALL: {
            auto callMI = (MI_Func_Call*)I;
            s->append("call %s", callMI->funcName.c_str());
        } break;
        case MI_RETURN: {
            s->append("jr ra");
        } break;
        case MI_MOVE: {
            auto mvMI = (MI_Move*)I;
            if(mvMI->src.tag == IMM) {
                s->append("li ");
                buildOperand(s, mvMI->dst);
                s->append(",");
                buildOperand(s, mvMI->src);
            } else {
                s->append("mv ");
                buildOperand(s, mvMI->dst);
                s->append(",");
                buildOperand(s, mvMI->src);
            }
        } break;
        case MI_FBINARY: {
            auto biMI = (MI_FBinary*)I;
            switch(biMI->op) {
                case BINARY_ADD: {
                    s->append("fadd.s");
                } break;
                case BINARY_SUB: {
                    s->append("fsub.s");
                } break;
                case BINARY_MUL: {
                    s->append("fmul.s");
                } break;
                case BINARY_DIV: {
                    s->append("fdiv.s");
                } break;
                case F_NEG: {
                    s->append("fneg.s");
                } break;
                default: {
                    assert(false && "Wrong binary instruction.");
                } break;
            }
            s->append(" ");
            buildOperand(s, biMI->dst);
            s->append(",");
            buildOperand(s, biMI->lhs);
            if(biMI->fneg != 1) {
                s->append(",");
                buildOperand(s, biMI->rhs);
            }
        } break;
        case MI_FLOAD:
        case MI_FSTORE: {
            auto loadOrStore = (MI_FLoad*)I;
            if(loadOrStore->base.tag == GLO_ADR) {
                s->append("la ");
                buildOperand(s, loadOrStore->reg);
                s->append(",");
                buildOperand(s, loadOrStore->base);
                s->append(" # load float global address");
            } else {
                s->append(I->tag == MI_FLOAD ? "fl" : "fs");
                s->append(loadOrStore->isRv64 ? "d " : "w ");
                buildOperand(s, loadOrStore->reg);
                s->append(",");
                buildOperand(s, loadOrStore->offset);
                s->append("(");
                buildOperand(s, loadOrStore->base);
                s->append(")");
            }
        } break;
        case MI_FPUSH: {
            auto pushMI = (MI_FPush*)I;
            auto numReg = pushMI->operands.size();
            auto sizeReg = numReg * 8;
            // according to ARM, we first order the regs
            std::sort(pushMI->operands.begin(), pushMI->operands.end());
            s->append("addi sp,sp,%d # push\n", -sizeReg);
            // store highest-numbered reg into higher address
            for(int i = numReg - 1; i >= 0; i--) {
                s->append("    fsd ");
                buildOperand(s, pushMI->operands[i]);
                s->append(",%d(sp)", 8 * i);
                if(i != 0)
                    s->append("\n");
            }
        } break;
        case MI_FPOP: {
            auto popMI = (MI_FPop*)I;
            auto numReg = popMI->operands.size();
            auto regSize = numReg * 8;
            // according to ARM, we first order the regs
            std::sort(popMI->operands.begin(), popMI->operands.end());
            for(int i = 0; i < numReg; i++) {
                if(i > 0)
                    s->append("    ");
                s->append("fld ");
                buildOperand(s, popMI->operands[i]);
                s->append(",%d(sp)\n", 8 * i);
            }
            s->append("    addi sp,sp,%d # pop", regSize);
        } break;
        case MI_FCVT: {
            auto cvtMI = (MI_FCvt*)I;
            s->append("fcvt.");
            buildCvtType(s, cvtMI->toType);
            s->append(".");
            buildCvtType(s, cvtMI->fromType);
            s->append(" ");
            buildOperand(s, cvtMI->dst);
            s->append(",");
            buildOperand(s, cvtMI->src);
            // round to zero
            if(cvtMI->fromType == F32) {
                s->append(",rtz");
            }
        } break;
        case MI_FCMP: {
            auto cmpMI = (MI_FCmp*)I;
            MOperand lhs, rhs;
            // feq, flt, fle
            if(cmpMI->isFcmpCond()) {
                lhs = cmpMI->lhs;
                rhs = cmpMI->rhs;
                s->append("f%s.s ", buildBranchSuffix(cmpMI->cond));
            }
            // fne
            else if(cmpMI->cond == BR_NE) {
                s->append("feq.s ");
                buildOperand(s, cmpMI->dst);
                s->append(",");
                buildOperand(s, cmpMI->lhs);
                s->append(",");
                buildOperand(s, cmpMI->rhs);
                s->append("\n    ");
                s->append("xori ");
                lhs = cmpMI->dst;
                rhs = makeImm(1);
            } else {
                lhs = cmpMI->rhs;
                rhs = cmpMI->lhs;
                s->append("f%s.s ", buildBranchSuffix(invertCmpCond(cmpMI->cond)));
            }
            buildOperand(s, cmpMI->dst);
            s->append(",");
            buildOperand(s, lhs);
            s->append(",");
            buildOperand(s, rhs);
        } break;
        case MI_FMOVE: {
            auto vmvMI = (MI_FMove*)I;
            s->append(vmvMI->fromS32 ? "fmv.w.x " : "fmv.s ");
            buildOperand(s, vmvMI->dst);
            s->append(",");
            buildOperand(s, vmvMI->src);
        } break;
        default: {
            assert(false && "Wrong instruction.");
        } break;
    }
    s->append("\n");
}

void buildFunction(StringBuilder* s, MFunc* func) {
    s->append("%s:\n", func->name.c_str());
    for(int i = 0; i < func->mBlocks.len; i++) {
        auto nextBb = (i != func->mBlocks.len - 1) ? func->mBlocks[i + 1] : nullptr;
        s->append(".L%d_%d:\n", func->index, func->mBlocks[i]->id);
        for(auto I = func->mBlocks[i]->firInst; I; I = I->next) {
            buildInstruction(s, I, nextBb, func);
        }
    }
}

void getInitSequence(Variable* gloVal, vector<pair<string, int>>& initInst) {
    if(gloVal->getType()->isInt()) {
        int constInt = gloVal->as<Int>()->intVal;
        if(constInt) {
            initInst.emplace_back(make_pair(".word", constInt));
        } else {
            if(initInst.empty() || initInst.back().first == ".word") {
                initInst.emplace_back(make_pair(".zero", 0));
            }
            initInst.back().second += 4;
        }
    } else if(gloVal->getType()->isFloat()) {
        f2i.floatValue = gloVal->as<Float>()->floatVal;
        initInst.emplace_back(make_pair(".word", (f2i.intValue)));
    } else if(gloVal->zero()) {
        if(initInst.empty() || initInst.back().first == ".word") {
            initInst.emplace_back(make_pair(".zero", 0));
        }
        auto zeroSize = gloVal->getType()->as<ArrType>()->getSize() * 4;
        initInst.back().second += zeroSize;
    } else {
        auto gloArr = gloVal->as<Arr>();
        int i = 0;
        for(; i < gloArr->inner.size(); i++) {
            getInitSequence(gloArr->inner[i], initInst);
        }
        if(gloArr->getElementType()->isArr()) {
            if(i != gloArr->getElementLength()) {
                if(initInst.empty() || initInst.back().first == ".word") {
                    initInst.emplace_back(make_pair(".zero", 0));
                }
                auto zeroSize = gloArr->getElementType()->as<ArrType>()->getSize();
                initInst.back().second += zeroSize * (gloArr->getElementLength() - i) * 4;
            }
        } else if(gloArr->getElementType()->isInt()) {
            if(i != gloArr->getElementLength()) {
                if(initInst.empty() || initInst.back().first == ".word") {
                    initInst.emplace_back(make_pair(".zero", 0));
                }
                initInst.back().second += (gloArr->getElementLength() - i) * 4;
            }
        } else if(gloArr->getElementType()->isFloat()) {
            if(i != gloArr->getElementLength()) {
                if(initInst.empty() || initInst.back().first == ".word") {
                    initInst.emplace_back(make_pair(".zero", 0));
                }
                initInst.back().second += (gloArr->getElementLength() - i) * 4;
            }
        }
    }
}

void buildGlobals(StringBuilder* s, vector<VariablePtr>& globalVariables) {
    if(globalVariables.size() == 0)
        return;
    s->append(".data\n.align 2\n");
    for(auto gloVal : globalVariables) {
        if(gloVal->getType()->isInt()) {
            s->append("%s:\n    ", gloVal->getName().c_str());
            int32 val = gloVal->as<Int>()->intVal;
            s->append(".word %d\n", val);
        } else if(gloVal->getType()->isFloat()) {
            s->append("%s:\n    ", gloVal->getName().c_str());
            f2i.floatValue = gloVal->as<Float>()->floatVal;
            s->append(".word %d\n", f2i.intValue);
        } else if(gloVal->getType()->isArr()) {
            // int complete?
            vector<pair<string, int>> initInst;
            auto globalarr = gloVal->as<Arr>();
            if(globalarr->zero()) {
                continue;
            } else {
                getInitSequence(gloVal, initInst);
                s->append("%s:\n", gloVal->getName().c_str());
                for(auto inst : initInst) {
                    s->append("    %s  %d\n", inst.first.c_str(), inst.second);
                }
            }
        } else {
            assert(false && "Wrong global type.");
        }
    }
    s->append(".bss\n");
    s->append(".align 2\n");
    for(auto gloVal : globalVariables) {
        if(gloVal->getType()->isArr()) {
            auto globalarr = gloVal->as<Arr>();
            if(globalarr->zero()) {
                auto zeroSize = globalarr->getType()->as<ArrType>()->getSize() * 4;
                s->append("    .type %s, @object\n", gloVal->getName().c_str());
                s->append("    .size %s, %d\n", gloVal->getName().c_str(), zeroSize);
                s->append("%s:\n", gloVal->getName().c_str());
                s->append("    .zero %d\n", zeroSize);
            }
        }
    }
    return;
}

void buildProgram(StringBuilder* s, MProg* prog, vector<Variable*>& globalVariables, bool shouldAddCacheLookup) {


    // 读取 runtime 文件为 字符串 appnd
    if(shouldAddCacheLookup) {
        // ifstream runtimeFile("/Users/nickchen/workspace/Yat-CC2/runtime/runtime.s");
        // if(runtimeFile.is_open()) {
        //     string line;
        //     while(getline(runtimeFile, line)) {
        //         s->append("%s\n", line.c_str());
        //     }
        //     runtimeFile.close();
        // } else {
        //     cout << "Unable to open file" << endl;
        // }
        s->append(yatccCacheLookUpStr.c_str());
    } else {

        s->append("    .option nopic\n");
        s->append("    .attribute arch, \"rv64i2p1_m2p0_f2p2_d2p2_zba1p0_zbb1p0\"\n");
        s->append("    .attribute unaligned_access, 0\n");
        s->append("    .attribute stack_align, 16\n");
    }

    buildGlobals(s, globalVariables);
    s->append("\n");
    s->append("    .text\n");
    s->append("    .align 1\n");
    s->append("    .globl main\n");
    s->append("    .type main, @function\n");

    for(int i = 0; i < prog->mFuncs.len; i++) {
        buildFunction(s, prog->mFuncs[i]);
        s->append("\n\n");
    }
}