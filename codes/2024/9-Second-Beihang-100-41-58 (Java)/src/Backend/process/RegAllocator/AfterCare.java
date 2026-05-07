package Backend.process.RegAllocator;

import Backend.component.AsmFunction;
import Backend.component.AsmInstr;
import Backend.component.AsmModule;
import Backend.instruction.AsmBinary;
import Backend.instruction.AsmMove;
import Backend.operand.*;

import java.util.Arrays;
import java.util.HashMap;
import java.util.LinkedList;

import static Backend.operand.AllPhysicalReg.*;

public class AfterCare {
    private final AsmModule asmModule;
    private final HashMap<AsmFunction, Integer> funcStack;

    public AfterCare(AsmModule asmModule, HashMap<AsmFunction, Integer> funcStack) {
        this.asmModule = asmModule;
        this.funcStack = funcStack;
    }

    public void run() {
        resetStackSize();
        for (AsmFunction objFunction : asmModule.getFunctions()) {
            objFunction.instAlignStack(funcStack.get(objFunction));
        }
    }

    private void resetStackSize() {
        for (AsmFunction objFunction : asmModule.getFunctions()) {
            AsmInstr first_inst = objFunction.getFirstBlock().getInstrList().getFirst();
            helpResetStack(objFunction, first_inst, true);
            AsmInstr last2inst = getPreInstr(objFunction.getExitBlock().getInstrList().getLast(), objFunction.getExitBlock().getInstrList());
            helpResetStack(objFunction, last2inst, false);
        }
    }

    private void helpResetStack(AsmFunction objFunction, AsmInstr instr, boolean isFirst) {
        int immediate;
        if (isFirst) {
            immediate = -objFunction.getStackSize();
        } else {
            immediate = objFunction.getStackSize();
        }
        if (instr instanceof AsmBinary) {
            if (2047 >= immediate && -2048 <= immediate) {
                ((AsmBinary) instr).setSrcRight(new AsmImm(immediate, true));
            } else {
                AsmMove move = new AsmMove(Arrays.asList(phyAllRegs.get(5), new AsmImm(immediate, false)));
                objFunction.addInstBefore(instr, move);
                ((AsmBinary) instr).setSrcRight(phyAllRegs.get(5));
            }
        }
    }

    private AsmInstr getPreInstr(AsmInstr instr, LinkedList<AsmInstr> list) {
        int index = list.indexOf(instr);
        if (index == 0) {
            return null;
        } else {
            return list.get(index - 1);
        }
    }
}
