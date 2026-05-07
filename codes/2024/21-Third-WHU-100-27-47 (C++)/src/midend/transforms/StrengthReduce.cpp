#include "StrengthReduce.h"

using namespace ir;

bool _isZero(Value val) {
    return val.isLiteral() && ((val.getLiteral().isInt() && val.getLiteral().getInt() == 0) ||
                               (val.getLiteral().isFloat() && val.getLiteral().getFloat() == 0.0F) ||
                               (val.getLiteral().isBool() && val.getLiteral().getBool() == false));
}

bool _isOne(Value val) {
    return val.isLiteral() && ((val.getLiteral().isInt() && val.getLiteral().getInt() == 1) ||
                               (val.getLiteral().isFloat() && val.getLiteral().getFloat() == 1.0F) ||
                               (val.getLiteral().isBool() && val.getLiteral().getBool() == true));
}

int _getShift(int &num) {
    int shift = 0;
    while (num > 1) {
        if (num & 1) {
            return false;
        }
        num >>= 1;
        ++shift;
    }
    return shift;
}

bool _isPowerOfTwo(Value val, int &shift) {
    if (!val.isLiteral() || !val.getLiteral().isInt() || val.getLiteral().getInt() <= 0) {
        return false;
    }
    int num = val.getLiteral().getInt();
    shift = _getShift(num);
    return num == 1;
}

bool _isNegativePowerOfTwo(Value val, int &shift) {
    if (!val.isLiteral() || !val.getLiteral().isInt() || val.getLiteral().getInt() >= 0) {
        return false;
    }
    int num = -val.getLiteral().getInt();
    shift = _getShift(num);
    return num == 1;
}

void ir::strengthReduce(FuncPtr func, bool introduceBitwiseOperations) {
    dbgout << std::endl
           << "StrengthReduce pass started (" << func->name() << ")." << std::endl;

    for (auto bb : func->bbSet()) {
        for (auto inst : bb->getInstSet()) {
            if (auto operInst = castPtr<ir::OperInst>(inst)) {
                auto destReg = operInst->destReg();
                auto op = operInst->op();
                Value lhs;
                if (operInst->isBinary()) {
                    lhs = operInst->lhs();
                }
                auto rhs = operInst->rhs();
                switch (operInst->op()) {
                    case OperInst::Operator::Mul: {
                        Value newLhs{};

                        if (_isZero(lhs) || _isZero(rhs)) {
                            // 有 0 参与运算，替换为一个赋值为 0 的指令
                            Instruction::replace(operInst, MoveInst::create(bb, destReg, Literal{destReg->dataType()}));
                            break;
                        }

                        bool mulByOne = false;
                        if (_isOne(lhs)) {
                            mulByOne = true;
                            newLhs = rhs;
                        } else if (_isOne(rhs)) {
                            mulByOne = true;
                            newLhs = lhs;
                        }
                        if (mulByOne) {
                            // 有 1 参与运算，替换为一个赋值指令
                            dbgout << func->toString(); // DEBUG
                            Instruction::replace(operInst, MoveInst::create(bb, destReg, newLhs));
                            break;
                        }

                        bool mulByPowerOfTwo = false;
                        int shift;
                        if (_isPowerOfTwo(lhs, shift)) {
                            mulByPowerOfTwo = true;
                            newLhs = rhs;
                        } else if (_isPowerOfTwo(rhs, shift)) {
                            mulByPowerOfTwo = true;
                            newLhs = lhs;
                        }
                        if (mulByPowerOfTwo) {
                            // 有 2 的幂参与运算，替换为一个左移操作
                            if (introduceBitwiseOperations) {
                                Instruction::replace(operInst, OperInst::createBinary(bb, destReg, OperInst::Operator::Shl, newLhs, Value(shift)));
                                break;
                            }
                        }
                        break;
                    }
                    case OperInst::Operator::Div: {
                        int shift;
                        if (_isOne(rhs)) {
                            // 除以 1，替换为一个赋值指令
                            Instruction::replace(operInst, MoveInst::create(bb, destReg, lhs));
                            break;
                        }
                        break;
                    }
                    case OperInst::Operator::Mod: {
                        if (lhs.dataType() == PrimaryDataType::Float) {
                            break;
                        }
                        int shift;
                        // TODO 下列取模的替换导致了一些 WA，暂时注释掉了
                        if (_isPowerOfTwo(rhs, shift)) {
                            // 对 2 的幂取模，替换为一个与操作，lhs & (2^n - 1)
                            if (introduceBitwiseOperations) {
                                // Instruction::replace(operInst, OperInst::createBinary(bb, destReg, OperInst::Operator::And, lhs, Value(~(~0 << shift))));
                            }
                            // break;
                        } else if (_isNegativePowerOfTwo(rhs, shift)) {
                            // TODO 对负数取模运算的这个优化可能造成溢出，且左操作数为负时不正确（例如：(-2147 % -8192) == -2147, ((-2147 + 8192) & 8191) == 6045）
                            // 对 2 的负幂取模，替换为一个与操作，(lhs + 2^n) & (2^n - 1)
                            if (introduceBitwiseOperations) {
                                // auto tmpReg = Register::create(destReg->dataType());
                                // OperInst::createBinary(bb, tmpReg, OperInst::Operator::Add, lhs, Value(1 << shift));
                                // Instruction::replace(operInst, OperInst::createBinary(bb, destReg, OperInst::Operator::And, tmpReg, Value(~(~0 << shift))));
                                // break;
                            }
                        } else {
                            break;
                        }
                        break;
                    }
                    case OperInst::Operator::Add: {
                        Value srcVal;
                        if (_isZero(lhs) || _isZero(rhs)) {
                            srcVal = _isZero(lhs) ? rhs : lhs;
                            // 有 0 参与运算，替换为一个赋值指令
                            if (srcVal.dataType() == destReg->dataType()) { // 考虑指针加减法的运算结果和操作数类型不同问题，不能随便替换
                                Instruction::replace(operInst, MoveInst::create(bb, destReg, srcVal));
                            }
                            break;
                        } else if (lhs == rhs) {
                            // 两个操作数相同，替换为一个左移操作
                            if (introduceBitwiseOperations) {
                                // Instruction::replace(operInst, OperInst::createBinary(bb, destReg, OperInst::Operator::Shl, lhs, Value(1)));
                                break;
                            }
                        }
                        break;
                    }
                    case OperInst::Operator::Sub: {
                        if (_isZero(rhs)) {
                            // 减去 0，替换为一个赋值指令
                            if (lhs.dataType() == destReg->dataType()) { // 考虑指针加减法的运算结果和操作数类型不同问题，不能随便替换
                                Instruction::replace(operInst, MoveInst::create(bb, destReg, lhs));
                            } else {
                                Instruction::replace(operInst, OperInst::createBinary(bb, destReg, Op::Add, lhs, ir::Literal{rhs.dataType()}));
                            }
                            break;
                        } else if (rhs.isLiteral() && ((float)rhs.getLiteral() < 0.0F)) {
                            // 减去负数，替换为一个加法操作
                            if (rhs.dataType() == PrimaryDataType::Float) {
                                Instruction::replace(operInst, OperInst::createBinary(bb, destReg, OperInst::Operator::Add, lhs, Value{-rhs.getLiteral().getFloat()}));
                            } else {
                                Instruction::replace(operInst, OperInst::createBinary(bb, destReg, OperInst::Operator::Add, lhs, Value{-(int)rhs.getLiteral()}));
                            }
                            break;
                        }
                        break;
                    }
                    default: {
                        break;
                    }
                }
            }
        }
    }

    dbgout << "└── StrengthReduce pass done." << std::endl;
}
