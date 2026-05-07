package backend.asm.instr.arm.neon.memory;

import backend.asm.ASMValue;
import backend.asm.instr.arm.ARMIns;
import backend.asm.register.Reg;
import backend.asm.register.phy.PhyFReg;
import backend.asm.register.store.RegStore;
import backend.asm.register.vir.VirFReg;
import backend.asm.structure.ASMBasicBlock;

import java.util.Arrays;
import java.util.List;

public class ARMVecStore extends ARMIns {
    private Reg base;
    private VirFReg value;   // value to store

    public ARMVecStore(Reg base, VirFReg value, ASMBasicBlock parentBB) {
        super(parentBB, RegStore.NA);
        this.base = base;
        this.value = value;

        addUsedVal(base);
        addUsedVal(value);
    }

    @Override
    public List<ASMValue> getOperands() {
        return Arrays.asList(base, value);
    }

    @Override
    public void resetOperands(List<ASMValue> values) {
        if (values == null || values.size() != 2) {
            throw new RuntimeException("ST1 指令接受且只接受两个操作数");
        }

        this.modifyUse(base, values.get(0));
        this.modifyUse(value, values.get(1));
        this.base = (Reg) values.get(0);
        this.value = (VirFReg) values.get(1);
    }

    @Override
    protected String printIns() {
        if (!(value.getPhyReg() instanceof PhyFReg fReg0)) {
            throw new IllegalArgumentException("The PhyReg of operands must be of type PhyFReg and PhyFReg");
        }

        return "ST1 { " + fReg0.printAsFullVec() + " }, [" + base.printAsOperand() + "]";
    }
}
