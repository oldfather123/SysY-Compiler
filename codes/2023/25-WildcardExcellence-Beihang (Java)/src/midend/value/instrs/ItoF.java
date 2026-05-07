package midend.value.instrs;

import midend.Type;
import midend.value.BasicBlock;
import midend.value.Value;

public class ItoF extends Reg {
    public ItoF(Value op, BasicBlock parent) {
        super(parent);
        this.getOperandList().add(op);
        setType(new Type.FloatType());
        for (int i = 0; i < this.getOperandList().size(); i++) {
            this.addUseOp(this.getOperandList().get(i), i);
        }
    }

    public String toString() {
        Value op = this.getOperandList().get(0);
        return getName() + " = sitofp i32 " + op.getName() + " to float";
    }
}
