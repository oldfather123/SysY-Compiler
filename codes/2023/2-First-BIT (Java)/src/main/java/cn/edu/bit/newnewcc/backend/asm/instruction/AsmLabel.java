package cn.edu.bit.newnewcc.backend.asm.instruction;

import cn.edu.bit.newnewcc.backend.asm.operand.Label;
import cn.edu.bit.newnewcc.backend.asm.operand.Register;

import java.util.Objects;
import java.util.Set;

/**
 * 标签也被视为一类指令
 */
public class AsmLabel extends AsmInstruction {
    private Label label;

    public AsmLabel(Label label) {
        super(null, null, null);
        this.label = Objects.requireNonNull(label);
    }

    public Label getLabel() {
        return label;
    }

    public void setLabel(Label label) {
        this.label = Objects.requireNonNull(label);
    }

    @Override
    public String toString() {
        return String.format("AsmLabel(%s)", getLabel());
    }

    @Override
    public String emit() {
        return String.format("%s:\n", getLabel().getLabelName());
    }

    @Override
    public Set<Register> getDef() {
        throw new UnsupportedOperationException();
    }

    @Override
    public Set<Register> getUse() {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean mayNotReturn() {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean willNeverReturn() {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean mayHaveSideEffects() {
        throw new UnsupportedOperationException();
    }
}
