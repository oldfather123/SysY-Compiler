package cn.edu.bit.newnewcc.backend.asm.operand;

/**
 * 栈偏移量，专用于栈帧重新分配的过程.
 */
public class ExStackVarOffset extends Immediate implements ExStackVarAdd {
    ExStackVarOffset(int val) {
        super(val);
    }
    public static ExStackVarOffset transform(long value) {
        return new ExStackVarOffset(Math.toIntExact(value));
    }

    @Override
    public AsmOperand add(int diff) {
        if (getValue() < 0) {
            return new ExStackVarOffset(getValue() + diff);
        } else {
            return this;
        }
    }
}
