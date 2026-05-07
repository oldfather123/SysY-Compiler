package midend;
import frontend.ir.instr.Instruction;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import midend.DCE.RemoveDeadCode;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
/**
 * 常量折叠
 * 前提：无 副作用：破坏CFG，破坏循环信息
 * 跑完后自动跑一次死代码删除，合并时可能有垃圾产生
 */
public class ConstFold {

    public static int depthCounter = 0;
    public static int MAX_DEPTH = 100;

    public static void execute(List<Function> functions) {
        for (Function function : functions) {
            depthCounter = 0;
            BasicBlock headBlk = function.getFirstBlk();
            dfsIdoms(headBlk, new HashMap<>());
            RemoveDeadCode.execute4func(function);
        }
    }

    private static void dfsIdoms(BasicBlock basicBlock, Map<String, Instruction> instrMap) {
        Instruction ins = basicBlock.getFirstInstr();
        while (ins != null) {
            ins.simplify();
            ins = (Instruction) ins.getNext();
        }
        for (BasicBlock next : basicBlock.getIDomSet()) {
            dfsIdoms(next, new HashMap<>(instrMap));
        }
    }
}
