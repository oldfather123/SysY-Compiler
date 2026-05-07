package midend.value.instrs;

import midend.Type;
import midend.value.BasicBlock;
import midend.value.Value;

public class Zext extends Reg {
    Type targetType;

    public Zext(Value op, Type targetType, BasicBlock parent) {
        super(parent);
        this.getOperandList().add(op);
        this.addUseOp(op, 0);
        setType(targetType);
        this.targetType = targetType;
    }

    public String toString() {
        Value op = this.getOperandList().get(0);
        return getName() + " = zext " + op.getType() + " " + op.getName()
                + " to " + targetType.toString();
    }
}
