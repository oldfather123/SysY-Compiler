package cn.edu.bit.newnewcc.pass.ir;

import cn.edu.bit.newnewcc.ir.Module;
import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.type.IntegerType;
import cn.edu.bit.newnewcc.ir.value.BasicBlock;
import cn.edu.bit.newnewcc.ir.value.Function;
import cn.edu.bit.newnewcc.ir.value.Instruction;
import cn.edu.bit.newnewcc.ir.value.constant.ConstBool;
import cn.edu.bit.newnewcc.ir.value.constant.ConstInt;
import cn.edu.bit.newnewcc.ir.value.constant.ConstLong;
import cn.edu.bit.newnewcc.ir.value.instruction.*;
import cn.edu.bit.newnewcc.pass.ir.structure.Loop;
import cn.edu.bit.newnewcc.pass.ir.structure.LoopForest;

import java.util.HashSet;
import java.util.Set;

/**
 * Integer Sum Module To Integer Multiply Module Pass
 */
public class IsmToImmPass {

    private static class Optimizer {

        private final Loop loop;
        private final Set<Instruction> internalInstructions = new HashSet<>();


        private BasicBlock condBlock, bodyBlock, exitBlock;
        private BranchInst loopExitBr;
        private IntegerCompareInst loopCond;
        private PhiInst sumBefore, iBefore;
        private IntegerAddInst sumAdded;
        private IntegerSignedRemainderInst sumAfter;
        private IntegerAddInst iAfter;
        private Set<BasicBlock> externalEntries = new HashSet<>();
        private Value initialI = null, initialSum = null, stepValue = null;
        private Value externalLimit = null, externalModulo = null;

        public Optimizer(Loop loop) {
            this.loop = loop;
            if (!loop.getSubLoops().isEmpty()) {
                // 不优化含有其他循环的
                throw new RuntimeException();
            }
            for (BasicBlock basicBlock : loop.getBasicBlocks()) {
                internalInstructions.addAll(basicBlock.getInstructions());
            }
        }

        private boolean isInternalBlock(BasicBlock basicBlock) {
            return loop.getBasicBlocks().contains(basicBlock);
        }

        private boolean isInternalValue(Value value) {
            if (!(value instanceof Instruction instruction)) return false;
            return internalInstructions.contains(instruction);
        }

        private boolean checkFormat() {
            if (condBlock.getInstructions().size() != 4) return false;
            if (bodyBlock.getInstructions().size() != 4) return false;
            // i_before
            if (iBefore.getBasicBlock() != condBlock) return false;
            for (BasicBlock entry : iBefore.getEntrySet()) {
                var value = iBefore.getValue(entry);
                if (entry == bodyBlock) {
                    if (value != iAfter) return false;
                } else {
                    if (value != initialI) return false;
                }
            }
            if (isInternalValue(initialI)) return false;
            if (initialI.getType() != IntegerType.getI32()) return false;
            // sum_before
            if (sumBefore.getBasicBlock() != condBlock) return false;
            for (BasicBlock entry : sumBefore.getEntrySet()) {
                var value = sumBefore.getValue(entry);
                if (entry == bodyBlock) {
                    if (value != sumAfter) return false;
                } else {
                    if (value != initialSum) return false;
                }
            }
            if (isInternalValue(initialSum)) return false;
            if (initialSum.getType() != IntegerType.getI32()) return false;
            // loop_cond
            if (loopCond.getBasicBlock() != condBlock) return false;
            if (loopCond.getCondition() != IntegerCompareInst.Condition.SLT) return false;
            if (loopCond.getOperand1() != iBefore) return false;
            if (isInternalValue(loopCond.getOperand2())) return false;
            // cond br
            if (loopExitBr.getBasicBlock() != condBlock) return false;
            if (loopExitBr.getCondition() != loopCond) return false;
            if (loopExitBr.getTrueExit() != bodyBlock) return false;
            if (loopExitBr.getFalseExit() != exitBlock) return false;
            // sum_added
            if (sumAdded.getBasicBlock() != bodyBlock) return false;
            if (!((sumAdded.getOperand1() == sumBefore && sumAdded.getOperand2() == stepValue) ||
                    (sumAdded.getOperand2() == sumBefore && sumAdded.getOperand1() == stepValue))) return false;
            if (isInternalValue(stepValue)) return false;
            // sum_after
            if (sumAfter.getBasicBlock() != bodyBlock) return false;
            if (sumAfter.getOperand1() != sumAdded) return false;
            if (isInternalValue(externalModulo)) return false;
            // i_after
            if (iAfter.getBasicBlock() != bodyBlock) return false;
            if (iAfter.getOperand1() != iBefore) return false;
            if (iAfter.getOperand2() != ConstInt.getInstance(1)) return false;
            return true;
        }

        //cond_block:
        //    %i_before = phi i32 [ %i_after, %body_block ], [ %initial_i, %initial_entry ]
        //    %sum_before = phi i32 [ %sum_after, %body_block ], [ %initial_sum, %initial_entry ]
        //    %loop_cond = icmp slt i32 %i_before, %limit_i ; 为了控制循环次数，此处强制要求符号为 <
        //    br i1 %loop_cond, label %body_block, label %exit_block
        //body_block:
        //    %sum_added = add i32 %sum_before, %external_constant
        //    %sum_after = srem i32 %sum_added, %external_module
        //    %i_after = add i32 %i_before, 1 ; 为了控制循环次数，此处强制要求操作为+1
        //    br label %cond_block
        public boolean tryOptimize() {
            // 只优化最简单的（上文描述的）加模模式
            if (loop.getBasicBlocks().size() > 2) return false;
            // cond block
            condBlock = loop.getHeaderBasicBlock();
            // loop exit br
            if (!(condBlock.getTerminateInstruction() instanceof BranchInst loopExitBr_)) return false;
            loopExitBr = loopExitBr_;
            // body block
            if (!isInternalBlock(loopExitBr.getTrueExit())) return false;
            if (loopExitBr.getTrueExit() == condBlock) return false;
            bodyBlock = loopExitBr.getTrueExit();
            if (!(bodyBlock.getTerminateInstruction() instanceof JumpInst bodyJump)) return false;
            if (bodyJump.getExit() != condBlock) return false;
            // exit block
            if (isInternalBlock(loopExitBr.getFalseExit())) return false;
            exitBlock = loopExitBr.getFalseExit();
            // loop cond
            if (!(loopExitBr.getCondition() instanceof IntegerCompareInst integerCompareInst)) return false;
            if (integerCompareInst.getBasicBlock() != condBlock) return false;
            if (isInternalValue(integerCompareInst.getOperand2())) return false;
            if (!isInternalValue(integerCompareInst.getOperand1())) return false;
            loopCond = integerCompareInst;
            externalLimit = integerCompareInst.getOperand2();
            // i before
            if (!(integerCompareInst.getOperand1() instanceof PhiInst iBefore_)) return false;
            iBefore = iBefore_;
            // initial i, external entries
            for (BasicBlock entry : iBefore.getEntrySet()) {
                if (entry != bodyBlock) {
                    externalEntries.add(entry);
                    if (initialI == null) {
                        initialI = iBefore.getValue(entry);
                    } else {
                        if (initialI != iBefore.getValue(entry)) {
                            return false;
                        }
                    }
                } else {
                    if (!(iBefore.getValue(entry) instanceof IntegerAddInst iAfter_)) return false;
                    iAfter = iAfter_;
                }
            }
            if (initialI == null) return false;
            if (iAfter == null) return false;
            // sum
            for (Instruction instruction : bodyBlock.getInstructions()) {
                if (instruction != iAfter && !(instruction instanceof TerminateInst)) {
                    if (instruction instanceof IntegerAddInst sumAdded_) {
                        sumAdded = sumAdded_;
                        if (isInternalValue(sumAdded.getOperand1())) {
                            if (!(sumAdded.getOperand1() instanceof PhiInst sumBefore_)) return false;
                            sumBefore = sumBefore_;
                            stepValue = sumAdded.getOperand2();
                        } else if (isInternalValue(sumAdded.getOperand2())) {
                            if (!(sumAdded.getOperand2() instanceof PhiInst sumBefore_)) return false;
                            sumBefore = sumBefore_;
                            stepValue = sumAdded.getOperand1();
                        } else {
                            return false;
                        }
                    } else if (instruction instanceof IntegerSignedRemainderInst sumAfter_) {
                        sumAfter = sumAfter_;
                        externalModulo = sumAfter.getOperand2();
                    }
                }
            }
            if (sumAdded == null || sumAfter == null) return false;
            for (BasicBlock entry : sumBefore.getEntrySet()) {
                if (entry != bodyBlock) {
                    initialSum = sumBefore.getValue(entry);
                }
            }
            if (initialSum == null) return false;
            if (!checkFormat()) return false;
            reconstruct();
            return true;
        }

        private void reconstruct() {
            // reconstruct
            var initialI64 = new SignedExtensionInst(initialI, IntegerType.getI64());
            var limitI64 = new SignedExtensionInst(externalLimit, IntegerType.getI64());
            var fakeLoopCount = new IntegerSubInst(IntegerType.getI64(), limitI64, initialI64);
            var loopCount = new SignedMaxInst(IntegerType.getI64(), fakeLoopCount, ConstLong.getInstance(0));
            var stepValue64 = new SignedExtensionInst(stepValue, IntegerType.getI64());
            var sumDelta = new IntegerMultiplyInst(IntegerType.getI64(), stepValue64, loopCount);
            var initialSum64 = new SignedExtensionInst(initialSum, IntegerType.getI64());
            var sumNotModulo = new IntegerAddInst(IntegerType.getI64(), initialSum64, sumDelta);
            var modulo64 = new SignedExtensionInst(externalModulo, IntegerType.getI64());
            var sumModulo = new IntegerSignedRemainderInst(IntegerType.getI64(), sumNotModulo, modulo64);
            var truncSum = new TruncInst(sumModulo, IntegerType.getI32());
            condBlock.addInstruction(initialI64);
            condBlock.addInstruction(limitI64);
            condBlock.addInstruction(fakeLoopCount);
            condBlock.addInstruction(loopCount);
            condBlock.addInstruction(stepValue64);
            condBlock.addInstruction(sumDelta);
            condBlock.addInstruction(initialSum64);
            condBlock.addInstruction(sumNotModulo);
            condBlock.addInstruction(modulo64);
            condBlock.addInstruction(sumModulo);
            condBlock.addInstruction(truncSum);
            sumBefore.replaceAllUsageTo(truncSum);
            loopExitBr.setCondition(ConstBool.getInstance(false));
        }

    }

    private static boolean dfsLoops(Loop loop) {
        if (loop.getSubLoops().isEmpty()) {
            return new Optimizer(loop).tryOptimize();
        } else {
            for (Loop subLoop : loop.getSubLoops()) {
                if (dfsLoops(subLoop)) return true;
            }
            return false;
        }
    }

    public static boolean runOnFunction(Function function) {
        var loopForest = LoopForest.buildOver(function);
        for (Loop rootLoop : loopForest.getRootLoops()) {
            if (dfsLoops(rootLoop)) return true;
        }
        return false;
    }

    public static boolean runOnModule(Module module) {
        boolean changed = false;
        for (Function function : module.getFunctions()) {
            while (runOnFunction(function)) {
                changed = true;
            }
        }
        return changed;
    }
}
