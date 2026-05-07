package cn.edu.bit.newnewcc.backend.asm.optimizer;

import cn.edu.bit.newnewcc.backend.asm.AsmFunction;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmAdd;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmInstruction;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmLoad;
import cn.edu.bit.newnewcc.backend.asm.operand.Immediate;
import cn.edu.bit.newnewcc.backend.asm.operand.IntRegister;
import cn.edu.bit.newnewcc.backend.asm.util.ImmediateValues;

import java.util.ArrayList;
import java.util.List;

public class LIAddToAddIOptimizer implements Optimizer {
    @Override
    public boolean runOn(AsmFunction function) {
        List<AsmInstruction> instrList = function.getInstrList();

        List<AsmInstruction> newInstrList = new ArrayList<>();

        int count = 0;

        for (int i = 0; i < instrList.size(); ++i) {
            boolean replaced = false;

            if (i + 1 < instrList.size()) {
                AsmInstruction curInstr = instrList.get(i);
                AsmInstruction nextInstr = instrList.get(i + 1);

                if (curInstr instanceof AsmLoad
                    && ((AsmLoad) curInstr).getOpcode() == AsmLoad.Opcode.LI
                    && !ImmediateValues.bitLengthNotInLimit(((Immediate) curInstr.getOperand(2)).getValue())
                    && nextInstr instanceof AsmAdd
                    && ((AsmAdd) nextInstr).getOpcode() == AsmAdd.Opcode.ADD
                    && (
                        nextInstr.getOperand(2).equals(curInstr.getOperand(1))
                        || nextInstr.getOperand(3).equals(curInstr.getOperand(1))
                    )
                ) {
                    AsmAdd newInstr;
                    if (nextInstr.getOperand(2).equals(curInstr.getOperand(1))) {
                        newInstr = new AsmAdd(
                            (IntRegister) nextInstr.getOperand(1),
                            (IntRegister) nextInstr.getOperand(3),
                            curInstr.getOperand(2),
                            64
                        );
                    } else {
                        newInstr = new AsmAdd(
                            (IntRegister) nextInstr.getOperand(1),
                            (IntRegister) nextInstr.getOperand(2),
                            curInstr.getOperand(2),
                            64
                        );
                    }
                    newInstrList.add(curInstr);
                    newInstrList.add(newInstr);
                    replaced = true;
                    ++count;
                    ++i;
                }
            }

            if (!replaced) {
                newInstrList.add(instrList.get(i));
            }
        }

        instrList.clear();
        instrList.addAll(newInstrList);
        return count > 0;
    }
}
