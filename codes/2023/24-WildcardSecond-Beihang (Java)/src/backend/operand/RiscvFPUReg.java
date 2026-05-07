package backend.operand;

import java.util.LinkedHashMap;

public class RiscvFPUReg extends RiscvPhyReg{
    private static final LinkedHashMap<Integer, String> nameMap = new LinkedHashMap<>();
    static
    {
        nameMap.put(0, "ft0");
        nameMap.put(1, "ft1");
        nameMap.put(2, "ft2");
        nameMap.put(3, "ft3");
        nameMap.put(4, "ft4");
        nameMap.put(5, "ft5");
        nameMap.put(6, "ft6");
        nameMap.put(7, "ft7");
        nameMap.put(8, "fs0");
        nameMap.put(9, "fs1");
        nameMap.put(10, "fa0");
        nameMap.put(11, "fa1");
        nameMap.put(12, "fa2");
        nameMap.put(13, "fa3");
        nameMap.put(14, "fa4");
        nameMap.put(15, "fa5");
        nameMap.put(16, "fa6");
        nameMap.put(17, "fa7");
        nameMap.put(18, "fs2");
        nameMap.put(19, "fs3");
        nameMap.put(20, "fs4");
        nameMap.put(21, "fs5");
        nameMap.put(22, "fs6");
        nameMap.put(23, "fs7");
        nameMap.put(24, "fs8");
        nameMap.put(25, "fs9");
        nameMap.put(26, "fs10");
        nameMap.put(27, "fs11");
        nameMap.put(28, "ft8");
        nameMap.put(29, "ft9");
        nameMap.put(30, "ft10");
        nameMap.put(31, "ft11");
    }

    private static final LinkedHashMap<Integer, RiscvFPUReg> riscvFPURegMap = new LinkedHashMap<>();
    private int index;
    private String name;
    private boolean isAllocated;

    public RiscvFPUReg() {}
    public static RiscvFPUReg getRiscvFPUReg(int index) {
        if (riscvFPURegMap.isEmpty()) {
            for (int i = 0; i <= 31; i++) {
                RiscvFPUReg riscvFPUReg = new RiscvFPUReg();
                riscvFPUReg.index = i;
                riscvFPUReg.name = nameMap.get(i);
                riscvFPUReg.isAllocated = false;
                riscvFPURegMap.put(i, riscvFPUReg);
            }
        }
        return riscvFPURegMap.get(index);
    }
    public static LinkedHashMap<Integer, RiscvFPUReg> getAllReg(){
        return riscvFPURegMap;
    }

    public int getIndex() {
        return index;
    }

    public String getName(int index) {
        return riscvFPURegMap.get(index).name;
    }

    @Override
    public String toString() {
        return nameMap.get(index);
    }
}

