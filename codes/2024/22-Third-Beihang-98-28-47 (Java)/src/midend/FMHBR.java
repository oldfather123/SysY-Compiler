package midend;

import frontend.ir.Value;
import frontend.ir.constvalue.ConstValue;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.memop.StoreInstr;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.instr.terminator.ReturnInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.FParam;
import frontend.ir.structure.Function;

import java.util.ArrayList;

/**
 * Function Memorize Hash Block Rearrangement
 * 对于做过记忆化的递归函数，将哈希计算块尽可能移动到递归条件判断之后，需要计算之前
 */
public class FMHBR {
    public static void execute(ArrayList<Function> functions) {
        for (Function function : functions) {
            if (!function.isMemorized()) {
                continue;
            }
            
            BasicBlock hashBlk = (BasicBlock) function.getBasicBlocks().getHead();
            BasicBlock hashRetBlk  = (BasicBlock) hashBlk.getNext();
            if (!hashBlk.getTag().equals("hashBlk") || !hashRetBlk.getTag().equals("hashRetBlk")) {
                throw new RuntimeException("未曾设想的结构，经过记忆化的函数头两个块现在应该一定是用作哈希的");
            }
            
            hashBlk.removeFromListWithInstrRemain();
            hashRetBlk.removeFromListWithInstrRemain();
            
            BasicBlock retBlk = (BasicBlock) function.getBasicBlocks().getTail();
            Instruction retIns = retBlk.getEndInstr();
            if (!(retIns instanceof ReturnInstr)) {
                throw new RuntimeException("最后一个块的最后一条指令必须是返回");
            }
            Value retVal = ((ReturnInstr) retIns).getReturnValue();
            if (!(retVal instanceof PhiInstr)) {
                throw new RuntimeException("做了记忆化的函数这里至少两种返回情况");
            }
            ArrayList<Value> values = ((PhiInstr) retVal).getValues();
            ArrayList<BasicBlock> prtBlks = ((PhiInstr) retVal).getPrtBlks();
            
            BasicBlock beginBlk = (BasicBlock) function.getBasicBlocks().getHead();
            BasicBlock recursiveHeader = null;  // 递归条件满足，开始计算的块
            for (int i = 0; i < values.size(); i++) {
                Value value = values.get(i);
                BasicBlock prtBlk = prtBlks.get(i);
                if (value instanceof ConstValue || value instanceof FParam) {
                    if (prtBlk.getPres().contains(beginBlk)) {
                        // 判断要求返回常数或者参数，并且直接前驱包括现在的开始块，从而保证是最开始的递归判断条件
                        for (BasicBlock suc : beginBlk.getSucs()) {
                            if (!suc.equals(prtBlk)) {
                                recursiveHeader = suc;
                                beginBlk.getEndInstr().modifyUse(recursiveHeader, hashBlk);
                                hashBlk.getEndInstr().modifyUse(beginBlk, recursiveHeader);
                                break;
                            }
                        }
                        
                        Instruction ins = (Instruction) prtBlk.getInstructions().getHead();
                        while (ins != null) {
                            if (ins instanceof StoreInstr) {
                                String symName = ((StoreInstr) ins).getSymbol().getName();
                                if (symName.equals("global_data_" + function.getName())) {
                                    ins.removeFromList();
                                } else if (symName.equals("global_used_" + function.getName())) {
                                    ins.removeFromList();
                                }
                            }
                            ins = (Instruction) ins.getNext();
                        }
                    }
                }
            }
            if (recursiveHeader == null) {
                // 直接放回去
                function.getBasicBlocks().addToHead(hashRetBlk);
                function.getBasicBlocks().addToHead(hashBlk);
            } else {
                hashBlk.insertBefore(recursiveHeader);
                hashRetBlk.insertBefore(recursiveHeader);
            }
        }
    }
}
