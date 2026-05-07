package mid.Optimizer.Loop;

import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.Function.Function;
import mid.IntermediatePresentation.Instruction.Br;
import mid.IntermediatePresentation.Instruction.Call;
import mid.IntermediatePresentation.Instruction.Cmp;
import mid.IntermediatePresentation.Instruction.GetElementPtr;
import mid.IntermediatePresentation.Instruction.Instruction;
import mid.IntermediatePresentation.Instruction.Load;
import mid.IntermediatePresentation.Instruction.Phi;
import mid.IntermediatePresentation.Instruction.Ret;
import mid.IntermediatePresentation.Instruction.Store;
import mid.IntermediatePresentation.Value;
import mid.Optimizer.Optimizer;
import mid.Optimizer.RedundancyElim.GCM;

import java.util.ArrayList;
import java.util.HashSet;

public class LoopInterChange {
    /*
        todo:只支持嵌套二重MinLoop，不考虑同时读写，这玩意也太难了，实在绷不住了
     */
    private HashSet<ArrayList<Loop>> loopsToInterchange;

    public void optimize() {
        // 在这种简单情形下，interchange是不需要迭代到不动点的，优化一次即可
        // 匹配符合条件的循环
        loopsToInterchange = new HashSet<>(Optimizer.instance().getLoopAnalyze().getNestedLoops());
        patternMatch();
        // 检查循环依赖
        checkRely();
        // 进行交换
        tryInterChange();
        (new GCM()).optimize();
        Optimizer.instance().dominAnalyze();
        Optimizer.instance().getLoopAnalyze().analyze();
        Optimizer.instance().loopOptAnalyze();
    }

    private void patternMatch() {
        // 这里检查一下循环有咩问题
        loopsToInterchange.removeIf(list -> list.size() < 2);
        for (ArrayList<Loop> loops : new HashSet<>(loopsToInterchange)) {
            if (loops.size() > 2) {
                loops.subList(loops.size() - 2, loops.size()).clear();
            }
            // 1. 均为minLoop
            if (!(loops.stream().allMatch(loop -> loop instanceof MinLoop))) {
                loopsToInterchange.remove(loops);
                continue;
            }

            // 2. 循环之间不存在包含副作用的指令
            // 这些副作用指令只要在两个循环的内部，就不会出问题，所以只需要检查在prevLoop中而不在nextLoop中的
            MinLoop prevLoop = (MinLoop) loops.get(0);
            MinLoop nextLoop = (MinLoop) loops.get(1);
            HashSet<BasicBlock> midBlocks = new HashSet<>(prevLoop.getBlocksInLoop());
            midBlocks.removeAll(nextLoop.getBlocksInLoop());
            boolean hasSideEffect = false;
            for (BasicBlock b : midBlocks) {
                if (b.getInstructionList().stream().anyMatch(instr ->
                        (instr instanceof Call call && Optimizer.instance().hasSideEffect(call.getCallingFunction())
                                || instr instanceof Store || instr instanceof Ret))) {
                    hasSideEffect = true;
                    break;
                }
            }
            if (hasSideEffect) {
                loopsToInterchange.remove(loops);
                continue;
            }

            // 3. 没有其他子循环
            if (Optimizer.instance().getLoopAnalyze().subLoopCount(prevLoop) != 1 ||
                    Optimizer.instance().getLoopAnalyze().subLoopCount(nextLoop) != 0) {
                loopsToInterchange.remove(loops);
                continue;
            }

            // 4. 所有循环退出条件中的归纳变量的step都是1
            Value cond = ((Br) prevLoop.getExiting().getLastInstruction()).getCond();
            if (!(cond instanceof Cmp cmp)) {
                loopsToInterchange.remove(loops);
                continue;
            }
            boolean flag = prevLoop.getInds().containsKey(cmp.getOperandList().get(0)) &&
                    prevLoop.getInds().get(cmp.getOperandList().get(0)).getStepVal() instanceof ConstNumber number
                    && number.getVal().floatValue() == 1;

            flag |= prevLoop.getInds().containsKey(cmp.getOperandList().get(1)) &&
                    prevLoop.getInds().get(cmp.getOperandList().get(1)).getStepVal() instanceof ConstNumber number
                    && number.getVal().floatValue() == 1;

            if (!flag) {
                loopsToInterchange.remove(loops);
                continue;
            }

            cond = ((Br) nextLoop.getExiting().getLastInstruction()).getCond();
            if (!(cond instanceof Cmp cmpd)) {
                loopsToInterchange.remove(loops);
                continue;
            }
            flag = nextLoop.getInds().containsKey(cmpd.getOperandList().get(0)) &&
                    nextLoop.getInds().get(cmpd.getOperandList().get(0)).getStepVal() instanceof ConstNumber number
                    && number.getVal().floatValue() == 1;
            flag |= nextLoop.getInds().containsKey(cmpd.getOperandList().get(1)) &&
                    nextLoop.getInds().get(cmpd.getOperandList().get(1)).getStepVal() instanceof ConstNumber number
                    && number.getVal().floatValue() == 1;

            if (!flag) {
                loopsToInterchange.remove(loops);
                continue;
            }

            // 5. exiting = latch
            if (!prevLoop.getExiting().equals(prevLoop.getLatch()) ||
                    !nextLoop.getExiting().equals(nextLoop.getLatch())) {
                loopsToInterchange.remove(loops);
            }
        }
    }

    private void checkRely() {
        /*
            假如确定了需要重排序，则需要检查内部的依赖关系：
                load对应的每个loop related index中的所有ind，都不晚于其store相对应的ind，暂时先直接读写不同时出现
                内部循环的header不使用外部循环的header中的变量，外部循环的exiting不使用内部循环中的变量
         */
        for (ArrayList<Loop> loops : new HashSet<>(loopsToInterchange)) {
            HashSet<Value> reads = new HashSet<>();
            HashSet<Value> writes = new HashSet<>();
            MinLoop outterLoop = (MinLoop) loops.get(0);
            MinLoop innerLoop = (MinLoop) loops.get(1);
            boolean flag = false;
            for (BasicBlock b : innerLoop.getBlocksInLoop()) {
                for (Instruction i : b.getInstructionList()) {
                    Value addr = null;
                    if (i instanceof Load load) {
                        addr = load.getAddr();
                        reads.add(addr);
                    } else if (i instanceof Store store) {
                        addr = store.getAddr();
                        writes.add(addr);
                    }
                    if (addr != null && reads.contains(addr) && writes.contains(addr)) {
                        flag = true;
                        break;
                    }
                }
                if (flag) {
                    break;
                }
            }
            if (flag || reads.size() == 0 && writes.size() == 0) {
                loopsToInterchange.remove(loops);
                continue;
            }

            BasicBlock outterLoopHeader = outterLoop.getHeader();
            for (Instruction i : innerLoop.getHeader().getInstructionList()) {
                if (i.getOperandList().stream().anyMatch(op -> op instanceof Instruction opInstr &&
                        opInstr.getBlock().equals(outterLoopHeader))) {
                    flag = true;
                    break;
                }
            }
            if (flag) {
                loopsToInterchange.remove(loops);
                continue;
            }
            for (Instruction i : outterLoop.getExiting().getInstructionList()) {
                if (i.getOperandList().stream().anyMatch(op -> op instanceof Instruction opInstr &&
                        innerLoop.getBlocksInLoop().contains(opInstr.getBlock()))) {
                    flag = true;
                    break;
                }
            }
            if (flag) {
                loopsToInterchange.remove(loops);
            }
        }
    }

    private void tryInterChange() {
        /*
            这里的循环都是二重minLoop，读写不同时存在
            对于每个ld/sd addr，为gep index, ptr，其index = ind1 + ind2 * size
            这里index实际上应该是innerLoop的归纳变量，其step=1则eval+1，否则eval-1，代表顺序有误
            若eval<0,则交换内外循环顺序
         */
        for (ArrayList<Loop> loops : loopsToInterchange) {
            MinLoop outterLoop = (MinLoop) loops.get(0);
            MinLoop innerLoop = (MinLoop) loops.get(1);
            int eval = 0;
            for (BasicBlock b : outterLoop.getBlocksInLoop()) {
                for (Instruction i : b.getInstructionList()) {
                    GetElementPtr gep = null;
                    if (i instanceof Load load) {
                        gep = (GetElementPtr) load.getAddr();
                    } else if (i instanceof Store store) {
                        gep = (GetElementPtr) store.getAddr();
                    }
                    if (gep != null) {
                        Value index = gep.getElemIndex();
                        if (innerLoop.getInds().containsKey(index)) {
                            ScEvValue scEvValue = innerLoop.getInds().get(index);
                            if (scEvValue.getStepVal() instanceof ConstNumber n && n.getVal().floatValue() == 1) {
                                eval++;
                            } else {
                                eval--;
                            }
                        } else if (!innerLoop.isConstant(index)) {
                            // 如果并非归纳变量，而又依赖于循环，则应该保守地认为不应该交换
                            eval++;
                        }
                    }
                }
            }

            if (eval < 0) {
                /*
                    可以交换内外循环，将内层循环的归纳变量外提，形成新的最外层循环，有一个header和一个latch(exiting)就可以
                    将innerLoop的循环次数强制改为1，这里检查过exiting=latch，只需要改成无条件跳往exit即可
                    之后可能需要gcm，因为innerLoopPreHeader中的指令不再支配其归纳变量
                 */
                BasicBlock newOutterLoopHeader = new BasicBlock();
                BasicBlock newOutterLoopLatch = new BasicBlock();
                Function f = outterLoop.getHeader().getFunction();
                f.addBlock(newOutterLoopHeader);
                f.addBlock(newOutterLoopLatch);
                // 将内层循环的归纳变量删去，放到header中
                for (Value v : innerLoop.getInds().keySet()) {
                    if (v instanceof Instruction i) {
                        newOutterLoopHeader.addInstruction(i);
                        innerLoop.getHeader().removeInstruction(i);
                    }
                }
                // 在outterLoop的preHeader和header之间插入新的header
                BasicBlock outterLoopPreHeader = outterLoop.getPreheader();
                ((Br) outterLoopPreHeader.getLastInstruction()).redirectTo(outterLoop.getHeader(), newOutterLoopHeader);
                for (Instruction i : newOutterLoopHeader.getInstructionList()) {
                    if (!(i instanceof Phi phi)) {
                        break;
                    }
                    phi.redirectFrom(innerLoop.getPreheader(), outterLoopPreHeader);
                }
                BasicBlock outterLoopHeader = outterLoop.getHeader();
                Br newOutterLHToOutterLH = new Br(outterLoopHeader);
                newOutterLoopHeader.addInstruction(newOutterLHToOutterLH);
                for (Instruction i : outterLoopHeader.getInstructionList()) {
                    if (!(i instanceof Phi phi)) {
                        break;
                    }
                    phi.redirectFrom(outterLoopPreHeader, newOutterLoopHeader);
                }
                // 在outterLoop的latch(exiting)和exit之间插入新的latch(exiting)
                BasicBlock outterLoopLatch = outterLoop.getLatch();
                BasicBlock outterLoopExit = outterLoop.getExit();
                ((Br) outterLoopLatch.getLastInstruction()).redirectTo(outterLoopExit, newOutterLoopLatch);
                for (Instruction i : outterLoopExit.getInstructionList()) {
                    if (!(i instanceof Phi phi)) {
                        continue;
                    }
                    phi.redirectFrom(outterLoopLatch, newOutterLoopLatch);
                }
                Br newOutterLTToOutterExit = new Br(outterLoopExit);
                newOutterLoopLatch.addInstruction(newOutterLTToOutterExit);

                //将latch的跳转改为跳往exit
                BasicBlock innerLoopLatch = innerLoop.getLatch();
                Br innerLoopLatchToExit = new Br(innerLoop.getExit());
                innerLoopLatch.getLastInstruction().destroy();
                innerLoopLatch.deleteLastInstruction();
                innerLoopLatch.addInstruction(innerLoopLatchToExit);
                for (Instruction i : innerLoop.getHeader().getInstructionList()) {
                    if (!(i instanceof Phi phi)) {
                        break;
                    }
                    phi.removeOperand(innerLoopLatch);
                }
                System.out.println("int fins");
            }
        }
    }
}
