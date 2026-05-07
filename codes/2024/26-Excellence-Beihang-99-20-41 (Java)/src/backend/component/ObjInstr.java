package backend.component;

import backend.operand.ObjOperand;
import backend.operand.ObjReg;

import java.util.ArrayList;

public class ObjInstr {
    private String type;
    public ArrayList<ObjReg> regDef, regUse;
    public ArrayList<ObjReg> livein = new ArrayList<>();
    public int index = -1;

    public ObjInstr(String type) {
        this.type = type;
        this.regDef = new ArrayList<>();
        this.regUse = new ArrayList<>();
    }

    public String getType() {
        return type;
    }

    public void changeType(String type) {
        this.type = type;
    }

    private void addUse(ObjOperand reg) {
        if (reg instanceof ObjReg)
            regUse.add((ObjReg) reg);
    }

    private void addDef(ObjOperand reg) {
        if (reg instanceof ObjReg)
            regDef.add((ObjReg) reg);
    }

    private void removeDef(ObjOperand reg) {
        if (reg instanceof ObjReg)
            regDef.remove((ObjReg) reg);
    }

    private void removeUse(ObjOperand reg) {
        if (reg instanceof ObjReg)
            regUse.remove((ObjReg) reg);
    }

    public void addDefReg(ObjOperand oldReg, ObjOperand newReg) {
        if (oldReg != null)
            removeDef(oldReg);
        addDef(newReg);
    }

    public void addUseReg(ObjOperand oldReg, ObjOperand newReg) {
        if (oldReg != null)
            removeUse(oldReg);
        addUse(newReg);
    }

    @Override
    public String toString() {
        return type;
    }

    public void setType(String type) {
        this.type = type;
    }

    public void replaceReg(ObjOperand oldReg, ObjOperand newReg) {
        throw new RuntimeException("replaceReg not implemented");
    }
}
