package cn.edu.bit.newnewcc.backend.asm.optimizer.ssabased;

import cn.edu.bit.newnewcc.backend.asm.instruction.AsmInstruction;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmLoad;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmMove;
import cn.edu.bit.newnewcc.backend.asm.operand.Immediate;
import cn.edu.bit.newnewcc.backend.asm.operand.IntRegister;
import cn.edu.bit.newnewcc.backend.asm.operand.Register;
import cn.edu.bit.newnewcc.backend.asm.optimizer.SSABasedOptimizer;

public class MvX0ToLI0Optimizer implements ISSABasedOptimizer {

    @Override
    public OptimizeResult getReplacement(SSABasedOptimizer helper, AsmInstruction instruction) {
        if (instruction instanceof AsmMove asmMove) {
            if (asmMove.getOperand(2).equals(IntRegister.ZERO)) {
                var result = OptimizeResult.getNew();
                var newFinalReg = helper.functionContext.getRegisterAllocator().allocateInt();
                var newInst = new AsmLoad(newFinalReg, new Immediate(0));
                result.addInstruction(newInst);
                result.addRegisterMapping((Register) asmMove.getOperand(1), newFinalReg);
                return result;
            }
        }
        return null;
    }

}
