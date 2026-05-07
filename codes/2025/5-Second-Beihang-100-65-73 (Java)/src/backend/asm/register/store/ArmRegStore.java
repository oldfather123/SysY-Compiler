package backend.asm.register.store;

import backend.asm.register.phy.PhyFReg;
import backend.asm.register.phy.PhyIReg;

public class ArmRegStore extends RegStore{
    private static final ArmRegStore  INSTANCE = new ArmRegStore();

    private final PhyIReg tempReg = new PhyIReg("W8", "X8"); // X8 作为临时寄存器使用（用于处理长立即数等） todo X8 在某些规范下可能会有其他用途

    private ArmRegStore() {
        super(new PhyIReg("WZR", "XZR"),
                new PhyIReg("LR", "LR"),
                new PhyIReg("SP", "SP"));

        for (int i = 0; i <= 7; i++) {      // 整数参数寄存器
            intArgRegs.add(new PhyIReg("W" + i, "X" + i));
        }

        for (int i = 9; i <= 18; i++) {     // Caller-saved todo: X16, X17, X18 可能有别的用途
            intTempRegs.add(new PhyIReg("W" + i, "X" + i));
        }

        for (int i = 19; i <= 29; i++) {    // Callee-saved
            intSavedRegs.add(new PhyIReg("W" + i, "X" + i));
        }

        // todo: 现在浮点寄存器先都按 S 访问（浮点）
        // Vn：128 位视图（默认） Dn：64 位（double） Sn：32 位（float）
        for (int i = 0; i <= 7; i++) {
            fltArgRegs.add(new PhyFReg("S" + i, "V" + i, "Q" + i));
        }

        for (int i = 8; i <= 15; i++) {
            fltTempRegs.add(new PhyFReg("S" + i, "V" + i, "Q" + i));
        }

        for (int i = 16; i <= 31; i++) {
            fltSavedRegs.add(new PhyFReg("S" + i, "V" + i, "Q" + i));
        }

        allTempRegs.addAll(intTempRegs);
        allTempRegs.addAll(fltTempRegs);
    }

    public static ArmRegStore getInstance() {
        return INSTANCE;
    }

    @Override
    public PhyIReg getReservedReg() {
        return tempReg;
    }
}
