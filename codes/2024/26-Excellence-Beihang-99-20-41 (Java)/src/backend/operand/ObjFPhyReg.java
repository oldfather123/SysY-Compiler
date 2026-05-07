package backend.operand;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

public class ObjFPhyReg extends ObjReg {
    private final int index;
    private final String name;

    public final static HashMap<Integer, String> indexToName = new HashMap<>();
    public final static HashMap<String, Integer> nameToIndex = new HashMap<>();
    public final static ArrayList<ObjFPhyReg> regs = new ArrayList<>();
    private boolean isAllocated;

    public ObjFPhyReg(String name) {
        super(name);
        this.name = name;
        this.index = nameToIndex.get(name);
        this.color = this.index;
    }

    public ObjFPhyReg(int index) {
        super(indexToName.get(index));
        this.name = indexToName.get(index);
        this.index = index;
        this.color = this.index;
    }

    public ObjFPhyReg(int index, boolean isAllocated) {
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
        for (int i = 0; i <= 7; i++) {
            indexToName.put(i, "ft" + i);
        }

        indexToName.put(8, "fs0");
        indexToName.put(9, "fs1");

        for (int i = 10; i <= 17; i++) {
            indexToName.put(i, "fa" + String.valueOf(i - 10));
        }
        for (int i = 18; i <= 27; i++) {
            indexToName.put(i, "fs" + String.valueOf(i - 16));
        }
        for (int i = 28; i <= 31; i++) {
            indexToName.put(i, "ft" + String.valueOf(i - 20));
        }
        for (Map.Entry<Integer, String> entry : indexToName.entrySet()) {
            nameToIndex.put(entry.getValue(), entry.getKey());
        }
        for (int i = 0; i <= 7; i++) {
            regs.add(new ObjFPhyReg("ft" + i));
        }

        regs.add(new ObjFPhyReg("fs0"));
        regs.add(new ObjFPhyReg("fs1"));

        for (int i = 10; i <= 17; i++) {
            ObjFPhyReg opr = new ObjFPhyReg("fa" + String.valueOf(i - 10));
            regs.add(opr);
        }
        for (int i = 18; i <= 27; i++) {
            regs.add(new ObjFPhyReg("fs" + String.valueOf(i - 16)));
        }
        for (int i = 28; i <= 31; i++) {
            regs.add(new ObjFPhyReg("ft" + String.valueOf(i - 20)));
        }
    }

    @Override
    public String toString() {
        return name;
    }
}
