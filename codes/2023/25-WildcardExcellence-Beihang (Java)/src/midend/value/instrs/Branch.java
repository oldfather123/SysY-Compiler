package midend.value.instrs;

import midend.value.BasicBlock;
import midend.value.Constant;
import midend.value.Value;

public class Branch extends Instr {
    public Branch(Value cond, BasicBlock trueBB, BasicBlock falseBB, BasicBlock parent) {
        super(parent);
        this.getOperandList().add(cond);
        this.addUseOp(cond, 0);
        this.getOperandList().add(trueBB);
        this.addUseOp(trueBB, 1);
        this.getOperandList().add(falseBB);
        this.addUseOp(falseBB, 2);
    }

    public void isSingleJump() {
        Value cond = this.getOperandList().get(0);
        Value trueBB = this.getOperandList().get(1);
        Value falseBB = this.getOperandList().get(2);
        if (cond instanceof Constant.ConstantInteger) {
            int res = Integer.parseInt(cond.getName());
            Instr newJump;
            if (res == 1) { //新指令不会插入block，手动插入
                newJump = new Jump((BasicBlock) trueBB, this.getParent());
                this.getParent().removeSucBlock((BasicBlock) falseBB);
            } else {
                newJump = new Jump((BasicBlock) falseBB, this.getParent());
                this.getParent().removeSucBlock((BasicBlock) trueBB);
            }
            this.getParent().getInstrList().addLast(newJump);
            this.remove();
        }
    }

    public String toString() {
        Value cond = this.getOperandList().get(0);
        Value trueBB = this.getOperandList().get(1);
        Value falseBB = this.getOperandList().get(2);
        StringBuilder sb = new StringBuilder();
        sb.append("br ").append(cond.getType()).append(" ");
        sb.append(cond.getName()).append(", ");
        sb.append("label %").append(trueBB.getName()).append(", ");
        sb.append("label %").append(falseBB.getName());
        return sb.toString();
    }
}
