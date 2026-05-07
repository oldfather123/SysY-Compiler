package backend.asm.register.vir;

import backend.asm.instr.ASMInstruction;
import backend.asm.register.Reg;
import backend.asm.register.phy.PhyReg;

import java.util.LinkedHashSet;

public abstract class VirReg extends Reg {
    private final LinkedHashSet<ASMInstruction> defInsSet;
    private PhyReg allocatedPhysicalRegister;

    protected VirReg() {
        this.defInsSet = new LinkedHashSet<>();
        allocatedPhysicalRegister = null;
    }

    public boolean stillVir() {
        return this.allocatedPhysicalRegister == null;
    }

    public void addDefIns(ASMInstruction def) {
        this.defInsSet.add(def);
    }

    public void deleteDefIns(ASMInstruction exDef) {
        this.defInsSet.remove(exDef);
    }

    public LinkedHashSet<ASMInstruction> getDefInsSet() {
        return defInsSet;
    }

    public void setPhyReg(PhyReg reg) {
        this.allocatedPhysicalRegister = reg;
    }

    public PhyReg getPhyReg() {
        return this.allocatedPhysicalRegister;
    }
    
    @Override
    public String toString() {
        return this.printAsOperand();
    }
}
