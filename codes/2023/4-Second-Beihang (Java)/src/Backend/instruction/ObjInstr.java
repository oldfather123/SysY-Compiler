package Backend.instruction;

import Backend.component.ObjBlock;
import Backend.operand.ObjOperand;
import Backend.operand.ObjPhyReg;
import Backend.operand.ObjReg;
import Utils.DataStruct.IList;

import java.util.ArrayList;

public class ObjInstr {

    private IList.INode<ObjInstr, ObjBlock> node;
    public ArrayList<ObjReg> regDef, regUse;
    public ArrayList<ObjReg> livein=new ArrayList<>();
    public ObjInstr() {
        this.node = new IList.INode<>(this);
        regDef = new ArrayList<>();
        regUse = new ArrayList<>();
    }
    public IList.INode<ObjInstr, ObjBlock> getNode() {
        return node;
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

    public void replaceReg(ObjOperand oldReg, ObjOperand newReg) {}
    public void replaceUseReg(ObjOperand oldReg, ObjOperand newReg) {}

    public ArrayList<ObjReg> getWriteRegs()
    {
        return new ArrayList<>(regDef);
    }

    public ArrayList<ObjReg> getReadRegs()
    {
        ArrayList<ObjReg> readRegs = regUse;

        if (this instanceof ObjCall)
        {
            readRegs.add(ObjPhyReg.SP);
        }

        return readRegs;
    }
    public ArrayList<ObjReg> getRegDef()
    {
        return regDef;
    }

    public ArrayList<ObjReg> getRegUse()
    {
        return regUse;
    }
}
