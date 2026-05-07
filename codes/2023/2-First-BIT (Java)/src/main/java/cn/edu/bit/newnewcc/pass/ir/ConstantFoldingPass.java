package cn.edu.bit.newnewcc.pass.ir;

import cn.edu.bit.newnewcc.ir.Module;
import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.exception.IllegalArgumentException;
import cn.edu.bit.newnewcc.ir.exception.IllegalStateException;
import cn.edu.bit.newnewcc.ir.value.BasicBlock;
import cn.edu.bit.newnewcc.ir.value.Function;
import cn.edu.bit.newnewcc.ir.value.Instruction;
import cn.edu.bit.newnewcc.ir.value.constant.*;
import cn.edu.bit.newnewcc.ir.value.instruction.*;

import java.util.HashSet;
import java.util.Set;

import static java.lang.Math.abs;

/**
 * 常量折叠 <br>
 * 若指令的运算结果可以在编译期推导得出，则折叠该语句 <br>
 */
public class ConstantFoldingPass {

    /**
     * 折叠语句 <br>
     * 若该语句的结果是定值，则返回该值；否则返回 null <br>
     * 对于 Call 语句，折叠在函数内联中进行 <br>
     *
     * @param instruction 待折叠的语句
     * @return 若该语句的结果是定值，则返回该值；否则返回null
     */
    private static Value foldInstruction(Instruction instruction) {
        if (instruction instanceof ArithmeticInst arithmeticInst) {
            var op1 = arithmeticInst.getOperand1();
            var op2 = arithmeticInst.getOperand2();
            if (arithmeticInst instanceof IntegerArithmeticInst integerArithmeticInst) {
                if (arithmeticInst instanceof IntegerAddInst) {
                    // 1+2 = 3
                    if (op1 instanceof ConstInteger constInt1 && op2 instanceof ConstInteger constInt2) {
                        return ConstInteger.getInstance(
                                integerArithmeticInst.getType().getBitWidth(),
                                ConstInteger.valueOf(constInt1) + ConstInteger.valueOf(constInt2)
                        );
                    }
                    // 0+x = x
                    if (op1 instanceof ConstInteger constInt1 && ConstInteger.valueOf(constInt1) == 0) {
                        return op2;
                    }
                    // x+0 = x
                    if (op2 instanceof ConstInteger constInt2 && ConstInteger.valueOf(constInt2) == 0) {
                        return op1;
                    }
                } else if (arithmeticInst instanceof IntegerSubInst) {
                    // 3-2 = 1
                    if (op1 instanceof ConstInteger constInt1 && op2 instanceof ConstInteger constInt2) {
                        return ConstInteger.getInstance(
                                integerArithmeticInst.getType().getBitWidth(),
                                ConstInteger.valueOf(constInt1) - ConstInteger.valueOf(constInt2)
                        );
                    }
                    // x-0 = x
                    if (op2 instanceof ConstInteger constInt2 && ConstInteger.valueOf(constInt2) == 0) {
                        return op1;
                    }
                    // x-x = 0
                    if (op1 == op2) {
                        return ConstInteger.getInstance(
                                integerArithmeticInst.getType().getBitWidth(),
                                0
                        );
                    }
                } else if (arithmeticInst instanceof IntegerMultiplyInst) {
                    // 2*3 = 6
                    if (op1 instanceof ConstInteger constInt1 && op2 instanceof ConstInteger constInt2) {
                        return ConstInteger.getInstance(
                                integerArithmeticInst.getType().getBitWidth(),
                                ConstInteger.valueOf(constInt1) * ConstInteger.valueOf(constInt2)
                        );
                    }
                    // 1*x = x
                    if (op1 instanceof ConstInteger constInt1 && ConstInteger.valueOf(constInt1) == 1) {
                        return op2;
                    }
                    // x*1 = x
                    if (op2 instanceof ConstInteger constInt2 && ConstInteger.valueOf(constInt2) == 1) {
                        return op1;
                    }
                    // x*(-1) = -x
                    if (op2 instanceof ConstInteger constInt2 && ConstInteger.valueOf(constInt2) == -1) {
                        var type = ((IntegerArithmeticInst) arithmeticInst).getType();
                        var subInst = new IntegerSubInst(
                                type,
                                ConstInteger.getInstance(type.getBitWidth(), 0),
                                arithmeticInst.getOperand1()
                        );
                        subInst.insertBefore(arithmeticInst);
                        return subInst;
                    }
                } else if (arithmeticInst instanceof IntegerSignedDivideInst) {
                    // 6/2 = 3
                    if (op1 instanceof ConstInteger constInt1 && op2 instanceof ConstInteger constInt2) {
                        return ConstInteger.getInstance(
                                integerArithmeticInst.getType().getBitWidth(),
                                ConstInteger.valueOf(constInt1) / ConstInteger.valueOf(constInt2)
                        );
                    }
                    // x/1 = x
                    if (op2 instanceof ConstInteger constInt2 && ConstInteger.valueOf(constInt2) == 1) {
                        return op1;
                    }
                    // x/(-1) = -x
                    if (op2 instanceof ConstInteger constInt2 && ConstInteger.valueOf(constInt2) == -1) {
                        var type = ((IntegerArithmeticInst) arithmeticInst).getType();
                        var subInst = new IntegerSubInst(
                                type,
                                ConstInteger.getInstance(type.getBitWidth(), 0),
                                arithmeticInst.getOperand1()
                        );
                        subInst.insertBefore(arithmeticInst);
                        return subInst;
                    }
                    // x/x = 1
                    if (op1 == op2) {
                        return ConstInteger.getInstance(
                                integerArithmeticInst.getType().getBitWidth(),
                                1
                        );
                    }
                    // x*y/y = x
                    if (op1 instanceof IntegerMultiplyInst integerMultiplyInst) {
                        if (integerMultiplyInst.getOperand1() == integerArithmeticInst.getOperand2()) {
                            return integerMultiplyInst.getOperand2();
                        }
                        if (integerMultiplyInst.getOperand2() == integerArithmeticInst.getOperand2()) {
                            return integerMultiplyInst.getOperand1();
                        }
                    }
                } else if (arithmeticInst instanceof IntegerSignedRemainderInst) {
                    // 6%2 = 0
                    if (op1 instanceof ConstInteger constInt1 && op2 instanceof ConstInteger constInt2) {
                        return ConstInteger.getInstance(
                                integerArithmeticInst.getType().getBitWidth(),
                                ConstInteger.valueOf(constInt1) % ConstInteger.valueOf(constInt2)
                        );
                    }
                    // x%1 = 0, x%(-1) = 0
                    if (op2 instanceof ConstInteger constInt2 && abs(ConstInteger.valueOf(constInt2)) == 1) {
                        return ConstInteger.getInstance(
                                integerArithmeticInst.getType().getBitWidth(),
                                0
                        );
                    }
                    // x%x = 0
                    if (op1 == op2) {
                        return ConstInteger.getInstance(
                                integerArithmeticInst.getType().getBitWidth(),
                                0
                        );
                    }
                } else {
                    throw new IllegalArgumentException("Unknown type of integer arithmetic instruction.");
                }
            } else if (arithmeticInst instanceof FloatArithmeticInst) {
                if (arithmeticInst instanceof FloatAddInst) {
                    // 1+2 = 3
                    if (op1 instanceof ConstFloat constFloat1 && op2 instanceof ConstFloat constFloat2) {
                        return ConstFloat.getInstance(constFloat1.getValue() + constFloat2.getValue());
                    }
                    // 0+x = x
                    if (op1 instanceof ConstFloat constFloat1 && constFloat1.getValue() == 0) {
                        return op2;
                    }
                    // x+0 = x
                    if (op2 instanceof ConstFloat constFloat2 && constFloat2.getValue() == 0) {
                        return op1;
                    }
                } else if (arithmeticInst instanceof FloatSubInst) {
                    // 3-2 = 1
                    if (op1 instanceof ConstFloat constFloat1 && op2 instanceof ConstFloat constFloat2) {
                        return ConstFloat.getInstance(constFloat1.getValue() - constFloat2.getValue());
                    }
                    // x-0 = x
                    if (op2 instanceof ConstFloat constFloat2 && constFloat2.getValue() == 0) {
                        return op1;
                    }
                    // x-x = 0
                    if (op1 == op2) {
                        return ConstFloat.getInstance(0);
                    }
                } else if (arithmeticInst instanceof FloatMultiplyInst) {
                    // 2*3 = 6
                    if (op1 instanceof ConstFloat constFloat1 && op2 instanceof ConstFloat constFloat2) {
                        return ConstFloat.getInstance(constFloat1.getValue() * constFloat2.getValue());
                    }
                    // 1*x = x
                    if (op1 instanceof ConstFloat constFloat1 && constFloat1.getValue() == 1) {
                        return op2;
                    }
                    // x*1 = x
                    if (op2 instanceof ConstFloat constFloat2 && constFloat2.getValue() == 1) {
                        return op1;
                    }
                    // x*(-1) = -x
                    if (op2 instanceof ConstFloat constFloat2 && constFloat2.getValue() == -1) {
                        var type = ((FloatArithmeticInst) arithmeticInst).getType();
                        var subInst = new FloatNegateInst(
                                type,
                                arithmeticInst.getOperand1()
                        );
                        subInst.insertBefore(arithmeticInst);
                        return subInst;
                    }
                } else if (arithmeticInst instanceof FloatDivideInst) {
                    // 6/2 = 3
                    if (op1 instanceof ConstFloat constFloat1 && op2 instanceof ConstFloat constFloat2) {
                        return ConstFloat.getInstance(constFloat1.getValue() / constFloat2.getValue());
                    }
                    // x/1 = x
                    if (op2 instanceof ConstFloat constFloat2 && constFloat2.getValue() == 1) {
                        return op1;
                    }
                    // x/(-1) = -x
                    if (op2 instanceof ConstFloat constFloat2 && constFloat2.getValue() == -1) {
                        var type = ((FloatArithmeticInst) arithmeticInst).getType();
                        var subInst = new FloatNegateInst(
                                type,
                                arithmeticInst.getOperand1()
                        );
                        subInst.insertBefore(arithmeticInst);
                        return subInst;
                    }
                    // x/x = 1
                    if (op1 == op2) {
                        return ConstFloat.getInstance(1);
                    }
                } else {
                    throw new IllegalArgumentException("Unknown type of float arithmetic instruction.");
                }
            } else {
                throw new IllegalArgumentException("Unknown type of arithmetic instruction.");
            }
        } else if (instruction instanceof CompareInst compareInst) {
            var op1 = compareInst.getOperand1();
            var op2 = compareInst.getOperand2();
            if (compareInst instanceof IntegerCompareInst integerCompareInst) {
                if (op1 instanceof ConstInteger constInt1 && op2 instanceof ConstInteger constInt2) {
                    return ConstBool.getInstance(switch (integerCompareInst.getCondition()) {
                        case EQ -> ConstInteger.valueOf(constInt1) == ConstInteger.valueOf(constInt2);
                        case NE -> ConstInteger.valueOf(constInt1) != ConstInteger.valueOf(constInt2);
                        case SLT -> ConstInteger.valueOf(constInt1) < ConstInteger.valueOf(constInt2);
                        case SLE -> ConstInteger.valueOf(constInt1) <= ConstInteger.valueOf(constInt2);
                        case SGT -> ConstInteger.valueOf(constInt1) > ConstInteger.valueOf(constInt2);
                        case SGE -> ConstInteger.valueOf(constInt1) >= ConstInteger.valueOf(constInt2);
                    });
                }
                if (op1 == op2) {
                    return ConstBool.getInstance(switch (integerCompareInst.getCondition()) {
                        case EQ, SGE, SLE -> true;
                        case NE, SGT, SLT -> false;
                    });
                }
                if (integerCompareInst.getCondition() == IntegerCompareInst.Condition.SLE && (
                        (op1 instanceof ConstInt constInt1 && constInt1.getValue() == Integer.MIN_VALUE) ||
                                (op2 instanceof ConstInt constInt2 && constInt2.getValue() == Integer.MAX_VALUE)
                )) {
                    return ConstBool.getInstance(true);
                }
                if (integerCompareInst.getCondition() == IntegerCompareInst.Condition.SGE && (
                        (op1 instanceof ConstInt constInt1 && constInt1.getValue() == Integer.MAX_VALUE) ||
                                (op2 instanceof ConstInt constInt2 && constInt2.getValue() == Integer.MIN_VALUE)
                )) {
                    return ConstBool.getInstance(true);
                }
            } else if (compareInst instanceof FloatCompareInst floatCompareInst) {
                if (op1 instanceof ConstFloat constFloat1 && op2 instanceof ConstFloat constFloat2) {
                    return ConstBool.getInstance(switch (floatCompareInst.getCondition()) {
                        case OEQ -> constFloat1.getValue() == constFloat2.getValue();
                        case ONE -> constFloat1.getValue() != constFloat2.getValue();
                        case OLT -> constFloat1.getValue() < constFloat2.getValue();
                        case OLE -> constFloat1.getValue() <= constFloat2.getValue();
                        case OGT -> constFloat1.getValue() > constFloat2.getValue();
                        case OGE -> constFloat1.getValue() >= constFloat2.getValue();
                    });
                }
                if (op1 == op2) {
                    return ConstBool.getInstance(switch (floatCompareInst.getCondition()) {
                        case OEQ, OGE, OLE -> true;
                        case ONE, OGT, OLT -> false;
                    });
                }
                if (floatCompareInst.getCondition() == FloatCompareInst.Condition.OLE && (
                        (op1 instanceof ConstFloat constFloat1 && constFloat1.getValue() == Float.MIN_VALUE) ||
                                (op2 instanceof ConstFloat constFloat2 && constFloat2.getValue() == Float.MAX_VALUE)
                )) {
                    return ConstBool.getInstance(true);
                }
                if (floatCompareInst.getCondition() == FloatCompareInst.Condition.OGE && (
                        (op1 instanceof ConstFloat constFloat1 && constFloat1.getValue() == Float.MAX_VALUE) ||
                                (op2 instanceof ConstFloat constFloat2 && constFloat2.getValue() == Float.MIN_VALUE)
                )) {
                    return ConstBool.getInstance(true);
                }
            } else {
                throw new IllegalArgumentException("Unknown type of compare instruction.");
            }
        } else if (instruction instanceof FloatNegateInst floatNegateInst) {
            var op = floatNegateInst.getOperand1();
            if (op instanceof ConstFloat constFloat) {
                return ConstFloat.getInstance(-constFloat.getValue());
            }
        } else if (instruction instanceof SignedMaxInst signedMaxInst) {
            var op1 = signedMaxInst.getOperand1();
            var op2 = signedMaxInst.getOperand2();
            if (op1 instanceof ConstInteger constInteger1 && op2 instanceof ConstInteger constInteger2) {
                return ConstInteger.valueOf(constInteger1) > ConstInteger.valueOf(constInteger2) ? constInteger1 : constInteger2;
            }
        } else if (instruction instanceof SignedMinInst signedMinInst) {
            var op1 = signedMinInst.getOperand1();
            var op2 = signedMinInst.getOperand2();
            if (op1 instanceof ConstInteger constInteger1 && op2 instanceof ConstInteger constInteger2) {
                return ConstInteger.valueOf(constInteger1) < ConstInteger.valueOf(constInteger2) ? constInteger1 : constInteger2;
            }
        } else if (instruction instanceof FloatToSignedIntegerInst floatToSignedIntegerInst) {
            var op = floatToSignedIntegerInst.getSourceOperand();
            if (op instanceof ConstFloat constFloat) {
                return ConstInt.getInstance((int) constFloat.getValue());
            }
        } else if (instruction instanceof PhiInst phiInst) {
            // 此处用到了 ConstInt 和 ConstFloat 都是单例的性质
            // 对于其他值，相等当且仅当内存对象相同
            Set<Value> valueSet = new HashSet<>();
            phiInst.forEach((basicBlock, value) -> valueSet.add(value));
            // 值集合中包含自己时，自己的值对语句结果的唯一性不产生影响
            valueSet.remove(phiInst);
            if (valueSet.size() == 1) {
                return valueSet.iterator().next();
            }
        } else if (instruction instanceof SignedIntegerToFloatInst signedIntegerToFloatInst) {
            var op = signedIntegerToFloatInst.getSourceOperand();
            if (op instanceof ConstInteger constInt) {
                return ConstFloat.getInstance((float) ConstInteger.valueOf(constInt));
            }
        } else if (instruction instanceof ZeroExtensionInst zeroExtensionInst) {
            var op = zeroExtensionInst.getSourceOperand();
            if (op instanceof ConstInteger constInt) {
                var targetType = zeroExtensionInst.getTargetType();
                return switch (targetType.getBitWidth()) {
                    case 1 -> ConstBool.getInstance(ConstInteger.valueOf(constInt) != 0);
                    case 32 -> ConstInt.getInstance((int) ConstInteger.valueOf(constInt));
                    case 64 -> ConstLong.getInstance(ConstInteger.valueOf(constInt));
                    default -> throw new IllegalStateException("Unknown target type " + targetType);
                };
            }
        } else if (instruction instanceof SignedExtensionInst signedExtensionInst) {
            var op = signedExtensionInst.getSourceOperand();
            if (op instanceof ConstInteger constInt) {
                var targetType = signedExtensionInst.getTargetType();
                return switch (targetType.getBitWidth()) {
                    case 1 -> ConstBool.getInstance(ConstInteger.valueOf(constInt) != 0);
                    case 32 -> ConstInt.getInstance((int) ConstInteger.valueOf(constInt));
                    case 64 -> ConstLong.getInstance(ConstInteger.valueOf(constInt));
                    default -> throw new IllegalStateException("Unknown target type " + targetType);
                };
            }
        } else if (instruction instanceof BitCastInst bitCastInst) {
            if (bitCastInst.getSourceType() == bitCastInst.getTargetType()) {
                return bitCastInst.getSourceOperand();
            }
        } else if (instruction instanceof TruncInst truncInst) {
            if (truncInst.getSourceType() == truncInst.getTargetType()) {
                return truncInst.getSourceOperand();
            }
        } else if (instruction instanceof CallInst || instruction instanceof GetElementPtrInst ||
                instruction instanceof MemoryInst || instruction instanceof TerminateInst) {
            return null;
        } else {
            throw new IllegalArgumentException("Unknown type of instruction.");
        }
        return null;
    }

    private static boolean runOnBasicBlock(BasicBlock basicBlock) {
        boolean changed = false;
        for (Instruction instruction : basicBlock.getInstructions()) {
            var foldedValue = foldInstruction(instruction);
            if (foldedValue != null) {
                instruction.replaceAllUsageTo(foldedValue);
                instruction.waste();
                changed = true;
            }
        }
        return changed;
    }

    private static boolean runOnFunction(Function function) {
        boolean changed = false;
        for (BasicBlock basicBlock : function.getBasicBlocks()) {
            if (runOnBasicBlock(basicBlock)) {
                changed = true;
            }
        }
        return changed;
    }

    public static boolean runOnModule(Module module) {
        boolean mChanged = false;
        while (true) {
            boolean changed = false;
            for (Function function : module.getFunctions()) {
                if (runOnFunction(function)) {
                    changed = true;
                    mChanged = true;
                }
            }
            if (!changed) break;
        }
        mChanged |= BranchSimplifyPass.runOnModule(module);
        mChanged |= DeadCodeEliminationPass.runOnModule(module);
        return mChanged;
    }
}
