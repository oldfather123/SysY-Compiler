package cn.edu.bit.newnewcc.pass.ir;

import cn.edu.bit.newnewcc.ir.Module;
import cn.edu.bit.newnewcc.ir.Operand;
import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.type.IntegerType;
import cn.edu.bit.newnewcc.ir.value.BasicBlock;
import cn.edu.bit.newnewcc.ir.value.Function;
import cn.edu.bit.newnewcc.ir.value.Instruction;
import cn.edu.bit.newnewcc.ir.value.constant.ConstBool;
import cn.edu.bit.newnewcc.ir.value.constant.ConstInt;
import cn.edu.bit.newnewcc.ir.value.instruction.IntegerCompareInst;
import cn.edu.bit.newnewcc.ir.value.instruction.PhiInst;
import cn.edu.bit.newnewcc.ir.value.instruction.SignedMaxInst;
import cn.edu.bit.newnewcc.ir.value.instruction.SignedMinInst;
import cn.edu.bit.newnewcc.pass.ir.structure.I32ValueRangeAnalyzer;

/**
 * 值域相关的常量折叠 <br>
 * 若指令的运算结果可以在编译期推导得出，则折叠该语句 <br>
 */
public class RangeRelatedConstantFoldingPass {

    private final I32ValueRangeAnalyzer analyzer;

    private RangeRelatedConstantFoldingPass(Function function) {
        analyzer = I32ValueRangeAnalyzer.analysis(function);
    }

    /**
     * 折叠语句 <br>
     * 若该语句的结果是定值，则返回该值；否则返回 null <br>
     * 对于 Call 语句，折叠在函数内联中进行 <br>
     *
     * @param instruction 待折叠的语句
     * @return 若该语句的结果是定值，则返回该值；否则返回null
     */
    private Value foldInstruction(Instruction instruction) {
        // 检查指令是否包含值确定为常数的操作数，如有，替换之
        if (instruction instanceof PhiInst phiInst) {
            if (phiInst.getType() == IntegerType.getI32()) {
                phiInst.forWithOperand((entryBlock, entryValueOperand) -> {
                    var range = I32ValueRangeAnalyzer.I32ValueRange.getEntryRange(
                            analyzer,
                            phiInst.getBasicBlock(),
                            entryBlock,
                            entryValueOperand.getValue()
                    );
                    if (range.minValue() == range.maxValue()) {
                        entryValueOperand.setValue(ConstInt.getInstance(range.minValue()));
                    }
                });
            }
        } else {
            for (Operand operand : instruction.getOperandList()) {
                if (operand.getType() == IntegerType.getI32()) {
                    var range = analyzer.getValueRangeAtBlock(operand.getValue(), instruction.getBasicBlock());
                    if (range.minValue() == range.maxValue()) {
                        operand.setValue(ConstInt.getInstance(range.minValue()));
                    }
                }
            }
        }
        if (instruction.getType() == IntegerType.getI32()) {
            // 如果范围缩小到只有一个数，则直接返回常数
            var range = analyzer.getValueRange(instruction);
            if (range.minValue() == range.maxValue()) {
                return ConstInt.getInstance(range.minValue());
            }
        }
        if (instruction instanceof IntegerCompareInst integerCompareInst &&
                integerCompareInst.getComparedType() == IntegerType.getI32()) {
            var range1 = analyzer.getValueRangeAtBlock(integerCompareInst.getOperand1(), instruction.getBasicBlock());
            var range2 = analyzer.getValueRangeAtBlock(integerCompareInst.getOperand2(), instruction.getBasicBlock());
            var compareCondition = integerCompareInst.getCondition();
            switch (compareCondition) {
                case NE, EQ -> {
                    if (range1.maxValue() < range2.minValue() || range1.minValue() > range2.maxValue()) {
                        // compareCondition == IntegerCompareInst.Condition.NE?true:false
                        return ConstBool.getInstance(compareCondition == IntegerCompareInst.Condition.NE);
                    }
                }
                case SLT -> {
                    if (range1.maxValue() < range2.minValue()) {
                        return ConstBool.getInstance(true);
                    }
                    if (range1.minValue() >= range2.maxValue()) {
                        return ConstBool.getInstance(false);
                    }
                }
                case SLE -> {
                    if (range1.maxValue() <= range2.minValue()) {
                        return ConstBool.getInstance(true);
                    }
                    if (range1.minValue() > range2.maxValue()) {
                        return ConstBool.getInstance(false);
                    }
                }
                case SGT -> {
                    if (range1.minValue() > range2.maxValue()) {
                        return ConstBool.getInstance(true);
                    }
                    if (range1.maxValue() <= range2.minValue()) {
                        return ConstBool.getInstance(false);
                    }
                }
                case SGE -> {
                    if (range1.minValue() >= range2.maxValue()) {
                        return ConstBool.getInstance(true);
                    }
                    if (range1.maxValue() < range2.minValue()) {
                        return ConstBool.getInstance(false);
                    }
                }
            }
        }
        if (instruction instanceof SignedMaxInst || instruction instanceof SignedMinInst) {
            if (instruction instanceof SignedMaxInst maxInst) {
                var op1 = maxInst.getOperand1();
                var op2 = maxInst.getOperand2();
                var range1 = analyzer.getValueRangeAtBlock(op1, maxInst.getBasicBlock());
                var range2 = analyzer.getValueRangeAtBlock(op2, maxInst.getBasicBlock());
                if (range1.maxValue() <= range2.minValue()) {
                    return op2;
                }
                if (range1.minValue() >= range2.maxValue()) {
                    return op1;
                }
            } else {
                var minInst = (SignedMinInst) instruction;
                var op1 = minInst.getOperand1();
                var op2 = minInst.getOperand2();
                var range1 = analyzer.getValueRangeAtBlock(op1, minInst.getBasicBlock());
                var range2 = analyzer.getValueRangeAtBlock(op2, minInst.getBasicBlock());
                if (range1.maxValue() <= range2.minValue()) {
                    return op1;
                }
                if (range1.minValue() >= range2.maxValue()) {
                    return op2;
                }
            }
        }
        return null;
    }

    private boolean runOnBasicBlock(BasicBlock basicBlock) {
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

    private boolean runOnFunction_(Function function) {
        boolean changed = false;
        for (BasicBlock basicBlock : function.getBasicBlocks()) {
            if (runOnBasicBlock(basicBlock)) {
                changed = true;
            }
        }
        return changed;
    }

    private static boolean runOnFunction(Function function) {
        return new RangeRelatedConstantFoldingPass(function).runOnFunction_(function);
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
