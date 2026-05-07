package backend.regs;

import backend.itemStructure.AsmOperand;

public class AsmFPhyReg extends AsmReg {
    private final int index;
    private final String name;
    public AsmFPhyReg(String name) {
        this.name = name;
        this.index = RegGeter.nameToIndexFloat.get(name);
    }
    public String toString() {
        return name;
    }
}
