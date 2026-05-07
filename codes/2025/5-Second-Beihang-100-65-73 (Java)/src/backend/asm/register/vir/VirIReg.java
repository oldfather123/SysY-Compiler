package backend.asm.register.vir;

import backend.asm.register.IReg;
import backend.asm.register.phy.PhyIReg;
import backend.asm.register.phy.PhyReg;

public class VirIReg extends VirReg implements IReg {
    private final int myIndex;
    private boolean isDouble = false; // 区分存的是 32 位还是 64 位，方便确定要不要用 store / load double

    private static int INDEX = 0;

    public VirIReg() {
        super();
        myIndex = INDEX++;
    }

    public static int getINDEX() {
        return INDEX;
    }

    public boolean isDouble() {
        return isDouble;
    }

    public void setDouble(boolean isDouble) {
        this.isDouble = isDouble;
    }

    @Override
    public String printAsOperand() {
        if (this.getPhyReg() instanceof PhyReg phyReg) {
            if (isDouble) {
                return ((PhyIReg) phyReg).printAsDouble();
            }
            return phyReg.printAsOperand();
        } else {
            return "VIR_I_" + this.myIndex;
        }
    }

    public String printAsWord() {
        if (this.getPhyReg() instanceof PhyReg phyReg) {
            return phyReg.printAsOperand();
        } else {
            return "VIR_I_" + this.myIndex;
        }
    }
}
