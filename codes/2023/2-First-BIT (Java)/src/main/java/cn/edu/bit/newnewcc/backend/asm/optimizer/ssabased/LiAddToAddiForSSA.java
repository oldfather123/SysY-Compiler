package cn.edu.bit.newnewcc.backend.asm.optimizer.ssabased;

import cn.edu.bit.newnewcc.backend.asm.instruction.AsmAdd;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmInstruction;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmLoad;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmSub;
import cn.edu.bit.newnewcc.backend.asm.operand.ExStackVarOffset;
import cn.edu.bit.newnewcc.backend.asm.operand.Immediate;
import cn.edu.bit.newnewcc.backend.asm.operand.IntRegister;
import cn.edu.bit.newnewcc.backend.asm.optimizer.SSABasedOptimizer;
import cn.edu.bit.newnewcc.backend.asm.util.ImmediateValues;

public class LiAddToAddiForSSA implements ISSABasedOptimizer {

    boolean isSourceLi(SSABasedOptimizer ssaBasedOptimizer, IntRegister reg) {
        var sourceInst = ssaBasedOptimizer.getValueSource(reg);
        if (sourceInst instanceof AsmLoad load && load.getOpcode().equals(AsmLoad.Opcode.LI)) {
            Immediate imm = (Immediate) load.getOperand(2);
            return !(imm instanceof ExStackVarOffset);
        }
        return false;
    }
    long getSourceValue(SSABasedOptimizer ssaBasedOptimizer, IntRegister reg) {
        var source = (Immediate)ssaBasedOptimizer.getValueSource(reg).getOperand(2);
        return source.getValue();
    }

    @Override
    public OptimizeResult getReplacement(SSABasedOptimizer ssaBasedOptimizer, AsmInstruction instruction) {
        OptimizeResult result = null;
        if (instruction instanceof AsmAdd asmAdd) {
            if (asmAdd.getOpcode().equals(AsmAdd.Opcode.ADD) || asmAdd.getOpcode().equals(AsmAdd.Opcode.ADDW)) {
                int bitLength = asmAdd.getOpcode().equals(AsmAdd.Opcode.ADD) ? 64 : 32;
                var rd = (IntRegister)asmAdd.getOperand(1);
                if (ssaBasedOptimizer.getValueSource(rd) != null) {
                    var rs1 = (IntRegister) asmAdd.getOperand(2);
                    var rs2 = (IntRegister) asmAdd.getOperand(3);
                    if (isSourceLi(ssaBasedOptimizer, rs1) && isSourceLi(ssaBasedOptimizer, rs2)) {
                        long value = getSourceValue(ssaBasedOptimizer, rs1) + getSourceValue(ssaBasedOptimizer, rs2);
                        if (!ImmediateValues.bitLengthNotInLimit(value)) {
                            var newRd = ssaBasedOptimizer.functionContext.getRegisterAllocator().allocateInt();
                            result = OptimizeResult.getNew();
                            result.addInstruction(new AsmLoad(newRd, new Immediate(Math.toIntExact(value))));
                            result.addRegisterMapping(rd, newRd);
                        }
                    } else if (isSourceLi(ssaBasedOptimizer, rs1) || isSourceLi(ssaBasedOptimizer, rs2)) {
                        var newRs1 = isSourceLi(ssaBasedOptimizer, rs1) ? rs2 : rs1;
                        long value = isSourceLi(ssaBasedOptimizer, rs1) ?
                                getSourceValue(ssaBasedOptimizer, rs1) : getSourceValue(ssaBasedOptimizer, rs2);
                        if (!ImmediateValues.bitLengthNotInLimit(value)) {
                            var newRd = ssaBasedOptimizer.functionContext.getRegisterAllocator().allocateInt();
                            result = OptimizeResult.getNew();
                            result.addInstruction(new AsmAdd(newRd, newRs1, new Immediate(Math.toIntExact(value)), bitLength));
                            result.addRegisterMapping(rd, newRd);
                        }
                    }
                }
            }
        }
        if (instruction instanceof AsmSub asmSub) {
            if (asmSub.getOpcode().equals(AsmSub.Opcode.SUB) || asmSub.getOpcode().equals(AsmSub.Opcode.SUBW)) {
                int bitLength = asmSub.getOpcode().equals(AsmSub.Opcode.SUB) ? 64 : 32;
                var rd = (IntRegister)asmSub.getOperand(1);
                if (ssaBasedOptimizer.getValueSource(rd) != null) {
                    var rs1 = (IntRegister) asmSub.getOperand(2);
                    var rs2 = (IntRegister) asmSub.getOperand(3);
                    if (isSourceLi(ssaBasedOptimizer, rs1) && isSourceLi(ssaBasedOptimizer, rs2)) {
                        long value = getSourceValue(ssaBasedOptimizer, rs1) + getSourceValue(ssaBasedOptimizer, rs2);
                        if (!ImmediateValues.bitLengthNotInLimit(value)) {
                            var newRd = ssaBasedOptimizer.functionContext.getRegisterAllocator().allocateInt();
                            result = OptimizeResult.getNew();
                            result.addInstruction(new AsmLoad(newRd, new Immediate(Math.toIntExact(value))));
                            result.addRegisterMapping(rd, newRd);
                        }
                    } else if (isSourceLi(ssaBasedOptimizer, rs1) || isSourceLi(ssaBasedOptimizer, rs2)) {
                        var newRs1 = isSourceLi(ssaBasedOptimizer, rs1) ? rs2 : rs1;
                        long value = isSourceLi(ssaBasedOptimizer, rs1) ?
                                getSourceValue(ssaBasedOptimizer, rs1) : getSourceValue(ssaBasedOptimizer, rs2);
                        if (!ImmediateValues.bitLengthNotInLimit(value)) {
                            var newRd = ssaBasedOptimizer.functionContext.getRegisterAllocator().allocateInt();
                            result = OptimizeResult.getNew();
                            result.addInstruction(new AsmAdd(newRd, newRs1, new Immediate(Math.toIntExact(value)), bitLength));
                            result.addRegisterMapping(rd, newRd);
                        }
                    }
                }
            }
        }
        return result;
    }
}
