package mid.Optimizer.Loop;

import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.Instruction.Br;
import mid.IntermediatePresentation.Instruction.Call;
import mid.IntermediatePresentation.Instruction.GetElementPtr;
import mid.IntermediatePresentation.Instruction.Instruction;
import mid.IntermediatePresentation.Instruction.Load;
import mid.IntermediatePresentation.Instruction.Phi;
import mid.IntermediatePresentation.Instruction.Ret;
import mid.IntermediatePresentation.Instruction.Store;
import mid.IntermediatePresentation.Value;
import mid.Optimizer.Optimizer;
import mid.Optimizer.RedundancyElim.Straighten;
import utils.tools.Timer;

import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;

public class LoopFusion {

    public void optimize() {
        boolean flag;
        do {
            flag = tryFusion();
            if (flag) {
                Optimizer.instance().getLoopAnalyze().analyze();
                Optimizer.instance().loopOptAnalyze();
            }
        } while (flag && !Timer.INSTANCE.timeOut());
    }

    /*
     * 两个循环必须**相邻**(从preLoop到nextLoop间不存在带有副作用的指令)
     * 都是minLoop
     * 不存在负距离依赖项
     */

    private boolean tryFusion() {
        HashMap<Value, HashSet<MinLoop>> repeatNumToLoops = new HashMap<>();
        for (Loop loop : Optimizer.instance().getLoopAnalyze().getLoops()) {
            // 两个minLoop如果循环次数相同，则可以合并
            if (loop instanceof MinLoop minLoop) {
                repeatNumToLoops.putIfAbsent(minLoop.getRepeatNumber(), new HashSet<>());
                repeatNumToLoops.get(minLoop.getRepeatNumber()).add(minLoop);
            }
        }

        for (HashSet<MinLoop> minLoops : repeatNumToLoops.values()) {
            // 两个loop之间，preLoop的exit是nextLoop的支配者
            for (MinLoop preLoop : minLoops) {
                for (MinLoop nextLoop : minLoops) {
                    if (preLoop.equals(nextLoop)) {
                        continue;
                    }
                    BasicBlock preExiting = preLoop.getExiting();
                    BasicBlock nextHeader = nextLoop.getHeader();
                    if (Optimizer.instance().getDominAnalyzer().getDominees(preExiting).contains(nextHeader)) {
                        boolean canFusion = true;
                        // 进一步检查两者之间的指令，是否存在副作用
                        HashSet<BasicBlock> midBlocks = new HashSet<>(Optimizer.instance().getCFG().mayPassingBy(
                                preExiting, nextHeader
                        ));
                        midBlocks.remove(preExiting);
                        midBlocks.remove(nextHeader);
                        for (BasicBlock b : midBlocks) {
                            for (Instruction i : b.getInstructionList()) {
                                if (i instanceof Ret || i instanceof Store || i instanceof Call call &&
                                        Optimizer.instance().hasSideEffect(call.getCallingFunction())) {
                                    canFusion = false;
                                    break;
                                }
                            }
                            if (!canFusion) {
                                break;
                            }
                        }
                        if (!canFusion) {
                            continue;
                        }
                        if (!nonExistRely(preLoop, nextLoop)) {
                            continue;
                        }

                        fusion(preLoop, nextLoop);
                        Optimizer.instance().dominAnalyze(preLoop.getHeader().getFunction());
                        (new Straighten()).optimize();
                        return true;
                    }
                }
            }
        }
        return false;
    }

    private boolean nonExistRely(MinLoop preLoop, MinLoop nextLoop) {
        /*
           检查是否存在反依赖：
               * 对于load、store指令，其地址gep：
                    * 要么不指向同一个pointer
                    * 要么可以证明preLoop中的store一定不晚于nextLoop中的load，preLoop中的load一定不晚于nextLoop中的store
                       例如 while (i<n) { i++; a[i] = i}和 while (i<n) { i++; b[i] = a[i]}
               * 对于其他指令：
                    * 保证preLoop中的指令不存在副作用（也就是call）
                    * 并且nextLoop中的变量（的operand闭包）不会使用preLoop以及两循环中间块的变量（根据lcssa，只需要检查exit中的phi）
        */

        HashSet<BasicBlock> preLoopBlocks = preLoop.getBlocksInLoop();
        HashSet<BasicBlock> midBlocks = new HashSet<>(Optimizer.instance().getCFG().mayPassingBy(
                preLoop.getExiting(), nextLoop.getHeader()
        ));
        preLoopBlocks.addAll(midBlocks);
        preLoopBlocks.remove(nextLoop.getHeader());

        HashSet<BasicBlock> nextLoopBlocks = nextLoop.getBlocksInLoop();
        for (BasicBlock b : nextLoopBlocks) {
            for (Instruction i : b.getInstructionList()) {
                if (!(i instanceof Load || i instanceof Store || i instanceof Call)) {
                    LinkedList<Instruction> queue = new LinkedList<>();
                    HashSet<Instruction> visited = new HashSet<>();
                    queue.add(i);
                    visited.add(i);
                    while (!queue.isEmpty()) {
                        Instruction instruction = queue.poll();
                        for (Value operand : instruction.getOperandList()) {
                            if (operand instanceof Instruction opInstr && !queue.contains(operand) && !visited.contains(operand)) {
                                if (preLoopBlocks.contains(opInstr.getBlock())) {
                                    return false;
                                }
                                queue.add(opInstr);
                                visited.add(opInstr);
                            }
                        }
                    }
                }
            }
        }

        HashMap<Value, ScEvValue> preLoopStoreIndex = new HashMap<>();
        HashMap<Value, ScEvValue> preLoopLoadIndex = new HashMap<>();
        for (BasicBlock b : preLoopBlocks) {
            for (Instruction i : b.getInstructionList()) {
                if (i instanceof Call call && Optimizer.instance().hasSideEffect(call.getCallingFunction())) {
                    return false;
                }
                if (i instanceof Load load) {
                    if (load.getAddr() instanceof GetElementPtr gep && preLoop.getInds().containsKey(gep.getElemIndex())) {
                        preLoopLoadIndex.put(gep.getPtr(), preLoop.getInds().get(gep.getElemIndex()));
                    } else {
                        return false;
                    }
                }

                if (i instanceof Store store) {
                    if (store.getAddr() instanceof GetElementPtr gep && preLoop.getInds().containsKey(gep.getElemIndex())) {
                        preLoopStoreIndex.put(gep.getPtr(), preLoop.getInds().get(gep.getElemIndex()));
                    } else {
                        return false;
                    }
                }
            }
        }

        for (BasicBlock b : nextLoopBlocks) {
            for (Instruction i : b.getInstructionList()) {
                if (i instanceof Load load) {
                    if (load.getAddr() instanceof GetElementPtr gep && nextLoop.getInds().containsKey(gep.getElemIndex())) {
                        if (!preLoopStoreIndex.containsKey(gep.getPtr())) {
                            continue;
                        }
                        ScEvValue preStore = preLoopStoreIndex.get(gep.getPtr());
                        ScEvValue loadIdx = nextLoop.getInds().get(gep.getElemIndex());
                        ScEvValue.CmpResult cmpResult = preStore.compareTo(loadIdx);
                        if (preStore.compareTo(loadIdx) != ScEvValue.CmpResult.GEQ && cmpResult != ScEvValue.CmpResult.EQ) {
                            return false;
                        }
                    } else {
                        return false;
                    }
                }

                if (i instanceof Store store) {
                    if (store.getAddr() instanceof GetElementPtr gep && nextLoop.getInds().containsKey(gep.getElemIndex())) {
                        if (!preLoopStoreIndex.containsKey(gep.getPtr())) {
                            continue;
                        }
                        ScEvValue preLoad = preLoopLoadIndex.get(gep.getPtr());
                        ScEvValue storeIdx = nextLoop.getInds().get(gep.getElemIndex());
                        ScEvValue.CmpResult cmpResult = preLoad.compareTo(storeIdx);
                        if (cmpResult != ScEvValue.CmpResult.GEQ && cmpResult != ScEvValue.CmpResult.EQ) {
                            return false;
                        }
                    } else {
                        return false;
                    }
                }
            }
        }

        return true;
    }

    private void fusion(MinLoop preLoop, MinLoop nextLoop) {
        // preLoop的 latch->header 转到nextLoop的header，nextLoop的 latch->header 转到preLoop的header
        BasicBlock preLoopLatch = preLoop.getLatch();
        BasicBlock preLoopHeader = preLoop.getHeader();
        BasicBlock nextLoopLatch = nextLoop.getLatch();
        BasicBlock nextLoopHeader = nextLoop.getHeader();
        ((Br) preLoopLatch.getLastInstruction()).redirectTo(preLoopHeader, nextLoopHeader);
        ((Br) nextLoopLatch.getLastInstruction()).redirectTo(nextLoopHeader, preLoopHeader);
        for (Instruction i : preLoopHeader.getInstructionList()) {
            if (!(i instanceof Phi phi)) {
                break;
            }
            phi.redirectFrom(preLoopLatch, nextLoopLatch);
        }
        for (Instruction i : nextLoopHeader.getInstructionList()) {
            // nextLoop的header中的phi移动到preLoop的header
            if (!(i instanceof Phi phi)) {
                break;
            }
            phi.redirectFrom(nextLoop.getPreheader(), preLoop.getPreheader());
            preLoopHeader.addInstrAtEntry(phi);
        }
        nextLoopHeader.getInstructionList().removeIf(i -> i instanceof Phi);

        // 去掉preLoop和nextLoop的 exiting-> exit，改为由nextLoop的exiting转入
        BasicBlock preLoopExiting = preLoop.getExiting();
        BasicBlock preLoopExit = preLoop.getExit();
        BasicBlock nextLoopExiting = nextLoop.getExiting();
        BasicBlock nextLoopExit = nextLoop.getExit();
        ((Br) nextLoopExiting.getLastInstruction()).redirectTo(nextLoopExit, preLoopExit);
        for (Instruction i : nextLoopExit.getInstructionList()) {
            if (!(i instanceof Phi phi)) {
                break;
            }
            phi.removeOperand(nextLoopExiting);
        }
        for (Instruction i : preLoopExit.getInstructionList()) {
            if (!(i instanceof Phi phi)) {
                break;
            }
            phi.redirectFrom(preLoopExiting, nextLoopExiting);
        }

        Br preLoopExitingBr = (Br) preLoopExiting.getLastInstruction();
        // minLoop中的exiting一定不止一个方向
        BasicBlock brDest = null;
        for (Value v : preLoopExitingBr.getOperandList()) {
            if (v instanceof BasicBlock blk && !blk.equals(preLoopExit)) {
                brDest = blk;
            }
        }
        Br newBr = new Br(brDest);
        preLoopExitingBr.beReplacedBy(newBr);
        preLoopExiting.addInstructionBeforeBranch(newBr);
        preLoopExiting.removeInstruction(preLoopExitingBr);
        preLoopExitingBr.destroy();

        for (Instruction instr : preLoopExit.getInstructionList()) {
            if (instr instanceof Phi phi) {
                phi.redirectFrom(preLoopExiting, nextLoopExiting);
            } else {
                break;
            }
        }

        // nextLoop的preheader->header转为preheader->exit
        BasicBlock nextLoopPreheader = nextLoop.getPreheader();
        ((Br) nextLoopPreheader.getLastInstruction()).redirectTo(nextLoopHeader, nextLoopExit);
        for (Instruction i : nextLoopHeader.getInstructionList()) {
            if (!(i instanceof Phi phi)) {
                continue;
            }
            phi.removeOperand(nextLoopPreheader);
        }
        for (Instruction i : nextLoopExit.getInstructionList()) {
            if (!(i instanceof Phi phi)) {
                continue;
            }
            phi.redirectFrom(nextLoopExiting, nextLoopPreheader);
        }
    }
}
