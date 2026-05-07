package midend;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.otherop.EmptyInstr;
import frontend.ir.instr.otherop.MoveInstr;
import frontend.ir.instr.otherop.PCInstr;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.instr.terminator.BranchInstr;
import frontend.ir.instr.terminator.JumpInstr;
import frontend.ir.instr.terminator.ReturnInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;

import java.util.*;

public class RemovePhi {
    public static void execute(List<Function> functions) {
        for (Function function : functions) {
            prepareMidBlk(function);
            phi2pc(function);
            pc2move(function);
        }
    }
    
    private static void pc2move(Function function) {
        BasicBlock basicBlock = function.getFirstBlk();
        while (basicBlock != null) {
            if (!(basicBlock.getLastInstr().getPrev() instanceof PCInstr pc)) {
                basicBlock = (BasicBlock) basicBlock.getNext();
                continue;
            }
            List<Value> srcValList = pc.getSrcValList();
            List<Value> dstValList = pc.getDstValList();
            Set<Value> srcValSet = new HashSet<>(srcValList);
            
            for (int i = 0; i < srcValList.size(); i++) {
                Value src = srcValList.get(i);
                Value dst = dstValList.get(i);
                
                if (src == dst) {
                    srcValList.remove(i);
                    dstValList.remove(i);
                    i--;
                    continue;
                }
                
                if (!srcValSet.contains(dst)) {
                    MoveInstr moveInstr = new MoveInstr(src, dst, basicBlock);
                    moveInstr.insertBefore(basicBlock.getLastInstr());
                } else {    // phi 应该是并行的，这里要用串行模拟并行，避免冲突
                    // a->b, b->c => b->b' a->b b'->c
                    EmptyInstr tmp = new EmptyInstr(dst.getType(), basicBlock);
                    MoveInstr move1 = new MoveInstr(dst, tmp, basicBlock);
                    move1.insertBefore(basicBlock.getLastInstr());
                    srcValSet.remove(dst);
                    srcValSet.add(tmp);
                    
                    MoveInstr move2 = new MoveInstr(src, dst, basicBlock);
                    move2.insertBefore(basicBlock.getLastInstr());
                    for (int j = i; j < srcValList.size(); j++) {
                        if (srcValList.get(j) == dst) {
                            srcValList.set(j, tmp);
                        }
                    }
                }
                srcValList.remove(i);
                dstValList.remove(i);
                i--;
            }
            pc.removeFromList();
            
            basicBlock = (BasicBlock) basicBlock.getNext();
        }
    }
    
    private static void phi2pc(Function function) {
        BasicBlock basicBlock = function.getFirstBlk();
        while (basicBlock != null) {
            Instruction ins = basicBlock.getFirstInstr();
            while (ins instanceof PhiInstr phi) {
                Map<BasicBlock, Value> operandMap = phi.getOperandMap();
                for (BasicBlock srcBlk : operandMap.keySet()) {
                    if (srcBlk.getLastInstr() instanceof ReturnInstr) {
                        continue;
                    }
                    PCInstr pc = (PCInstr) srcBlk.getLastInstr().getPrev();
                    pc.putPair(operandMap.get(srcBlk), phi);
                }
                phi.forceRemoveFromList();
                ins = (Instruction) ins.getNext();
            }
            basicBlock = (BasicBlock) basicBlock.getNext();
        }
    }
    
    private static void prepareMidBlk(Function function) {
        BasicBlock curBasicBlock = function.getFirstBlk();
        while (curBasicBlock != null) {
            Instruction ins = curBasicBlock.getFirstInstr();
            while (ins instanceof PhiInstr phi) {
                Set<BasicBlock> srcBlkSet = new HashSet<>(phi.getOperandMap().keySet());
                for (BasicBlock srcBlk : srcBlkSet) {
                    Instruction lastIns = srcBlk.getLastInstr();
                    if (lastIns instanceof BranchInstr) {
                        // 如果来源基本块有分支，则应当在本块与来源之间插入一个新块，这样可以避免后续添加 move 污染其它分支
                        BasicBlock midBB = new BasicBlock(function.getAndAddBlkIdx());
                        midBB.insertBefore(curBasicBlock);  // 插在当前块之前，以便于后端直接把 midBB 的 J 合并掉
                        lastIns.modifyUse(curBasicBlock, midBB);
                        new PCInstr(midBB);
                        new JumpInstr(curBasicBlock, midBB);
                        
                        Instruction toChangeSrc = phi;
                        while (toChangeSrc instanceof PhiInstr) {
                            if (((PhiInstr) toChangeSrc).getOperandMap().containsKey(srcBlk)) {
                                toChangeSrc.modifyUse(srcBlk, midBB);
                            }
                            toChangeSrc = (Instruction) toChangeSrc.getNext();
                        }
                    } else if (lastIns instanceof JumpInstr) {
                        if (!(lastIns.getPrev() instanceof PCInstr)) {
                            PCInstr pc = new PCInstr(srcBlk);    // 此时这个 blk 已经 close 了，不接受新的指令，因此也不用特意删除
                            pc.insertBefore(lastIns);
                        }
                    }
                }
                
                ins = (Instruction) ins.getNext();
            }
            curBasicBlock = (BasicBlock) curBasicBlock.getNext();
        }
    }
}
