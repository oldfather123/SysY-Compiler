package midend.value.instrs;

import midend.value.BasicBlock;
import midend.value.Value;

public class Jump extends Instr {
    public Jump(BasicBlock targetBB, BasicBlock parent) {
        super(parent);
        this.getOperandList().add(targetBB);
        this.addUseOp(targetBB, 0);
    }

    public String toString() {
        Value targetBB = this.getOperandList().get(0);
        return"br label %" + targetBB.getName();
    }
}
