package Backend.instruction;

import Backend.component.ObjBlock;
import Backend.operand.ObjOperand;

public class ObjBranch extends ObjInstr{
    private ObjOperand src = null;
    private boolean hasSrc = false, condType;
    private ObjBlock target;
    // j
    public ObjBranch(ObjBlock target) {
        this.target = target;
    }
    // beqz bneq
    public ObjBranch(boolean condType, ObjOperand src, ObjBlock target) {
        addUseReg(this.src, src);
        this.src = src;
        this.target = target;
        this.hasSrc = true;
        this.condType = condType;
    }

    public void setSrc(ObjOperand src) {
        addUseReg(this.src, src);
        this.src = src;
    }

    @Override
    public void replaceReg(ObjOperand oldReg, ObjOperand newReg) {
        if(src != null && src.equals(oldReg))
            setSrc(newReg);
    }
    @Override
    public void replaceUseReg(ObjOperand oldReg, ObjOperand newReg) {
        if(src != null && src.equals(oldReg))
            setSrc(src);
    }

    public ObjOperand getSrc(){
        return src;
    }

    public ObjBlock getTarget() {
        return target;
    }
    public boolean getCondType() {
        return condType;
    }

    public boolean isHasSrc(){
        return hasSrc;
    }

    @Override
    public String toString() {
        if(hasSrc) {
            String s = "";
            if(condType)
                s = "beqz\t";
            else s = "bnez\t";
            return s + src + ",\t" + target.getName();
        }
        else return "   j\t" + target.getName();
    }
}
