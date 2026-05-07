package backend.asm.register.phy;

import backend.asm.register.Reg;

public abstract class PhyReg extends Reg {
    private final String name;

    protected PhyReg(String name) {
        this.name = name;
    }

    @Override
    public String toString() {
        return this.name;
    }
}
