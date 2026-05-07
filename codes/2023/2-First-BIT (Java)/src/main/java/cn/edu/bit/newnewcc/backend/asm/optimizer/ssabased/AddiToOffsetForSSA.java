package cn.edu.bit.newnewcc.backend.asm.optimizer.ssabased;

import cn.edu.bit.newnewcc.backend.asm.instruction.AsmAdd;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmInstruction;
import cn.edu.bit.newnewcc.backend.asm.operand.ExStackVarOffset;
import cn.edu.bit.newnewcc.backend.asm.operand.Immediate;
import cn.edu.bit.newnewcc.backend.asm.operand.IntRegister;
import cn.edu.bit.newnewcc.backend.asm.operand.MemoryAddress;
import cn.edu.bit.newnewcc.backend.asm.optimizer.SSABasedOptimizer;
import cn.edu.bit.newnewcc.backend.asm.util.AsmInstructions;
import cn.edu.bit.newnewcc.backend.asm.util.ImmediateValues;
import cn.edu.bit.newnewcc.backend.asm.util.Registers;

public class AddiToOffsetForSSA implements ISSABasedOptimizer {
    @Override
    public OptimizeResult getReplacement(SSABasedOptimizer helper, AsmInstruction instruction) {
        OptimizeResult result = null;
        var op = instruction.getOperand(2);
        if (op instanceof MemoryAddress memoryAddress) {
            var reg = memoryAddress.getRegister();
            var sourceInst = helper.getValueSource(reg);
            if (sourceInst instanceof AsmAdd asmAdd && asmAdd.getOpcode().equals(AsmAdd.Opcode.ADDI)) {
                var imm = (Immediate)asmAdd.getOperand(3);
                var rs = (IntRegister)asmAdd.getOperand(2);
                if (!(imm instanceof ExStackVarOffset) && (helper.getValueSource(rs) != null || Registers.CONSTANT_REGISTERS.contains(rs))) {
                    long val = imm.getValue() + memoryAddress.getOffset();
                    if (!ImmediateValues.bitLengthNotInLimit(val)) {
                        result = OptimizeResult.getNew();
                        instruction.setOperand(2, new MemoryAddress(val, rs));
                    }
                }
            }
        }
        return result;
    }
}
