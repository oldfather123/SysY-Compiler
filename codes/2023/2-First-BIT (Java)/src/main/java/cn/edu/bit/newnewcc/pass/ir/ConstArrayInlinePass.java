package cn.edu.bit.newnewcc.pass.ir;

import cn.edu.bit.newnewcc.ir.Module;
import cn.edu.bit.newnewcc.ir.Operand;
import cn.edu.bit.newnewcc.ir.exception.CompilationProcessCheckFailedException;
import cn.edu.bit.newnewcc.ir.value.Constant;
import cn.edu.bit.newnewcc.ir.value.GlobalVariable;
import cn.edu.bit.newnewcc.ir.value.Instruction;
import cn.edu.bit.newnewcc.ir.value.constant.ConstArray;
import cn.edu.bit.newnewcc.ir.value.constant.ConstInt;
import cn.edu.bit.newnewcc.ir.value.instruction.*;

import java.util.LinkedList;
import java.util.Queue;

/**
 * 将全局常量数组，常下标的load，内联为值
 */
public class ConstArrayInlinePass {

    private static boolean canGlobalVariableBeConstant(GlobalVariable globalVariable) {
        Queue<Instruction> useList = new LinkedList<>();
        for (Operand usage : globalVariable.getUsages()) {
            useList.add(usage.getInstruction());
        }
        while (!useList.isEmpty()) {
            var instruction = useList.remove();
            if (instruction instanceof StoreInst || instruction instanceof CallInst) {
                return false;
            } else if (instruction instanceof LoadInst) {
                // do nothing
            } else if (instruction instanceof GetElementPtrInst || instruction instanceof BitCastInst) {
                for (Operand usage : instruction.getUsages()) {
                    useList.add(usage.getInstruction());
                }
            } else {
                // 需要手动补充处理规则
                throw new CompilationProcessCheckFailedException("Process rule not configured: " + instruction.getClass());
            }
        }
        return true;
    }

    private static boolean replaceLoadToConstValue(Instruction instruction, Constant constant) {
        if (instruction instanceof GetElementPtrInst getElementPtrInst) {
            if (!(getElementPtrInst.getIndexAt(0) instanceof ConstInt constInt && constInt.getValue() == 0)) {
                return false;
            }
            for (int i = 1; i < getElementPtrInst.getIndicesSize(); i++) {
                if (!(getElementPtrInst.getIndexAt(i) instanceof ConstInt constInt1)) {
                    return false;
                }
                constant = ((ConstArray) constant).getValueAt(constInt1.getValue());
            }
            boolean changed = false;
            for (Operand usage : getElementPtrInst.getUsages()) {
                changed |= replaceLoadToConstValue(usage.getInstruction(), constant);
            }
            return changed;
        } else if (instruction instanceof LoadInst loadInst) {
            loadInst.replaceAllUsageTo(constant);
            return true;
        }
        return false;
    }

    public static boolean runOnModule(Module module) {
        boolean changed = false;
        for (GlobalVariable globalVariable : module.getGlobalVariables()) {
            if (!globalVariable.isConstant() && canGlobalVariableBeConstant(globalVariable)) {
                globalVariable.setIsConstant(true);
            }
            if (globalVariable.isConstant()) {
                for (Operand usage : globalVariable.getUsages()) {
                    boolean result = replaceLoadToConstValue(usage.getInstruction(), globalVariable.getInitialValue());
                    changed |= result;
                }
            }
        }
        return changed;
    }
}
