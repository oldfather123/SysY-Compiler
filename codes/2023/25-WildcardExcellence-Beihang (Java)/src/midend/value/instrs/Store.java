package midend.value.instrs;

import midend.Type;
import midend.value.BasicBlock;
import midend.value.Constant;
import midend.value.Value;

public class Store extends Instr {
    public Value getOpValue() {
        return this.getOperandList().get(0);
    }

    public Store(Value value, Value pointer, BasicBlock parent) {
        super(parent);
        Type targetType = pointer.getType().getElementType();
        if (targetType.isIntegerType() && value.getType().isFloatType()) {
            // FtoI
            value = FtoI32(value, parent);
        } else if (targetType.isFloatType() && value.getType().isIntegerType()) {
            // ItoF
            value = I32toF(value, parent);
        }
        this.getOperandList().add(value);
        this.getOperandList().add(pointer);
        for (int i = 0; i < this.getOperandList().size(); i++) {
            this.addUseOp(this.getOperandList().get(i), i);
        }

        this.getParent().addInstr(this);
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        Value value = this.getOperandList().get(0);
        Value pointer = this.getOperandList().get(1);
        sb.append("store ");
        sb.append(value.getType()).append(" ");
        sb.append(value.getName()).append(", ");
        sb.append(pointer.getType().toString()).append(" ");
        sb.append(pointer.getName());
        return sb.toString();
    }
}
