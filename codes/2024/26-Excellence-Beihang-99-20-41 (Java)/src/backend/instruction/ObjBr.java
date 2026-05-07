package backend.instruction;

import backend.component.ObjBlock;
import backend.component.ObjFunction;
import backend.component.ObjInstr;
import backend.operand.ObjImm;
import backend.operand.ObjOperand;

public class ObjBr extends ObjInstr {
    //br i1 <cond>, label <iftrue>, label <iffalse>
    //br label <dst>
    private ObjOperand i1;
    private ObjBlock thenBlock; //then block
    private ObjBlock elseBlock; // else block
    private boolean isCond;
    private ObjFunction objFunction;

    public ObjBr(ObjBlock block) {
        super("j");
        i1 = null;
        isCond = false;
        thenBlock = block;
        elseBlock = null;
    }

    public ObjBr(ObjFunction objFunction) {
        super("jal");
        this.objFunction = objFunction;
    }

    public ObjBr(ObjOperand operand, ObjBlock thenBlock, ObjBlock elseBlock) {
        super("bnez");
//        if (operand.equals(new ObjImm(1))) {
//            super.setType("bnez");
//        } else {
//            super.setType("beqz");
//        }
        addUseReg(this.i1, operand);
        i1 = operand;
        isCond = true;
        this.thenBlock = thenBlock;
        this.elseBlock = elseBlock;
    }

    public ObjOperand getI1() {
        return i1;
    }

    public ObjBlock getThenBlock() {
        return thenBlock;
    }

    public ObjBlock getElseBlock() {
        return elseBlock;
    }

    public void setI1(ObjOperand i1) {
        addUseReg(this.i1, i1);
        this.i1 = i1;
    }

    @Override
    public String toString() {
        if (isCond) {
            {
                return getType() + "\t" + i1 + ",\t" + thenBlock.getName()
                        + "\n\tj\t" + elseBlock.getName();
            }
        } else {
            if (getType().equals("jal")) {
                return "jal\tra,\t" + objFunction.getName();
            }
            return getType() + "\t" + thenBlock.getName();
        }
    }

    @Override
    public void replaceReg(ObjOperand oldReg, ObjOperand newReg) {
        if(i1 != null && i1.equals(oldReg))
            setI1(newReg);
    }
}
