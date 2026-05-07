package midend.SSA;

import Utils.CustomList;
import frontend.ir.DataType;
import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.otherop.CallInstr;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.instr.terminator.JumpInstr;
import frontend.ir.instr.terminator.ReturnInstr;
import frontend.ir.instr.terminator.Terminator;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;

import java.util.*;

/**
 * Function inlining
 * 函数内联
 * 值得注意的是，函数内联并不一定能提高运行效率，有时候一些不常用的代码（冷代码）可能作为函数调用存在会更合适一些
 */
public class FI {
    public static void execute(ArrayList<Function> functions) {
        if (functions == null) {
            throw new NullPointerException();
        }
        for (Function function : functions) {
            BasicBlock basicBlock = (BasicBlock) function.getBasicBlocks().getHead();
            while (basicBlock != null) {
                Instruction instr = (Instruction) basicBlock.getInstructions().getHead();
                while (instr != null) {
                    if (!(instr instanceof CallInstr) || ((CallInstr) instr).callsLibFunc()) {
                        instr = (Instruction) instr.getNext();
                        continue;
                    }
                    Function callee = (Function) (((CallInstr) instr).getFuncDef());
                    
                    // 对于返回值无用而且没有副作用的函数调用可以直接删掉
                    if (instr.getBeginUse() == null && callee.checkNoSideEffect()) {
                        instr.removeFromList();
                        callee.minusCall();
                        instr = (Instruction) instr.getNext();
                        continue;
                    }
                    
                    boolean doNotInline = callee.canBeMemorized();    // 优先记忆化（放弃了，性能不好
                    // doNotInline = callee.getCalledCnt() > 3 && callee.checkInsTooMany() && !(instr.getParentBB().getLoopDepth() > 1);
                    if (callee.isRecursive()) { // 现版本不做更多限制，能联都联
                        instr = (Instruction) instr.getNext();
                        continue;
                    }
                    
                    callee.minusCall();
                    
                    // 将原本的基本块拆成两个
                    Instruction nextIns = (Instruction) instr.getNext();
                    int curDepth = basicBlock.getLoopDepth();
                    BasicBlock nextBB = new BasicBlock(curDepth, function.getAndAddBlkIndex());
                    nextBB.insertAfter(basicBlock);
                    
                    while (nextIns != null) {
                        Instruction tmp = (Instruction) nextIns.getNext();
                        if (nextIns instanceof Terminator) {
                            nextBB.addInstruction(nextIns.cloneShell(function));
                            nextIns.removeFromList();
                        } else {
                            nextIns.removeFromListWithUseRemain();
                            nextBB.addInstruction(nextIns);
                        }
                        if (nextIns.getPrev() == instr) {
                            nextIns.setPrev(null);
                        }
                        nextIns = tmp;
                    }
                    
                    List<Value> rParams = ((CallInstr) instr).getRParams();
                    ArrayList<BasicBlock> bbList = callee.func2blocks(curDepth, rParams, function);
                    
                    Collections.reverse(bbList);
                    
//                    JumpInstr prev2func = new JumpInstr(bbList.get(bbList.size() - 1));
                    basicBlock.setRet(false);
                    BasicBlock firstBlkInFunc = bbList.get(bbList.size() - 1);
                    bbList.remove(firstBlkInFunc);
                    Instruction insInFirstBlk = (Instruction) firstBlkInFunc.getInstructions().getHead();
                    while (insInFirstBlk != null) {
                        Instruction tmp = (Instruction) insInFirstBlk.getNext();
                        if (insInFirstBlk instanceof Terminator) {
                            basicBlock.addInstruction(insInFirstBlk.cloneShell(function));
                            insInFirstBlk.removeFromList();
                        } else {
                            insInFirstBlk.removeFromListWithUseRemain();
                            basicBlock.addInstruction(insInFirstBlk);
                        }
                        
                        insInFirstBlk = tmp;
                    }

                    for (BasicBlock block : bbList) {
                        CustomList.Node node = block.getInstructions().getHead();
                        while (node instanceof PhiInstr) {
                            ((PhiInstr) node).modifyPrtBlk(firstBlkInFunc, basicBlock);
                            node = node.getNext();
                        }
                    }
                    
                    for (BasicBlock newBB : bbList) {
                        newBB.insertAfter(basicBlock);
                    }
                    
                    BasicBlock funcLastBB = bbList.get(0);
                    ReturnInstr retIns = (ReturnInstr) funcLastBB.getEndInstr();
                    if (retIns.getDataType() != DataType.VOID) {
                        instr.replaceUseTo(retIns.getReturnValue());
                    }
                    retIns.removeFromList();
                    funcLastBB.setRet(false);
                    JumpInstr func2next = new JumpInstr(nextBB);
                    funcLastBB.addInstruction(func2next);
                    
                    HashMap<Value, Value> old2new = new HashMap<>();
                    old2new.put(basicBlock, nextBB);
                    for (BasicBlock block : nextBB.getSucs()) {
                        Instruction ins = (Instruction) block.getInstructions().getHead();
                        while (ins instanceof PhiInstr) {
                            ((PhiInstr) ins).renewBlocks(old2new);
                            ins = (Instruction) ins.getNext();
                        }
                    }
                    
                    
                    instr.removeFromList();
                    instr = (Instruction) instr.getNext();
                }
                basicBlock = (BasicBlock) basicBlock.getNext();
            }
            
            function.allocaRearrangement();
        }
    }
}
