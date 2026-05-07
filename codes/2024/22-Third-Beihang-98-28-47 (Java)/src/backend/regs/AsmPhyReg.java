package backend.regs;

import backend.itemStructure.AsmOperand;

public class AsmPhyReg extends AsmReg {
    private int index;
    private String name;
    public AsmPhyReg(String name) {
        this.index = RegGeter.nameToIndexInt.get(name);
        this.name = name;
    }
    public String toString() {
        return name;
    }
}
