package cn.edu.bit.newnewcc.backend.asm.operand;

public abstract class AsmOperand {

    public abstract String emit();

    @Override
    public String toString() {
        return emit();
    }
}
