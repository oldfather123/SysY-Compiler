package cn.edu.bit.newnewcc.pass.ir;

import cn.edu.bit.newnewcc.ir.Module;
import cn.edu.bit.newnewcc.ir.type.IntegerType;
import cn.edu.bit.newnewcc.ir.value.BasicBlock;
import cn.edu.bit.newnewcc.ir.value.Function;
import cn.edu.bit.newnewcc.ir.value.Instruction;
import cn.edu.bit.newnewcc.ir.value.constant.ConstInt;
import cn.edu.bit.newnewcc.ir.value.instruction.IntegerAddInst;
import cn.edu.bit.newnewcc.ir.value.instruction.IntegerSignedRemainderInst;
import cn.edu.bit.newnewcc.pass.ir.structure.I32ValueRangeAnalyzer;

public class IntegerSumModuloCombinePass {

    private final Function function;
    private final I32ValueRangeAnalyzer analyzer;

    private IntegerSumModuloCombinePass(Function function) {
        this.function = function;
        this.analyzer = I32ValueRangeAnalyzer.analysis(function);
    }

    private boolean tryEliminateExtraModules(IntegerSignedRemainderInst remainderInst) {
        var instBlock = remainderInst.getBasicBlock();
        if (remainderInst.getType() != IntegerType.getI32()) return false;
        if (!(remainderInst.getOperand2() instanceof ConstInt constMod)) return false;
        if (!(remainderInst.getOperand1() instanceof IntegerAddInst addInst)) return false;
        if (analyzer.getValueRangeAtBlock(addInst, instBlock).minValue() < 0) return false;
        if (addInst.getOperand1() instanceof IntegerSignedRemainderInst subRemainder) {
            if (subRemainder.getOperand2() == constMod) {
                var subAddendBeforeMod = subRemainder.getOperand1();
                try {
                    Math.addExact(analyzer.getValueRangeAtBlock(subAddendBeforeMod, instBlock).maxValue(),
                            analyzer.getValueRangeAtBlock(addInst.getOperand2(), instBlock).maxValue());
                    addInst.setOperand1(subAddendBeforeMod);
                    analyzer.onInstructionUpdated(addInst);
                    return true;
                } catch (ArithmeticException ignored) {
                }
            }
        }
        if (addInst.getOperand2() instanceof IntegerSignedRemainderInst subRemainder) {
            if (subRemainder.getOperand2() == constMod) {
                var subAddendBeforeMod = subRemainder.getOperand1();
                try {
                    Math.addExact(analyzer.getValueRangeAtBlock(subAddendBeforeMod, instBlock).maxValue(),
                            analyzer.getValueRangeAtBlock(addInst.getOperand1(), instBlock).maxValue());
                    addInst.setOperand2(subAddendBeforeMod);
                    analyzer.onInstructionUpdated(addInst);
                    return true;
                } catch (ArithmeticException ignored) {
                }
            }
        }
        return false;
    }

    private boolean run() {
        boolean changed = false;
        for (BasicBlock basicBlock : function.getBasicBlocks()) {
            for (Instruction instruction : basicBlock.getInstructions()) {
                if (instruction instanceof IntegerSignedRemainderInst integerSignedRemainderInst) {
                    while (true) {
                        boolean eliminateResult = tryEliminateExtraModules(integerSignedRemainderInst);
                        changed |= eliminateResult;
                        if (!eliminateResult) break;
                    }
                }
            }
        }
        return changed;
    }

    public static boolean runOnFunction(Function function) {
        return new IntegerSumModuloCombinePass(function).run();
    }

    public static boolean runOnModule(Module module) {
        boolean changed = false;
        changed |= DeadCodeEliminationPass.runOnModule(module);
        for (Function function : module.getFunctions()) {
            changed |= runOnFunction(function);
        }
        return changed;
    }
}
