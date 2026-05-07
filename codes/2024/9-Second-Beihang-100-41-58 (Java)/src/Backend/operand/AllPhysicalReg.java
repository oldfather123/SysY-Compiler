package Backend.operand;

import java.util.HashMap;

public class AllPhysicalReg {
    public static HashMap<Integer, PhysicalReg> phyAllRegs = new HashMap<>();
    public static HashMap<Integer, FPhysicalReg> fPhyAllRegs = new HashMap<>();
    public static HashMap<Integer, PhysicalReg> phyA = new HashMap<>();
    public static HashMap<Integer, FPhysicalReg> fPhyA = new HashMap<>();
    public static PhysicalReg ZERO = new PhysicalReg("zero");
    public static PhysicalReg SP = new PhysicalReg("sp");
    public static PhysicalReg RA = new PhysicalReg("ra");
    public static int counterForPhy = 0;
    public static int counterForFPhy = 0;

    public static void init() {
        phyAllRegs.put(ZERO.getIndex(), ZERO);
        phyAllRegs.put(RA.getIndex(), RA);
        phyAllRegs.put(SP.getIndex(), SP);
        phyAllRegs.put(3, new PhysicalReg("gp"));
        phyAllRegs.put(4, new PhysicalReg("tp"));
        phyAllRegs.put(5, new PhysicalReg("t0"));
        phyAllRegs.put(6, new PhysicalReg("t1"));
        phyAllRegs.put(7, new PhysicalReg("t2"));
        phyAllRegs.put(8, new PhysicalReg("s0"));
        phyAllRegs.put(9, new PhysicalReg("s1"));
        phyAllRegs.put(10, new PhysicalReg("a0"));
        phyA.put(0, phyAllRegs.get(10));
        phyAllRegs.put(11, new PhysicalReg("a1"));
        phyA.put(1, phyAllRegs.get(11));
        phyAllRegs.put(12, new PhysicalReg("a2"));
        phyA.put(2, phyAllRegs.get(12));
        phyAllRegs.put(13, new PhysicalReg("a3"));
        phyA.put(3, phyAllRegs.get(13));
        phyAllRegs.put(14, new PhysicalReg("a4"));
        phyA.put(4, phyAllRegs.get(14));
        phyAllRegs.put(15, new PhysicalReg("a5"));
        phyA.put(5, phyAllRegs.get(15));
        phyAllRegs.put(16, new PhysicalReg("a6"));
        phyA.put(6, phyAllRegs.get(16));
        phyAllRegs.put(17, new PhysicalReg("a7"));
        phyA.put(7, phyAllRegs.get(17));
        phyAllRegs.put(18, new PhysicalReg("s2"));
        phyAllRegs.put(19, new PhysicalReg("s3"));
        phyAllRegs.put(20, new PhysicalReg("s4"));
        phyAllRegs.put(21, new PhysicalReg("s5"));
        phyAllRegs.put(22, new PhysicalReg("s6"));
        phyAllRegs.put(23, new PhysicalReg("s7"));
        phyAllRegs.put(24, new PhysicalReg("s8"));
        phyAllRegs.put(25, new PhysicalReg("s9"));
        phyAllRegs.put(26, new PhysicalReg("s10"));
        phyAllRegs.put(27, new PhysicalReg("s11"));
        phyAllRegs.put(28, new PhysicalReg("t3"));
        phyAllRegs.put(29, new PhysicalReg("t4"));
        phyAllRegs.put(30, new PhysicalReg("t5"));
        phyAllRegs.put(31, new PhysicalReg("t6"));
        fPhyAllRegs.put(0, new FPhysicalReg("ft0"));
        fPhyAllRegs.put(1, new FPhysicalReg("ft1"));
        fPhyAllRegs.put(2, new FPhysicalReg("ft2"));
        fPhyAllRegs.put(3, new FPhysicalReg("ft3"));
        fPhyAllRegs.put(4, new FPhysicalReg("ft4"));
        fPhyAllRegs.put(5, new FPhysicalReg("ft5"));
        fPhyAllRegs.put(6, new FPhysicalReg("ft6"));
        fPhyAllRegs.put(7, new FPhysicalReg("ft7"));
        fPhyAllRegs.put(8, new FPhysicalReg("fs0"));
        fPhyAllRegs.put(9, new FPhysicalReg("fs1"));
        fPhyAllRegs.put(10, new FPhysicalReg("fa0"));
        fPhyA.put(0, fPhyAllRegs.get(10));
        fPhyAllRegs.put(11, new FPhysicalReg("fa1"));
        fPhyA.put(1, fPhyAllRegs.get(11));
        fPhyAllRegs.put(12, new FPhysicalReg("fa2"));
        fPhyA.put(2, fPhyAllRegs.get(12));
        fPhyAllRegs.put(13, new FPhysicalReg("fa3"));
        fPhyA.put(3, fPhyAllRegs.get(13));
        fPhyAllRegs.put(14, new FPhysicalReg("fa4"));
        fPhyA.put(4, fPhyAllRegs.get(14));
        fPhyAllRegs.put(15, new FPhysicalReg("fa5"));
        fPhyA.put(5, fPhyAllRegs.get(15));
        fPhyAllRegs.put(16, new FPhysicalReg("fa6"));
        fPhyA.put(6, fPhyAllRegs.get(16));
        fPhyAllRegs.put(17, new FPhysicalReg("fa7"));
        fPhyA.put(7, fPhyAllRegs.get(17));
        fPhyAllRegs.put(18, new FPhysicalReg("fs2"));
        fPhyAllRegs.put(19, new FPhysicalReg("fs3"));
        fPhyAllRegs.put(20, new FPhysicalReg("fs4"));
        fPhyAllRegs.put(21, new FPhysicalReg("fs5"));
        fPhyAllRegs.put(22, new FPhysicalReg("fs6"));
        fPhyAllRegs.put(23, new FPhysicalReg("fs7"));
        fPhyAllRegs.put(24, new FPhysicalReg("fs8"));
        fPhyAllRegs.put(25, new FPhysicalReg("fs9"));
        fPhyAllRegs.put(26, new FPhysicalReg("fs10"));
        fPhyAllRegs.put(27, new FPhysicalReg("fs11"));
        fPhyAllRegs.put(28, new FPhysicalReg("ft8"));
        fPhyAllRegs.put(29, new FPhysicalReg("ft9"));
        fPhyAllRegs.put(30, new FPhysicalReg("ft10"));
        fPhyAllRegs.put(31, new FPhysicalReg("ft11"));
        SP.notNeedsColor();
        RA.notNeedsColor();
        for (int key : phyA.keySet()) {
            phyA.get(key).notNeedsColor();
        }
    }

    public static void addCounterForFPhy() {
        counterForFPhy++;
    }
}
