package backend.operand;

import java.util.LinkedHashMap;

public class RiscvCPUReg extends RiscvPhyReg{
    private static final LinkedHashMap<Integer, String> nameMap = new LinkedHashMap<>();
    static
    {
        nameMap.put(0, "zero");//static
        nameMap.put(1, "ra");//reserved
        nameMap.put(2, "sp");//reserved
        nameMap.put(3, "gp");//reserved
        nameMap.put(4, "tp");//reserved
        nameMap.put(5, "t0");
        nameMap.put(6, "t1");
        nameMap.put(7, "t2");
        nameMap.put(8, "s0");
        nameMap.put(9, "s1");
        nameMap.put(10, "a0");
        nameMap.put(11, "a1");
        nameMap.put(12, "a2");
        nameMap.put(13, "a3");
        nameMap.put(14, "a4");
        nameMap.put(15, "a5");
        nameMap.put(16, "a6");
        nameMap.put(17, "a7");
        nameMap.put(18, "s2");
        nameMap.put(19, "s3");
        nameMap.put(20, "s4");
        nameMap.put(21, "s5");
        nameMap.put(22, "s6");
        nameMap.put(23, "s7");
        nameMap.put(24, "s8");
        nameMap.put(25, "s9");
        nameMap.put(26, "s10");
        nameMap.put(27, "s11");
        nameMap.put(28, "t3");
        nameMap.put(29, "t4");
        nameMap.put(30, "t5");
        nameMap.put(31, "t6");
    }
    private static final LinkedHashMap<Integer, RiscvCPUReg> riscvCPURegMap = new LinkedHashMap<>();
    public static LinkedHashMap<Integer, RiscvCPUReg> getAllReg(){
        return riscvCPURegMap;
    }

    public int getIndex() {
        return index;
    }

    private int index;

    private String name;
    private boolean isAllocated;

    public RiscvCPUReg() {}

    public static RiscvCPUReg getRiscvCPUReg(int index) {
        if (riscvCPURegMap.isEmpty()) {
            for (int i = 0; i <= 31; i++) {
                RiscvCPUReg riscvCPUReg = new RiscvCPUReg();
                riscvCPUReg.index = i;
                riscvCPUReg.name = nameMap.get(i);
                riscvCPUReg.isAllocated = false;
                riscvCPURegMap.put(i, riscvCPUReg);
            }
        }
        return riscvCPURegMap.get(index);
    }

    public int getIndex(int index) {
        return riscvCPURegMap.get(index).index;
    }

    public String getName(int index) {
        return riscvCPURegMap.get(index).name;
    }

    @Override
    public String toString() {
        return nameMap.get(index);
    }
}