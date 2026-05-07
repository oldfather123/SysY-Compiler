package backend.asm.register.store;

import backend.asm.register.phy.PhyFReg;
import backend.asm.register.phy.PhyIReg;

public class RvRegStore extends RegStore {
    private static final RvRegStore INSTANCE = new RvRegStore();

    // risc-v 特有的两个寄存器，后续考虑直接纳入寄存器分配
    private final PhyIReg gp;
    private final PhyIReg tp;
    private final PhyIReg s11;

    private RvRegStore() {
        super(new PhyIReg("zero", "zero"),
                new PhyIReg("ra", "ra"),
                new PhyIReg("sp", "sp"));
        gp = new PhyIReg("gp", "gp");
        tp = new PhyIReg("tp", "tp");
        s11 = new PhyIReg("s11", "s11");

        for (int i = 0; i <= 6; i++) {      //  7 个 t
            intTempRegs.add(new PhyIReg("t" + i, "t" + i));
        }

        for (int i = 0; i <= 10; i++) {     // 12 个 s 分出去一个用来修正立即数
            intSavedRegs.add(new PhyIReg("s" + i, "s" + i));
        }

        for (int i = 0; i <= 7; i++) {      //  8 个 a
            intArgRegs.add(new PhyIReg("a" + i, "a" + i));
        }

        for (int i = 0; i <= 11; i++) {     // 12 个 ft
            fltTempRegs.add(new PhyFReg("ft" + i, "ft" + i, "ft" + i));
        }

        for (int i = 0; i <= 11; i++) {     // 12 个 fs
            fltSavedRegs.add(new PhyFReg("fs" + i, "fs" + i, "fs" + i));
        }

        for (int i = 0; i <= 7; i++) {      //  8 个 fa
            fltArgRegs.add(new PhyFReg("fa" + i, "fa" + i, "fa" + i));
        }

        allTempRegs.addAll(intTempRegs);
        allTempRegs.addAll(fltTempRegs);
    }

    public static RvRegStore getInstance() {
        return INSTANCE;
    }

    @Override
    public PhyIReg getReservedReg() {
        return this.s11;
    }
}
