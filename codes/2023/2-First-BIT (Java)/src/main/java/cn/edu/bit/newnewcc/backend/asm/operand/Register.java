package cn.edu.bit.newnewcc.backend.asm.operand;

public abstract class Register extends AsmOperand implements RegisterReplaceable {
    public abstract String getName();

    public abstract int getIndex();

    /**
     * 获取寄存器下标的**绝对值**
     */
    public int getAbsoluteIndex() {
        return Math.abs(getIndex());
    }

    public boolean isVirtual() {
        return getIndex() < 0;
    }

    @Override
    public Register getRegister() {
        return this;
    }

    @Override
    public Register withRegister(Register register) {
        return register;
    }
}
