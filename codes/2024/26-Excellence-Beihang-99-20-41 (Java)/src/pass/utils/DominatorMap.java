package pass.utils;

import ir.IrInstr;
import ir.instr.TermInstr;
import ir.value.BasicBlock;
import ir.value.Function;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;

// 支配图，但图是反向的，即如果a块能跳转到b块，则存在边b->a
public class DominatorMap {
    public static HashMap<String, ArrayList<String>> getDominatorTree(
            Function function,HashMap<String,Integer> inNum,HashMap<String,Integer> outNum) {
        HashMap<String, ArrayList<String>> dominatorTree = new HashMap<>();
        ArrayList<String> blockName = function.getBasicBlocksName();
        List<BasicBlock> basicBlocks = function.getBasicBlocks();
        for (BasicBlock block : basicBlocks) {
            LinkedList<IrInstr> instrs = block.getInstrs();
            for (var instr : instrs) {
                tackleTermInstr(inNum, outNum, block, instr, dominatorTree);
            }
        }
        return dominatorTree;
    }

    private static void tackleTermInstr(HashMap<String, Integer> inNum, HashMap<String, Integer> outNum, BasicBlock block, IrInstr instr, HashMap<String, ArrayList<String>> dominatorTree) {
        if (instr instanceof TermInstr term) {
            for (var target : term.getJumpTargets()) {
                BasicBlock dest = (BasicBlock) target;
                if (!dominatorTree.containsKey(dest.getName())) {
                    dominatorTree.put(dest.getName(), new ArrayList<>());
                }
                dominatorTree.get(dest.getName()).add(block.getName());
                inNum.put(block.getName(), inNum.getOrDefault(block.getName(), 0) + 1);
                outNum.put(dest.getName(), outNum.getOrDefault(dest.getName(), 0) + 1);
            }
        }
    }
}
