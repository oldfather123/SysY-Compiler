package backend.instruction;

import backend.component.ObjInstr;
import backend.operand.ObjFPhyReg;
import backend.operand.ObjFVirReg;
import backend.operand.ObjImm;
import backend.operand.ObjOperand;
import ir.value.Value;

public class ObjStore extends ObjInstr {
    private ObjOperand pointer;
    private ObjOperand value;
    private ObjOperand offset;
    private boolean canBeAlter = false;

    public ObjStore(ObjOperand pointer, ObjOperand value, ObjOperand offset, String type) {
        super(type);
        this.pointer = pointer;
        this.value = value;
        this.offset = offset;
        addUseReg(this.pointer, pointer);
        addUseReg(this.value, value);
        addUseReg(this.offset, offset);
    }

    public ObjOperand getPointer() {
        return pointer;
    }

    public ObjOperand getValue(ObjOperand value) {
        return value;
    }

    public void setCanBeAlter(boolean canBeAlter) {
        this.canBeAlter = canBeAlter;
    }

    public int getOffset() {
        return ((ObjImm) offset).getImmediate();
    }

    public void changeOffset(int immediate) {
        this.offset = new ObjImm(immediate);
    }

    public void setOffset(ObjOperand offset) {
        addUseReg(this.offset, offset);
        this.offset = offset;
    }

    public void setPointer(ObjOperand pointer) {
        addUseReg(this.pointer, pointer);
        this.pointer = pointer;
    }

    public void setValue(ObjOperand value) {
        addUseReg(this.value, value);
        this.value = value;
    }

    @Override
    public String toString() {
        // if (value instanceof ObjFPhyReg || value instanceof ObjFVirReg){
        //     return "fsd\t" + value + ",\t" + offset + "(" + pointer + ")";
        // }
        return getType() + "\t" + value.toString() + ",\t" + offset.toString() + "(" + pointer.toString() + ")";
    }

    @Override
    public void replaceReg(ObjOperand oldReg, ObjOperand newReg) {
        if (value.equals(oldReg))
            setValue(newReg);
        if (pointer.equals(oldReg))
            setPointer(newReg);
        if (offset.equals(oldReg))
            setOffset(newReg);
    }
}
