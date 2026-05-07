package cn.edu.bit.newnewcc.backend.asm.optimizer.ssabased;

import cn.edu.bit.newnewcc.backend.asm.instruction.AsmInstruction;
import cn.edu.bit.newnewcc.backend.asm.operand.IntRegister;
import cn.edu.bit.newnewcc.backend.asm.operand.Register;
import cn.edu.bit.newnewcc.backend.asm.optimizer.SSABasedOptimizer;
import cn.edu.bit.newnewcc.backend.asm.util.AsmInstructions;

public class LI0ToX0Optimizer implements ISSABasedOptimizer {

    @Override
    public OptimizeResult getReplacement(SSABasedOptimizer helper, AsmInstruction instruction) {
        boolean hasResult = false;
        for (int id : AsmInstructions.getReadRegId(instruction)) {
            var operand = instruction.getOperand(id);
            if (!(operand instanceof Register)) continue;
            if (helper.isConstIntOperand(operand) && helper.getConstIntValueFromOperand(operand) == 0 && !IntRegister.ZERO.equals(operand)) {
                instruction.setOperand(id, IntRegister.ZERO);
                hasResult = true;
            }
        }
        return hasResult ? OptimizeResult.getNew() : null;
    }
}
