package backend.regs;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

public class RegGeter {
    public final static HashMap<Integer,String> indexToNameInt = new HashMap<>();
    public final static HashMap<String,Integer> nameToIndexInt = new HashMap<>();
    public final static ArrayList<AsmPhyReg> AregsInt = new ArrayList<>();
    public final static ArrayList<AsmPhyReg> AllRegsInt = new ArrayList<>();

    static {
        indexToNameInt.put(0, "zero");
        indexToNameInt.put(1, "ra");
        indexToNameInt.put(2, "sp");
        indexToNameInt.put(3, "gp");
        indexToNameInt.put(4, "tp");
        indexToNameInt.put(5, "t0");
        indexToNameInt.put(6, "t1");
        indexToNameInt.put(7, "t2");
        indexToNameInt.put(8, "s0");
        indexToNameInt.put(9, "s1");
        indexToNameInt.put(10, "a0");
        indexToNameInt.put(11, "a1");

        for(int i = 12; i <= 17; i ++)
            indexToNameInt.put(i, "a" + (i - 10));
        for(int i = 18; i <= 27; i ++)
            indexToNameInt.put(i, "s" + (i - 16));
        for(int i = 28; i <= 31; i ++)
            indexToNameInt.put(i, "t" + (i - 25));

        for (Map.Entry<Integer, String> entry : indexToNameInt.entrySet())
            nameToIndexInt.put(entry.getValue(), entry.getKey());
    }

    public final static AsmPhyReg ZERO = new AsmPhyReg("zero");
    public final static AsmPhyReg SP = new AsmPhyReg("sp");
    public final static AsmPhyReg RA = new AsmPhyReg("ra");

    static{
        AllRegsInt.add(ZERO);
        AllRegsInt.add(RA);
        AllRegsInt.add(SP);
        AllRegsInt.add(new AsmPhyReg("gp"));
        AllRegsInt.add(new AsmPhyReg("tp"));
        AllRegsInt.add(new AsmPhyReg("t0"));
        AllRegsInt.add(new AsmPhyReg("t1"));
        AllRegsInt.add(new AsmPhyReg("t2"));
        AllRegsInt.add(new AsmPhyReg("s0"));
        AllRegsInt.add(new AsmPhyReg("s1"));

        for(int i = 0; i <= 7 ; i ++)
        {
            AsmPhyReg apr= new AsmPhyReg("a" + i);
            AllRegsInt.add(apr);
            AregsInt.add(apr);
        }

        for(int i = 18; i <= 27; i ++)
            AllRegsInt.add(new AsmPhyReg("s" + (i - 16)));

        for(int i = 28; i <= 31; i ++)
            AllRegsInt.add(new AsmPhyReg("t" + (i - 25)));
    }

    public final static HashMap<Integer,String> indexToNameFloat = new HashMap<>();
    public final static HashMap<String,Integer> nameToIndexFloat = new HashMap<>();
    public final static ArrayList<AsmFPhyReg> AregsFloat = new ArrayList<>();
    public final static ArrayList<AsmFPhyReg> AllRegsFloat = new ArrayList<>();
    static {
        for(int i = 0; i <= 7; i ++)
        {
            indexToNameFloat.put(i, "ft" + i);
        }

        indexToNameFloat.put(8, "fs0");
        indexToNameFloat.put(9, "fs1");

        for(int i = 10; i <= 17; i ++)
        {
            indexToNameFloat.put(i, "fa" + String.valueOf(i - 10));
        }
        for(int i = 18; i <= 27; i ++)
        {
            indexToNameFloat.put(i, "fs" + String.valueOf(i - 16));

        }
        for(int i = 28; i <= 31; i ++)
        {
            indexToNameFloat.put(i, "ft" + String.valueOf(i - 20));

        }

        for (Map.Entry<Integer, String> entry : indexToNameFloat.entrySet())
            nameToIndexFloat.put(entry.getValue(), entry.getKey());

        for(int i = 0; i <= 7; i ++)
        {

            AllRegsFloat.add(new AsmFPhyReg("ft" + i));
        }


        AllRegsFloat.add(new AsmFPhyReg("fs0"));

        AllRegsFloat.add(new AsmFPhyReg("fs1"));

        for(int i = 10; i <= 17; i ++)
        {
            AsmFPhyReg afpr= new AsmFPhyReg("fa" + (i - 10));
            AllRegsFloat.add(afpr);
            AregsFloat.add(afpr);
        }
        for(int i = 18; i <= 27; i ++)
        {

            AllRegsFloat.add(new AsmFPhyReg("fs" + (i - 16)));
        }
        for(int i = 28; i <= 31; i ++)
        {

            AllRegsFloat.add(new AsmFPhyReg("ft" + (i - 20)));
        }

    }
}
