package cn.edu.bit.newnewcc.backend.asm.optimizer.ssabased;

import cn.edu.bit.newnewcc.backend.asm.instruction.AsmInstruction;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmMul;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmShiftLeft;
import cn.edu.bit.newnewcc.backend.asm.operand.Immediate;
import cn.edu.bit.newnewcc.backend.asm.operand.IntRegister;
import cn.edu.bit.newnewcc.backend.asm.optimizer.SSABasedOptimizer;
import cn.edu.bit.newnewcc.backend.asm.util.Utility;

public class StrengthReductionOptimizer implements ISSABasedOptimizer {

    @Override
    public OptimizeResult getReplacement(SSABasedOptimizer helper, AsmInstruction instruction) {
        if (instruction instanceof AsmMul mulInstr) {
            var opcode = mulInstr.getOpcode();
            if (opcode == AsmMul.Opcode.MUL || opcode == AsmMul.Opcode.MULW) {
                IntRegister dest = (IntRegister) mulInstr.getOperand(1);
                IntRegister source1 = (IntRegister) mulInstr.getOperand(2);
                IntRegister source2 = (IntRegister) mulInstr.getOperand(3);
                int bitLength = opcode == AsmMul.Opcode.MUL ? 64 : 32;

                if (helper.isConstIntOperand(source1)) {
                    int value = helper.getConstIntValueFromOperand(source1);
                    if (Utility.isPowerOf2(value)) {
                        OptimizeResult result = OptimizeResult.getNew();
                        var newFinalReg = helper.functionContext.getRegisterAllocator().allocateInt();
                        result.addInstruction(new AsmShiftLeft(newFinalReg, source2, new Immediate(Utility.log2(value)), bitLength));
                        result.addRegisterMapping(dest, newFinalReg);
                        return result;
                    }
                } else if (helper.isConstIntOperand(source2)) {
                    int value = helper.getConstIntValueFromOperand(source2);
                    if (Utility.isPowerOf2(value)) {
                        OptimizeResult result = OptimizeResult.getNew();
                        var newFinalReg = helper.functionContext.getRegisterAllocator().allocateInt();
                        result.addInstruction(new AsmShiftLeft(newFinalReg, source1, new Immediate(Utility.log2(value)), bitLength));
                        result.addRegisterMapping(dest, newFinalReg);
                        return result;
                    }
                }
            }
        }
        return null;
    }
}
