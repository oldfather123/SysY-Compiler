package backend.asm.instr.arm.cmp;

import backend.asm.ASMValue;
import backend.asm.instr.arm.ARMIns;
import backend.asm.register.FReg;
import backend.asm.register.IReg;
import backend.asm.register.Reg;
import backend.asm.register.store.RegStore;
import backend.asm.structure.ASMBasicBlock;

import java.util.Arrays;
import java.util.List;

public class ARMCmp extends ARMIns {
    private Reg operand1;
    private ASMValue operand2;

    public ARMCmp(Reg operand1, ASMValue operand2, ASMBasicBlock parentBlock) {
        super(parentBlock, RegStore.NA);
        this.operand1 = operand1;
        this.operand2 = operand2;

        this.addUsedVal(operand1);
        this.addUsedVal(operand2);
    }

    @Override
    public List<ASMValue> getOperands() {
        return Arrays.asList(operand1, operand2);
    }

    @Override
    public void resetOperands(List<ASMValue> values) {
        if (values == null || values.size() != 2) {
            throw new RuntimeException("CMP 指令接受且只接受两个操作数");
        }

        this.modifyUse(operand1, values.get(0));
        this.modifyUse(operand2, values.get(1));
        this.operand1 = (Reg) values.get(0);
        this.operand2 = values.get(1);
    }

    @Override
    protected String printIns() {
        if (operand1 instanceof IReg) {
            return "CMP " + operand1.printAsOperand() + ", " + operand2.printAsOperand();
        } else if (operand1 instanceof FReg) {
            return "FCMP " + operand1.printAsOperand() + ", " + operand2.printAsOperand();
        } else {
            throw new RuntimeException("ASMlt 指令的操作数必须是 IReg 或 FReg");
        }
    }
}
