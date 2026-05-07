package cn.edu.bit.newnewcc.pass.ir;

import cn.edu.bit.newnewcc.ir.Module;
import cn.edu.bit.newnewcc.ir.Operand;
import cn.edu.bit.newnewcc.ir.type.ArrayType;
import cn.edu.bit.newnewcc.ir.value.Function;
import cn.edu.bit.newnewcc.ir.value.GlobalVariable;
import cn.edu.bit.newnewcc.ir.value.instruction.AllocateInst;
import cn.edu.bit.newnewcc.ir.value.instruction.StoreInst;

import java.util.ArrayList;

/**
 * Global variable to local variable pass <br>
 * 将全局的单个变量提升到局部，使其能放入寄存器中 <br>
 */
public class GvToLvPass {

    private static ArrayList<GlobalVariable> getPromotableGv(Module module, Function main) {
        var promotableGvs = new ArrayList<GlobalVariable>();
        for (GlobalVariable globalVariable : module.getGlobalVariables()) {
            if (globalVariable.getStoredValueType() instanceof ArrayType) continue;
            boolean ok = true;
            for (Operand usage : globalVariable.getUsages()) {
                if (usage.getInstruction().getBasicBlock().getFunction() != main) {
                    ok = false;
                    break;
                }
            }
            if (ok) {
                promotableGvs.add(globalVariable);
            }
        }
        return promotableGvs;
    }

    public static boolean runOnModule(Module module) {
        Function main = null;
        for (Function function : module.getFunctions()) {
            if (function.getValueName().equals("main")) {
                main = function;
                break;
            }
        }
        assert main != null;
        var promotableGvs = getPromotableGv(module, main);
        if (promotableGvs.size() == 0) return false;
        for (GlobalVariable promotableGv : promotableGvs) {
            var allocateInst = new AllocateInst(promotableGv.getStoredValueType());
            main.getEntryBasicBlock().addInstruction(allocateInst);
            main.getEntryBasicBlock().addMainInstructionAtBeginning(new StoreInst(allocateInst, promotableGv.getInitialValue()));
            promotableGv.replaceAllUsageTo(allocateInst);
            module.removeGlobalVariable(promotableGv);
        }
        // 跑 mem2reg ，将单变量放到寄存器上
        MemoryToRegisterPass.runOnModule(module);
        return true;
    }

}
