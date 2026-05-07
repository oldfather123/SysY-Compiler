package midend.DCE;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.terminator.ReturnInstr;
import frontend.ir.instr.terminator.Terminator;
import frontend.ir.objecttype.TVoid;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;

import java.util.List;

/**
 * 处理返回值无用的函数
 * 1. 消除ret语句
 * 2. 修改为void
 */
public class RemoveDeadRet {
    public static void execute(List<Function> functions) {
        for (Function function : functions) {
            executeOnFunc(function);
        }
    }

    public static void executeOnFunc(Function function) {
        if (function.getType() instanceof TVoid || function.getName().equals("main")) {
            return;
        }

        boolean isUseful = false;
        for (Instruction call : function.getCallInstrList()) {
            if (!call.getUserList().isEmpty()) {
                isUseful = true;
                break;
            }
        }
        if (isUseful) return;

        for (BasicBlock block : function.getBasicBlockList()) {
            Instruction instr = block.getLastInstr();
            if (instr instanceof ReturnInstr retur) {
                retur.forceRemoveFromList();
                // todo: check是否真的添加
                new ReturnInstr(null, block);
            }
        }
        function.setType(new TVoid());
        for (Instruction call : function.getCallInstrList()) {
            call.setType(new TVoid());
        }
    }
}
