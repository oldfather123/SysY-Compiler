package midend.DCE;

import frontend.ir.instr.otherop.CallInstr;
import frontend.ir.objecttype.TVoid;
import frontend.ir.structure.Function;
import midend.FunctionInfoCollector;
import midend.PureFunctionAnalysis;

import java.util.ArrayList;
import java.util.List;

/**
 * 必须在函数内联前完成
 */
public class RemoveDeadFunction {
    public static void execute(List<Function> functions) {
        // 确保dead return被删除
        RemoveDeadRet.execute(functions);
        // 确保纯函数分析完成
        PureFunctionAnalysis.execute(functions);

        List<Function> toRemove = new ArrayList<>();
        for (Function function : functions) {
            if (isDead(function)) {
                toRemove.add(function);
            }
        }

        // 删除函数
        for (Function function : toRemove) {
            function.removeFromList();
            functions.remove(function);
            // 删除调用它的语句
            for(CallInstr callInstr: new ArrayList<>(function.getCallInstrList())) {
                callInstr.forceRemoveFromList();
            }
        }
    }

    public static boolean isDead(Function function) {
        if (function.getType() instanceof TVoid && FunctionInfoCollector.getFuncInfo(function).isDead()) {
            return true;
        } else {
            return false;
        }
    }
}
