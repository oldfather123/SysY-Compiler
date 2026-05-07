package midpass;

import midend.value.Function;
import midend.value.Value;
import midend.value.instrs.BinaryOp;
import midend.value.instrs.Instr;

import java.util.ArrayList;
import static midend.value.Value.flag;
public class ConstantFold {
    ArrayList<Function> functions;

    public ConstantFold(ArrayList<Function> functions) {
        this.functions = functions;
        flag = true;
        while (flag) {
            flag = false;
            for (Function function : functions) {
                for (var blockNode : function.getBasicBlocks()) {
                    for (var instrNode : blockNode.get().getInstrList()) {
                        Instr instr = instrNode.get();
                        if (instr instanceof BinaryOp) {
                            Value res = ((BinaryOp) instr).evaluate();
                            if (res != null) {
                                instr.changeAllUse2UseOther(res);
                                instr.remove();
                            }
                        }
                    }
                }
            }
        }

    }
}
