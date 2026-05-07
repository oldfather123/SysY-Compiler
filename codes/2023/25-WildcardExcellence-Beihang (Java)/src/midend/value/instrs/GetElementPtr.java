package midend.value.instrs;

import midend.Type;
import midend.value.BasicBlock;
import midend.value.Value;

import java.util.ArrayList;

public class GetElementPtr extends Loc {

    public Value getPointer() {
        return this.getOperandList().get(this.getOperandList().size() - 1);
    }

    public GetElementPtr(Value pointer, ArrayList<Value> offsets, BasicBlock parent) {
        super(parent);
        this.getOperandList().addAll(offsets);
        this.getOperandList().add(pointer); // 将pointer加入operandList
        int size = offsets.size();
        Type temp = pointer.getType();
        for (int i = 0; i < size; i++) {
            temp = temp.getElementType();
        }
        for (int i = 0; i < this.getOperandList().size(); i++) {
            this.addUseOp(this.getOperandList().get(i), i);
        }
        setType(new Type.PointerType(temp));
    }

    public ArrayList<Value> getOffsets() {
        ArrayList<Value> offsets = new ArrayList<>(getOperandList());
        offsets.remove(offsets.size() - 1);
        return offsets;
    }

    public String getHash() {
        StringBuilder sb = new StringBuilder();
        ArrayList<Value> offsets = getOffsets();
        Value pointer = getPointer();
        sb.append("getelementptr inbounds ");
        sb.append(pointer.getType().getElementType()).append(", ");
        sb.append(pointer.getType()).append(" ").append(pointer.getName()).append(", ");
        int size = offsets.size();
        for (int i = 0; i < size; i++) {
            if (i != 0) {
                sb.append(", ");
            }
            sb.append(offsets.get(i).getType()).append(" ").append(offsets.get(i).getName());
        }
        return sb.toString();
    }

    public String toString() {
        return getName() + " = " + getHash();
    }
}
