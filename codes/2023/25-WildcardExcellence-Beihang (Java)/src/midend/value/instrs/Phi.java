package midend.value.instrs;

import midend.Type;
import midend.value.BasicBlock;
import midend.value.Constant;
import midend.value.Value;

import java.util.ArrayList;

public class Phi extends Reg {
    public Phi(ArrayList<Value> values, BasicBlock parent) {
        super(parent);
        this.getOperandList().addAll(values);
        for (int i = 0; i < this.getOperandList().size(); i++) {
            this.addUseOp(this.getOperandList().get(i), i);
        }
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append(getName()).append(" = ").append("phi ").append(getType()).append(" ");
        Type curType = getType();
        int len = getOperandList().size();
        for (int i = 0; i < len; i++) {
            Value value = getOperandList().get(i);
            if (value == Constant.ConstantInteger.Constant0) {
                if (curType.isFloatType()) {
                    value = Constant.ConstantFloat.float0;
                }
            }
            sb.append("[ ").append(value.getName()).append(", %").append(this.getParent().getPrevBB().get(i).getName()).append(" ]");
            if (i < len - 1) {
                sb.append(", ");
            }
        }
        return sb.toString();
    }
}
