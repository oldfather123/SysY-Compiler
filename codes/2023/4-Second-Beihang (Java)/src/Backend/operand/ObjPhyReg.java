package Backend.operand;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;

public class ObjPhyReg extends ObjReg {
    public final static HashMap<Integer, String> indexToName = new HashMap<>();
    private final static HashMap<String, Integer> nameToIndex = new HashMap<>();
    public final static ArrayList<ObjPhyReg> A = new ArrayList<>();
    public final static ArrayList<ObjPhyReg> AllRegs = new ArrayList<>();

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

        for(int i = 12; i <= 17; i ++)
            indexToName.put(i, "a" + (i - 10));
        for(int i = 18; i <= 27; i ++)
            indexToName.put(i, "s" + (i - 16));
        for(int i = 28; i <= 31; i ++)
            indexToName.put(i, "t" + (i - 25));

        for (Map.Entry<Integer, String> entry : indexToName.entrySet())
            nameToIndex.put(entry.getValue(), entry.getKey());
    }
    public final static ObjPhyReg ZERO = new ObjPhyReg("zero");
    // public final static ObjPhyReg A0 = new ObjPhyReg("a0");
    public final static ObjPhyReg SP = new ObjPhyReg("sp");
    public final static ObjPhyReg RA = new ObjPhyReg("ra");
    static{
        AllRegs.add(ZERO);
        AllRegs.add(RA);
        AllRegs.add(SP);
        AllRegs.add(new ObjPhyReg("gp"));
        AllRegs.add(new ObjPhyReg("tp"));
        AllRegs.add(new ObjPhyReg("t0"));
        AllRegs.add(new ObjPhyReg("t1"));
        AllRegs.add(new ObjPhyReg("t2"));
        AllRegs.add(new ObjPhyReg("s0"));
        AllRegs.add(new ObjPhyReg("s1"));

        for(int i = 0; i <= 7 ; i ++)
        {
            ObjPhyReg opr= new ObjPhyReg("a" + String.valueOf(i));
            AllRegs.add(opr);
            A.add(opr);
        }

        for(int i = 18; i <= 27; i ++)
            AllRegs.add(new ObjPhyReg("s" + String.valueOf(i - 16)));

        for(int i = 28; i <= 31; i ++)
            AllRegs.add(new ObjPhyReg("t" + String.valueOf(i - 25)));

    }
    private final int index;
    private final String name;
    private boolean isAllocated;

    public ObjPhyReg(String name) {
        this.name = name;
        this.index = nameToIndex.get(name);
        this.isAllocated = false;
        this.color=this.index;
    }
    public ObjPhyReg(int index) {
        this.name = indexToName.get(index);
        this.index = index;
        this.isAllocated = false;
    }
    public ObjPhyReg(int index, boolean isAllocated) {
        this.name = indexToName.get(index);
        this.index = index;
        this.isAllocated = isAllocated;
    }
    public void setAllocated(boolean isAllocated) {
        this.isAllocated = isAllocated;
    }
    public int getIndex() {
        return index;
    }

    @Override
    public String toString() {
        return  name;
    }
}
