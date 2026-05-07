package cn.edu.bit.newnewcc.backend.asm.optimizer;

import cn.edu.bit.newnewcc.backend.asm.AsmFunction;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmAdd;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmBlockEnd;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmInstruction;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmLabel;
import cn.edu.bit.newnewcc.backend.asm.operand.*;
import cn.edu.bit.newnewcc.backend.asm.util.ImmediateValues;

import java.util.HashMap;
import java.util.Map;

public class AddiToOffset implements Optimizer {
    @Override
    public boolean runOn(AsmFunction function) {
        Map<Register, AsmInstruction> sourceInst = new HashMap<>();
        Map<AsmInstruction, Map<AsmOperand, AsmInstruction>> opSource = new HashMap<>();
        boolean replaced = false;
        for (var inst : function.getInstrList()) {
            if (inst instanceof AsmLabel || inst instanceof AsmBlockEnd) {
                sourceInst.clear();
                opSource.clear();
                continue;
            }
            opSource.put(inst, new HashMap<>());
            for (int i = 1; i <= 3; i++) {
                var op = inst.getOperand(i);
                if (op instanceof Register reg) {
                    opSource.get(inst).put(op, sourceInst.get(reg));
                }
            }
            var op = inst.getOperand(2);
            if (op instanceof MemoryAddress memoryAddress) {
                var base = memoryAddress.getRegister();
                if (sourceInst.get(base) instanceof AsmAdd asmAdd && asmAdd.getOpcode().equals(AsmAdd.Opcode.ADDI)) {
                    var value = memoryAddress.getOffset() + ((Immediate)asmAdd.getOperand(3)).getValue();
                    var oldBase = (IntRegister) asmAdd.getOperand(2);
                    if (!ImmediateValues.bitLengthNotInLimit(value) && opSource.get(asmAdd).get(oldBase) == sourceInst.get(oldBase)) {
                        inst.setOperand(2, new MemoryAddress(value, oldBase));
                        replaced = true;
                    }
                }
            } else if (op instanceof ExStackVarContent stackVar) {
                var address = stackVar.getAddress();
                var base = address.getRegister();
                if (sourceInst.get(base) instanceof AsmAdd asmAdd && asmAdd.getOpcode().equals(AsmAdd.Opcode.ADDI)) {
                    var value = address.getOffset() + ((Immediate)asmAdd.getOperand(3)).getValue();
                    var oldBase = (IntRegister) asmAdd.getOperand(2);
                    if (!ImmediateValues.bitLengthNotInLimit(value) && opSource.get(asmAdd).get(oldBase) == sourceInst.get(oldBase)) {
                        inst.setOperand(2, ExStackVarContent.transform(stackVar, new MemoryAddress(value, oldBase)));
                        replaced = true;
                    }
                }
            }
            for (var reg : inst.getDef()) {
                sourceInst.put(reg, inst);
            }
        }
        return replaced;
    }
}