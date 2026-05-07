package backend.component;

import backend.instruction.RiscvBinary;
import backend.operand.RiscvOperand;
import backend.operand.RiscvReg;
import tools.DoubleNode;

import java.util.ArrayList;

public class RiscvInstr {
    protected ArrayList<RiscvOperand> operands;
    protected RiscvReg defReg;

    public RiscvInstr(RiscvReg defReg, ArrayList<RiscvOperand> operands) {
        this.defReg = defReg;
        this.operands = operands;
    }

    public void replaceDefReg(RiscvReg reg, DoubleNode<RiscvInstr> node) {
        defReg = reg;
        reg.getUser().remove(node);
        defReg.getUser().add(node);
        defReg.addDefOrUse(node.getParentList().loopdepth, node.getParentList().loop);
    }

    public void replaceOperands(RiscvReg reg1, RiscvReg reg2, DoubleNode<RiscvInstr> node) {
        int index = 0;
        for(int i = 0;i < operands.size();i++) {
            if(operands.get(i).equals(reg1)) {
                operands.get(i).getUser().remove(node);
                reg2.getUser().add(node);
                operands.set(i, reg2);
                reg2.addDefOrUse(node.getParentList().loopdepth, node.getParentList().loop);
            }
        }
    }

    public RiscvReg getDefReg() {
        return defReg;
    }

    public ArrayList<RiscvOperand> getOperands() {
        return operands;
    }
}
