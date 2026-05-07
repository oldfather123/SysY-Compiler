package cn.edu.bit.newnewcc.pass.ir.structure;

import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.exception.IllegalStateException;
import cn.edu.bit.newnewcc.ir.value.BasicBlock;
import cn.edu.bit.newnewcc.ir.value.Instruction;
import cn.edu.bit.newnewcc.ir.value.constant.ConstInt;
import cn.edu.bit.newnewcc.ir.value.instruction.*;
import org.antlr.v4.runtime.misc.Pair;

import java.util.HashSet;
import java.util.Set;

/**
 * 简单循环结构，仅支持整数的加法和减法作为循环变量的循环
 * 循环的退出语句必须在循环头内
 *
 * @param condition    循环的条件语句
 * @param stepInst     循环的步进语句
 * @param initialValue 循环变量的初值
 */
public record SimpleLoopInfo(IntegerCompareInst condition, IntegerArithmeticInst stepInst, Value initialValue,
                             BasicBlock exitBlock) {
    private static class Builder {

        private static class BuildFailedException extends Exception {
        }

        private final BasicBlock loopHeadBlock;
        private final Set<BasicBlock> inLoopBlocks = new HashSet<>();
        private final Set<Instruction> inLoopInstructions = new HashSet<>();
        private final SimpleLoopInfo simpleLoopInfo;

        public Builder(Loop loop) {
            this.loopHeadBlock = loop.getHeaderBasicBlock();
            searchLoop(loop);
            simpleLoopInfo = tryCreateSimpleLoopInfo();
        }

        public SimpleLoopInfo getSimpleLoopInfo() {
            return simpleLoopInfo;
        }

        private void searchLoop(Loop loop) {
            inLoopBlocks.addAll(loop.getBasicBlocks());
            for (BasicBlock basicBlock : loop.getBasicBlocks()) {
                inLoopInstructions.addAll(basicBlock.getInstructions());
            }
            for (Loop subLoop : loop.getSubLoops()) {
                searchLoop(subLoop);
            }
        }

        private Value initialValue, exitLoopValue;

        private SimpleLoopInfo tryCreateSimpleLoopInfo() {
            try {
                var p = getExitCondition();
                var exitCondition = p.a;
                var exitBlock = p.b;
                if (!(exitCondition.getOperand1() instanceof PhiInst phiInst)) throw new BuildFailedException();
                analysisPhi(phiInst);
                IntegerArithmeticInst stepInstruction = getStepInstruction(exitLoopValue);
                if (stepInstruction.getOperand1() != phiInst) throw new BuildFailedException();
                return new SimpleLoopInfo(exitCondition, stepInstruction, initialValue, exitBlock);
            } catch (BuildFailedException e) {
                return null;
            }
        }

        private void analysisPhi(PhiInst phiInst) throws BuildFailedException {
            if (phiInst.getBasicBlock() != loopHeadBlock) throw new BuildFailedException();
            Value initialValue = null;
            Value exitLoopValue = null;
            for (BasicBlock block : phiInst.getEntrySet()) {
                var value = phiInst.getValue(block);
                // 对于来自循环内的入边，取值应当为步进后的值；否则，应当为初值
                if (inLoopBlocks.contains(block)) {
                    if (!(value instanceof Instruction && inLoopInstructions.contains(value))) {
                        throw new BuildFailedException();
                    }
                    if (exitLoopValue == null) {
                        exitLoopValue = value;
                    } else {
                        if (value != exitLoopValue) throw new BuildFailedException();
                    }
                } else {
                    if (initialValue == null) {
                        initialValue = value;
                    } else {
                        if (value != initialValue) throw new BuildFailedException();
                    }
                }
            }
            if (initialValue == null) throw new BuildFailedException();
            if (exitLoopValue == null) throw new BuildFailedException();
            this.initialValue = initialValue;
            this.exitLoopValue = exitLoopValue;
        }

        /**
         * 获取循环的退出条件语句 <br>
         * 如果有多个可能的退出条件，则返回null <br>
         *
         * @return 二元组(循环的唯一退出条件语句, 出口块)，或是null
         */
        private Pair<IntegerCompareInst, BasicBlock> getExitCondition() throws BuildFailedException {
            // 获取循环退出的条件值
            Value conditionValue = null;
            BranchInst br = null;
            for (BasicBlock inLoopBlock : inLoopBlocks) {
                var terminateInst = inLoopBlock.getTerminateInstruction();
                if (terminateInst instanceof BranchInst branchInst) {
                    for (BasicBlock exit : branchInst.getExits()) {
                        if (inLoopBlocks.contains(exit)) continue;
                        if (conditionValue == null) {
                            conditionValue = branchInst.getCondition();
                            br = branchInst;
                        } else {
                            // 多出口，不是简单循环
                            throw new BuildFailedException();
                        }
                    }
                } else if (terminateInst instanceof JumpInst jumpInst) {
                    assert inLoopBlocks.contains(jumpInst.getExit());
                } else {
                    // 未处理的结束语句
                    // 例如 ret, unreachable 等，他们都不该出现在循环中
                    throw new IllegalStateException();
                }
            }

            // 仅支持整数加法或减法的循环
            if (!(conditionValue instanceof IntegerCompareInst integerCompareInst)) {
                throw new BuildFailedException();
            }

            // 循环的退出块必须在循环头
            if (br.getBasicBlock() != loopHeadBlock) {
                throw new BuildFailedException();
            }

            // 标准化br语句
            if (!inLoopBlocks.contains(br.getTrueExit())) {
                // 交换exit
                var trueExitTemp = br.getTrueExit();
                var falseExitTemp = br.getFalseExit();
                // 避免trueExit和falseExit为相同值导致被entrySet去重
                br.setTrueExit(null);
                br.setFalseExit(null);
                br.setTrueExit(falseExitTemp);
                br.setFalseExit(trueExitTemp);
                // 新建反转的cmp语句
                // 此处不能删除旧的cmp语句，因为该语句可能还被其他语句使用
                var newCmpInstruction = new IntegerCompareInst(
                        integerCompareInst.getComparedType(),
                        integerCompareInst.getCondition().not(),
                        integerCompareInst.getOperand1(), integerCompareInst.getOperand2()
                );
                newCmpInstruction.insertBefore(integerCompareInst);
                br.setCondition(newCmpInstruction);
                // 在可能的情况下删除旧指令，以确保模块中没有死代码
                if (integerCompareInst.getUsages().isEmpty()) {
                    // 直接waste此指令即可，不会递归产生死代码
                    integerCompareInst.waste();
                }
                integerCompareInst = newCmpInstruction;
            }
            var exitBlock = br.getFalseExit();

            // 标准化比较语句，动态变量应当放置在左侧
            var op1 = integerCompareInst.getOperand1();
            var op2 = integerCompareInst.getOperand2();
            boolean isOp1Dynamic = op1 instanceof Instruction && inLoopInstructions.contains((Instruction) op1);
            boolean isOp2Dynamic = op2 instanceof Instruction && inLoopInstructions.contains((Instruction) op2);
            // 两侧都是常量或都是动态量，不属于简单循环
            if (isOp1Dynamic == isOp2Dynamic) {
                throw new BuildFailedException();
            }
            // 标准化的比较语句要求左侧是动态量，右侧是常量
            // 只需要检测op1是动态的，因为op1与op2不同
            IntegerCompareInst condition;
            if (isOp1Dynamic) {
                condition = integerCompareInst;
            } else {
                // 如果放反了，新建一个指令替换之
                var newCmpInstruction = new IntegerCompareInst(
                        integerCompareInst.getComparedType(),
                        integerCompareInst.getCondition().swap(),
                        op2, op1
                );
                integerCompareInst.replaceInstructionTo(newCmpInstruction);
                condition = newCmpInstruction;
            }
            return new Pair<>(condition, exitBlock);
        }

        private IntegerArithmeticInst getStepInstruction(Value exitLoopValue) throws BuildFailedException {
            // 仅支持整数加或减的循环
            if (!(exitLoopValue instanceof IntegerAddInst || exitLoopValue instanceof IntegerSubInst)) {
                throw new BuildFailedException();
            }
            var stepInstruction = (IntegerArithmeticInst) exitLoopValue;
            // 标准化步进语句，动态变量应当放置在左侧
            var op1 = stepInstruction.getOperand1();
            var op2 = stepInstruction.getOperand2();
            boolean isOp1Dynamic = op1 instanceof Instruction && inLoopInstructions.contains((Instruction) op1);
            boolean isOp2Dynamic = op2 instanceof Instruction && inLoopInstructions.contains((Instruction) op2);
            // 两侧都是常量或都是动态量，不属于简单循环
            if (isOp1Dynamic == isOp2Dynamic) {
                throw new BuildFailedException();
            }
            // 标准化的比较语句要求左侧是动态量，右侧是常量
            // 只需要检测op1是动态的，因为op1与op2不同
            if (isOp1Dynamic) {
                return stepInstruction;
            } else {
                // 如果放反了，新建一个指令替换之
                IntegerArithmeticInst newStepInstruction;
                if (stepInstruction instanceof IntegerAddInst) {
                    newStepInstruction = new IntegerAddInst(stepInstruction.getType(), op2, op1);
                } else {
                    // 减法无法交换
                    throw new BuildFailedException();
                }
                stepInstruction.replaceInstructionTo(newStepInstruction);
                return newStepInstruction;
            }
        }

    }

    public static SimpleLoopInfo buildFrom(Loop loop) {
        return new Builder(loop).getSimpleLoopInfo();
    }

    /**
     * 判断当前循环次数是否固定
     *
     * @return 循环次数固定返回true；否则返回false。
     */
    public boolean isFixedLoop() {
        return getLoopCount() != -1;
    }

    /**
     * 获取循环执行的次数
     *
     * @return 若循环执行固定次数，返回其执行的次数；否则返回-1。
     */
    public int getLoopCount() {
        if (!(initialValue instanceof ConstInt)) return -1;
        if (!(stepInst.getOperand2() instanceof ConstInt)) return -1;
        if (!(condition.getOperand2() instanceof ConstInt)) return -1;
        var initialValue = (long) ((ConstInt) this.initialValue).getValue();
        var stepValue = (long) ((ConstInt) stepInst.getOperand2()).getValue();
        if (stepInst instanceof IntegerSubInst) {
            stepValue = -stepValue;
        }
        var limitValue = (long) ((ConstInt) condition.getOperand2()).getValue();
        var condition = this.condition.getCondition();
        if (condition == IntegerCompareInst.Condition.SGE) {
            // 可以认为 limitValue-1 一定不会下溢，否则原循环是死循环
            limitValue = limitValue - 1;
            condition = IntegerCompareInst.Condition.SGT;
        }
        if (condition == IntegerCompareInst.Condition.SLE) {
            // 同样地，可以假设 limitValue+1 不会上溢
            limitValue = limitValue + 1;
            condition = IntegerCompareInst.Condition.SLT;
        }
        switch (condition) {
            case EQ -> {
                if (initialValue != limitValue) return 0;
                else return -1;
            }
            case NE -> {
                if (initialValue == limitValue) return 0;
                if (stepValue == 0) return -1;
                if ((limitValue - initialValue) % stepValue != 0) {
                    return -1;
                } else {
                    long count = (limitValue - initialValue) / stepValue;
                    if (count > Integer.MAX_VALUE || count < 0) return -1;
                    return (int) count;
                }
            }
            case SLT -> {
                var interval = limitValue - initialValue;
                if (interval <= 0) return 0;
                if (stepValue <= 0) return -1;
                long count = (interval + stepValue - 1) / stepValue;
                if (count > Integer.MAX_VALUE) return -1;
                return (int) count;
            }
            case SGT -> {
                var interval = limitValue - initialValue;
                if (interval >= 0) return 0;
                if (stepValue >= 0) return -1;
                interval = -interval;
                stepValue = -stepValue;
                long count = (interval + stepValue - 1) / stepValue;
                if (count > Integer.MAX_VALUE) return -1;
                return (int) count;
            }
        }
        return -1;
    }

}
