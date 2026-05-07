package backend.asm.register.phy;

import backend.asm.register.FReg;

public class PhyFReg extends PhyReg implements FReg {
    private final String name4vector;
    private final String name4whole;

    public PhyFReg(String name, String name4vector, String name4whole) {
        super(name);
        this.name4vector = name4vector;
        this.name4whole = name4whole;
    }

    @Override
    public String printAsOperand() {
        return this.toString();
    }

    public String printAsWholeScalar() {
        return this.name4whole;
    }

    public String printAsFullVec() {
        return this.name4vector + ".4S";
    }

    public String printAsFullByteVec() {
        return this.name4vector + ".16B";
    }

    public String printAsPartNOfVec(int n) {
        return this.name4vector + ".S[" + n + "]";
    }
}
