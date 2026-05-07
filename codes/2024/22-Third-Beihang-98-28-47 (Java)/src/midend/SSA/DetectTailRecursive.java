package midend.SSA;

import Utils.CustomList;
import frontend.ir.instr.otherop.CallInstr;
import frontend.ir.instr.terminator.JumpInstr;
import frontend.ir.structure.Function;

import java.util.ArrayList;

public class DetectTailRecursive {
    public static void detect(ArrayList<Function> functions) {
        for (Function f : functions) {
            if (f.getSelfCallingInstrSet() == null || f.getSelfCallingInstrSet().isEmpty()) {
                continue;
            }
            boolean flag = true;
            for (CallInstr callInstr : f.getSelfCallingInstrSet()) {
                if (!(callInstr.getNext() instanceof JumpInstr jumpInstr)) {
                    flag = false;
                    break;
                }
                if (!(jumpInstr.getTarget().equals(f.getBasicBlocks().getTail()))) {
                    flag = false;
                    break;
                }
            }
            if (flag) {
                f.setTailRecursive(true);
            }
        }
    }
}
