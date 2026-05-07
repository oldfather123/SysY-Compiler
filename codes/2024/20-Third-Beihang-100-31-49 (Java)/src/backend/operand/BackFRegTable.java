package backend.operand;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;

public class BackFRegTable {
    private static HashMap<Integer, BackIReg> num2reg = new HashMap<>();
    private static HashMap<String, Integer> reg2num = new HashMap<>();

    static {
        for (int i = 0; i <= 7; i++) {
            num2reg.put(i, new BackIReg("ft" + i));
        }
        for (int i = 0; i <= 1; i++) {
            num2reg.put(i + 8, new BackIReg("fs" + i));
        }
        for (int i = 0; i <= 7; i++) {
            num2reg.put(i + 10, new BackIReg("fa" + i));
        }
        for (int i = 2; i <= 7; i++) {
            num2reg.put(i + 16, new BackIReg("fs" + i));
        }
        for (int i = 8; i <= 11; i++) {
            num2reg.put(i + 20, new BackIReg("ft" + i));
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
        for (int i = 0; i <= 11; i++) {
            tempRegs.add(getBackReg("ft" + i));
        }
        for (int i = 0; i <= 7; i++) {
            tempRegs.add(getBackReg("fa" + i));
        }
        return tempRegs;
    }
}
