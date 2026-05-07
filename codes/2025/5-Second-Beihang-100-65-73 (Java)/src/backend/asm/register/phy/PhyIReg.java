package backend.asm.register.phy;

import backend.asm.register.IReg;

public class PhyIReg extends PhyReg implements IReg {
    private final String name4double;

    public PhyIReg(String name, String name4double) {
        super(name);
        this.name4double = name4double;
    }

    @Override
    public String printAsOperand() {
        return this.toString();
    }

    public String printAsDouble() {
        return this.name4double;
    }
}
