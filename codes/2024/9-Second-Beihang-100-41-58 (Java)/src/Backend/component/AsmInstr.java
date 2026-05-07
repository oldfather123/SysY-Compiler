package Backend.component;

import Backend.operand.AsmOperand;
import Backend.operand.AsmReg;

import java.util.ArrayList;

public class AsmInstr {
    private AsmReg regDef;
    private final ArrayList<AsmReg> regUse;
    private final ArrayList<AsmReg> liveIn = new ArrayList<>();

    public AsmInstr() {
        regDef = null;
        regUse = new ArrayList<>();
    }

    private void addUse(AsmOperand reg) {
        if (reg instanceof AsmReg) {
            regUse.add((AsmReg) reg);
            ((AsmReg) reg).addFrequency();
        }
    }

    private void setDef(AsmOperand reg) {
        if (reg instanceof AsmReg) {
            regDef = (AsmReg) reg;
            ((AsmReg) reg).addFrequency();
        }
    }

    public void setDefReg(AsmOperand oldReg, AsmOperand newReg) {
        if (oldReg != null) {
            if (oldReg instanceof AsmReg) {
                regDef = null;
            }
        }
        setDef(newReg);
    }

    public void addUseReg(AsmOperand oldReg, AsmOperand newReg) {
        if (oldReg != null) {
            if (oldReg instanceof AsmReg) {
                regUse.remove((AsmReg) oldReg);
            }
        }
        addUse(newReg);
    }

    public AsmReg getRegDef() {
        return regDef;
    }

    public ArrayList<AsmReg> getRegUse() {
        return regUse;
    }

    public ArrayList<AsmReg> getLiveIn() {
        return liveIn;
    }

    public void replaceReg(AsmOperand oldReg, AsmOperand newReg) {
        System.out.println("Left for subclasses to implement.");
    }
}
