package backend.operand;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

public class ObjPhyReg extends ObjReg {
    private final int index;
    private final String name;

    public final static HashMap<String, Integer> nameToIndex = new HashMap<>();
    public final static HashMap<Integer, String> indexToName = new HashMap<>();
    public final static ArrayList<ObjPhyReg> regs = new ArrayList<>();
    private boolean isAllocated;

    public ObjPhyReg(String name) {
        super(name);
        this.name = name;
        this.index = nameToIndex.get(name);
        this.color = this.index;
    }

    public ObjPhyReg(int index) {
        super(indexToName.get(index));
        this.index = index;
        this.name = indexToName.get(index);
        this.color = this.index;
    }

    public ObjPhyReg(int index, boolean isAllocated) {
        this.name = indexToName.get(index);
        this.index = index;
        this.isAllocated = isAllocated;
        this.color = this.index;
    }
    public void setAllocated(boolean isAllocated) {
        this.isAllocated = isAllocated;
    }
    public int getIndex() {
        return index;
    }

    static {
        indexToName.put(0, "zero");
        indexToName.put(1, "ra");
        indexToName.put(2, "sp");
        indexToName.put(3, "gp");
        indexToName.put(4, "tp");
        indexToName.put(5, "t0");
        indexToName.put(6, "t1");
        indexToName.put(7, "t2");
        indexToName.put(8, "s0");
        indexToName.put(9, "s1");
        indexToName.put(10, "a0");
        indexToName.put(11, "a1");

        for (int i = 12; i <= 17; i++) {
            indexToName.put(i, "a" + (i - 10));
        }
        for (int i = 18; i <= 27; i++) {
            indexToName.put(i, "s" + (i - 16));
        }
        for (int i = 28; i <= 31; i++) {
            indexToName.put(i, "t" + (i - 25));
        }

        for (Map.Entry<Integer, String> entry : indexToName.entrySet()) {
            nameToIndex.put(entry.getValue(), entry.getKey());
        }
    }

    public final static ObjPhyReg ZERO = new ObjPhyReg("zero");
    public final static ObjPhyReg SP = new ObjPhyReg("sp");
    public final static ObjPhyReg RA = new ObjPhyReg("ra");

    static {
        regs.add(ZERO);
        regs.add(RA);
        regs.add(SP);
        regs.add(new ObjPhyReg("gp"));
        regs.add(new ObjPhyReg("tp"));
        regs.add(new ObjPhyReg("t0"));
        regs.add(new ObjPhyReg("t1"));
        regs.add(new ObjPhyReg("t2"));
        regs.add(new ObjPhyReg("s0"));
        regs.add(new ObjPhyReg("s1"));

        for (int i = 0; i <= 7; i++) {
            ObjPhyReg opr = new ObjPhyReg("a" + String.valueOf(i));
            regs.add(opr);
        }

        for (int i = 18; i <= 27; i++) {
            regs.add(new ObjPhyReg("s" + String.valueOf(i - 16)));
        }

        for (int i = 28; i <= 31; i++) {
            regs.add(new ObjPhyReg("t" + String.valueOf(i - 25)));
        }
    }

    @Override
    public String toString() {
        return name;
    }
}
