package backend.operand;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;

public class BackRegTable {
    private static HashMap<Integer, BackIReg> num2reg = new HashMap<>();
    private static HashMap<String, Integer> reg2num = new HashMap<>();

    static {
        num2reg.put(0, new BackIReg("zero"));
        num2reg.put(1, new BackIReg("ra"));
        num2reg.put(2, new BackIReg("sp"));
        num2reg.put(3, new BackIReg("gp"));
        num2reg.put(4, new BackIReg("tp"));
        for (int i = 0; i <= 2; i++) {
            num2reg.put(i + 5, new BackIReg("t" + i));
        }
        for (int i = 0; i <= 1; i++) {
            num2reg.put(i + 8, new BackIReg("s" + i));
        }
        for (int i = 0; i <= 7; i++) {
            num2reg.put(i + 10, new BackIReg("a" + i));
        }
        for (int i = 2; i <= 11; i++) {
            num2reg.put(i + 16, new BackIReg("s" + i));
        }
        for (int i = 2; i <= 6; i++) {
            num2reg.put(i + 26, new BackIReg("t" + i));
        }
    }

    static {
        for (Map.Entry<Integer, BackIReg> entry : num2reg.entrySet()) {
            Integer num = entry.getKey();
            BackIReg reg = entry.getValue();
            reg2num.put(reg.getName(), num);
        }
    }


    public static BackIReg getBackReg(String s) {
        return num2reg.get(reg2num.get(s));
    }

    public static HashSet<BackIReg> getAllTempRegs() {
        HashSet<BackIReg> tempRegs = new HashSet<>();
        for (int i = 0; i <= 6; i++) {
            tempRegs.add(getBackReg("t" + i));
        }
        for (int i = 0; i <= 7; i++) {
            tempRegs.add(getBackReg("a" + i));
        }
        return tempRegs;
    }
}
