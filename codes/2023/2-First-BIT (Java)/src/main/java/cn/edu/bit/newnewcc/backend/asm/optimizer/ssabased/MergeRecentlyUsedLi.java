package cn.edu.bit.newnewcc.backend.asm.optimizer.ssabased;

import cn.edu.bit.newnewcc.backend.asm.instruction.AsmInstruction;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmLoad;
import cn.edu.bit.newnewcc.backend.asm.operand.Immediate;
import cn.edu.bit.newnewcc.backend.asm.operand.IntRegister;
import cn.edu.bit.newnewcc.backend.asm.optimizer.SSABasedOptimizer;

import java.util.HashMap;
import java.util.Map;

public class MergeRecentlyUsedLi implements ISSABasedOptimizer {
    private static final int TIME_LIMIT = 16;
    private final Map<Integer, IntRegister> immSavedMap = new HashMap<>();
    private final Map<Integer, Integer> immTimeStamp = new HashMap<>();
    private int instCounter;

    @Override
    public void setBlockBegins() {
        ISSABasedOptimizer.super.setBlockBegins();
        instCounter = 0;
        immSavedMap.clear();
        immTimeStamp.clear();
    }

    @Override
    public OptimizeResult getReplacement(SSABasedOptimizer ssaBasedOptimizer, AsmInstruction instruction) {
        OptimizeResult result = null;
        instCounter++;
        if (instruction instanceof AsmLoad asmLoad && asmLoad.getOpcode().equals(AsmLoad.Opcode.LI)) {
            IntRegister resultReg = (IntRegister) instruction.getOperand(1);
            if (resultReg.isVirtual() && ssaBasedOptimizer.getValueSource(resultReg) == instruction) {
                var imm = ((Immediate) asmLoad.getOperand(2)).getValue();
                if (immTimeStamp.containsKey(imm) && instCounter <= immTimeStamp.get(imm) + TIME_LIMIT) {
                    IntRegister previousReg = immSavedMap.get(imm);
                    if (!previousReg.equals(resultReg)) {
                        result = OptimizeResult.getNew();
                        result.addRegisterMapping(resultReg, previousReg);
                    }
                } else {
                    immSavedMap.put(imm, resultReg);
                }
                immTimeStamp.put(imm, instCounter);
            }
        }
        return result;
    }
}
