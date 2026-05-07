package midend.value.instrs;

import midend.Type;
import midend.value.BasicBlock;
import midend.value.Value;

public class FtoI extends Reg {
    public FtoI(Value op, BasicBlock parent) {
        super(parent);
        this.getOperandList().add(op);
        setType(Type.IntegerType.I32);
        for (int i = 0; i < this.getOperandList().size(); i++) {
            this.addUseOp(this.getOperandList().get(i), i);
        }
    }

    public String toString() {
        Value op = this.getOperandList().get(0);
        return getName() + " = fptosi float " + op.getName() + " to i32";
    }
}
