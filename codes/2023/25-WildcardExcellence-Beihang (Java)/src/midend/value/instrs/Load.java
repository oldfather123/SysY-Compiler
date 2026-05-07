package midend.value.instrs;

import midend.Type;
import midend.value.BasicBlock;
import midend.value.Value;

public class Load extends Reg {
    public Load(Value pointer, BasicBlock parent) {
        super(parent);
        this.getOperandList().add(pointer);
        setType((pointer.getType()).getElementType());
        for (int i = 0; i < this.getOperandList().size(); i++) {
            this.addUseOp(this.getOperandList().get(i), i);
        }
    }

    public String toString() {
        Value pointer = this.getOperandList().get(0);
        return getName() + " = " +
                "load " + getType().toString() + ", " +
                pointer.getType().toString() +
                " " + pointer.getName();
    }
}
