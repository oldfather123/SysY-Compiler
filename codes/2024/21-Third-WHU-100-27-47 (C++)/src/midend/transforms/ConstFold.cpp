#include "ConstFold.h"
#include "DFA.h"

using namespace ir;

bool ir::constFold(FuncPtr func) {
    bool changed = false;

    dbgout << std::endl
           << "ConstFold pass started (" << func->name() << ")." << std::endl;

    for (auto bb : func->bbSet()) {
        for (auto inst : bb->getInstSet()) {
            Value resultVal;
            Literal resultLiteralVal;
            RegPtr destReg;
            if (auto operInst = castPtr<OperInst>(inst)) {
                destReg = operInst->destReg();
                bool isConstExpr = true;
                Value lhs, rhs;
                if (operInst->isBinary()) {
                    lhs = operInst->lhs();
                    isConstExpr &= lhs.isLiteral();
                }
                rhs = operInst->rhs();
                isConstExpr &= rhs.isLiteral();
                if (!isConstExpr) {
                    continue;
                }
                Literal lhsLiteral, rhsLiteral;
                if (lhs.hasValue()) {
                    lhsLiteral = lhs.getLiteral();
                }
                rhsLiteral = rhs.getLiteral();
                resultLiteralVal = foldOperation(operInst->op(), lhsLiteral, rhsLiteral);
            } else if (auto castInst = castPtr<CastInst>(inst)) {
                destReg = castInst->destReg();
                bool isConstExpr = true;
                auto src = castInst->srcVal();
                isConstExpr &= src.isLiteral();
                if (!isConstExpr) {
                    continue;
                }
                Literal srcVal = src.getLiteral();
                if (destReg->dataType() == srcVal.dataType()) {
                    resultLiteralVal = srcVal;
                } else if (destReg->dataType() == PrimaryDataType::Int && srcVal.dataType() == PrimaryDataType::Float) {
                    resultLiteralVal = (int)srcVal.getFloat();
                } else if (destReg->dataType() == PrimaryDataType::Float && srcVal.dataType() == PrimaryDataType::Int) {
                    resultLiteralVal = (float)srcVal.getInt();
                } else if (destReg->dataType() == PrimaryDataType::Bool && srcVal.dataType() == PrimaryDataType::Int) {
                    resultLiteralVal = (bool)srcVal.getInt();
                } else if (destReg->dataType() == PrimaryDataType::Int && srcVal.dataType() == PrimaryDataType::Bool) {
                    resultLiteralVal = (int)srcVal.getBool();
                }
            } else if (auto phiInst = castPtr<PhiInst>(inst)) {
                destReg = phiInst->destReg();
                Value srcVal; // 如果所有源值都相同，则合并为这个值
                bool canMerge = true;
                for (auto &tuple : phiInst->getTuples()) {
                    if (srcVal.isUnspecified()) {
                        srcVal = tuple.value();
                        continue;
                    }
                    if (tuple.value() != srcVal) {
                        canMerge = false;
                        break;
                    }
                }
                if (canMerge) {
                    resultVal = srcVal;
                }
            }
            if (resultVal.isUnspecified()) {
                if (resultLiteralVal.isUnspecified()) {
                    continue;
                }
                resultVal = resultLiteralVal;
            }
            // 用复制指令替换原指令
            Instruction::replace(inst, MoveInst::create(bb, destReg, resultVal));
            changed = true;
        }
    }

    dbgout << "└── ConstFold pass done." << std::endl;
    return changed;
}
