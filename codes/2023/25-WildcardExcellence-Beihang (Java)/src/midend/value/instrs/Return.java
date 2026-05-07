package midend.value.instrs;

import midend.Type;
import midend.value.BasicBlock;
import midend.value.Value;

public class Return extends Instr {
    public Return(Value retValue, BasicBlock parent) {
        super(parent);
        if (retValue != null) {
            Type returnType = parent.getParent().getType();
            Type nowType = retValue.getType();
            if (nowType.isIntegerType() && returnType.isFloatType()) {
                retValue = I32toF(retValue, parent);
            } else if (nowType.isFloatType() && returnType.isIntegerType()) {
                retValue = FtoI32(retValue, parent);
            }
            this.getOperandList().add(retValue);
            this.addUseOp(retValue, 0);
        }

        this.getParent().addInstr(this);
    }

    /*
    return value == null 对应 return;
     */
    public String toString() {
        StringBuilder sb = new StringBuilder();
        if (this.getOperandList().size() == 0) {
            Type returnType = this.getParent().getParent().getType();
            if (returnType.isVoidType()) {
                sb.append("ret void");
            } else if (returnType.isIntegerType()) {
                sb.append("ret i32 0");
            } else if (returnType.isFloatType()) {
                sb.append("ret float 0x0");
            }
        } else {
            Value retValue = this.getOperandList().get(0);
            if (retValue != null) {
                sb.append("ret ").append(retValue.getType()).append(" ").append(retValue.getName());
            }
            else {
                sb.append("ret void");
            }
        }
        return sb.toString();
    }
}
