package cn.edu.bit.newnewcc.backend.asm.optimizer.ssabased;

import cn.edu.bit.newnewcc.backend.asm.instruction.*;
import cn.edu.bit.newnewcc.backend.asm.operand.Immediate;
import cn.edu.bit.newnewcc.backend.asm.operand.IntRegister;
import cn.edu.bit.newnewcc.backend.asm.operand.Register;
import cn.edu.bit.newnewcc.backend.asm.optimizer.SSABasedOptimizer;
import cn.edu.bit.newnewcc.ir.value.constant.ConstLong;

/**
 * Shift Right Shift Left 优化 <br>
 */
/* from:
 * %1 = srl %src, #c
 * %ans = sll %src, #c
 *
 * to:
 * %ans = andi %src, #((1<<c)-1)
 */
public class SrslMergeOptimizer implements ISSABasedOptimizer {

    @Override
    public OptimizeResult getReplacement(SSABasedOptimizer helper, AsmInstruction instruction) {
        if (instruction instanceof AsmShiftLeft shiftLeft) {
            int bitWidth = switch (shiftLeft.getOpcode()) {
                case SLL, SLLW -> -1;
                case SLLI -> 64;
                case SLLIW -> 32;
            };
            if (bitWidth == -1) return null;
            var shiftLeftLength = ((Immediate) shiftLeft.getOperand(3)).getValue();
            var sourceInstruction = helper.getValueSource((Register) instruction.getOperand(2));
            if (sourceInstruction instanceof AsmShiftRightLogical shiftRightLogical) {
                switch (shiftRightLogical.getOpcode()) {
                    case SRLI -> {
                        if (bitWidth != 64) return null;
                    }
                    case SRLIW -> {
                        if (bitWidth != 32) return null;
                    }
                    case SRL, SRLW -> {
                        return null;
                    }
                }
                var shiftRightLength = ((Immediate) shiftRightLogical.getOperand(3)).getValue();
                if (shiftLeftLength == shiftRightLength) {
                    long mask = (1L << shiftLeftLength) - 1;
                    var result = OptimizeResult.getNew();
                    var finalReg = helper.functionContext.getRegisterAllocator().allocateInt();
                    var immMask = helper.functionContext.getValueToRegister(ConstLong.getInstance(mask));
                    result.addInstruction(new AsmAnd(finalReg, (IntRegister) shiftRightLogical.getOperand(2), immMask));
                    result.addRegisterMapping((Register) shiftLeft.getOperand(1), finalReg);
                    return result;
                }
            } else if (sourceInstruction instanceof AsmShiftRightArithmetic shiftRightArithmetic) {
                switch (shiftRightArithmetic.getOpcode()) {
                    case SRAI -> {
                        if (bitWidth != 64) return null;
                    }
                    case SRAIW -> {
                        if (bitWidth != 32) return null;
                    }
                    case SRA, SRAW -> {
                        return null;
                    }
                }
                var shiftRightLength = ((Immediate) shiftRightArithmetic.getOperand(3)).getValue();
                if (shiftLeftLength == shiftRightLength) {
                    long mask = -(1L << shiftLeftLength);
                    var result = OptimizeResult.getNew();
                    var finalReg = helper.functionContext.getRegisterAllocator().allocateInt();
                    var immMask = helper.functionContext.getValue(ConstLong.getInstance(mask));
                    result.addInstruction(new AsmAnd(finalReg, (IntRegister) shiftRightArithmetic.getOperand(2), immMask));
                    result.addRegisterMapping((Register) shiftLeft.getOperand(1), finalReg);
                    return result;
                }
            }
        }
        return null;
    }

}
