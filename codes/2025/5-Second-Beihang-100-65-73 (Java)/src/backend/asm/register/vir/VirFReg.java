package backend.asm.register.vir;

import backend.asm.register.FReg;
import backend.asm.register.phy.PhyReg;

public class VirFReg extends VirReg implements FReg {
    private final int myIndex;
    private final boolean vector;

    private static int INDEX = 0;

    public VirFReg(boolean vector) {
        super();
        this.vector = vector;

        myIndex = INDEX++;
    }

    public boolean isVector() {
        return vector;
    }

    public static int getINDEX() {
        return INDEX;
    }

    @Override
    public String printAsOperand() {
        if (this.getPhyReg() instanceof PhyReg phyReg) {
            return phyReg.printAsOperand();
        } else {
            return "VIR_F_" + this.myIndex;
        }
    }
}
