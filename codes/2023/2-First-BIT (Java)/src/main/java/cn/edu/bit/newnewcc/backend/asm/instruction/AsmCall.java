package cn.edu.bit.newnewcc.backend.asm.instruction;

import cn.edu.bit.newnewcc.backend.asm.operand.Label;
import cn.edu.bit.newnewcc.backend.asm.operand.Register;
import cn.edu.bit.newnewcc.backend.asm.util.Registers;

import java.util.List;
import java.util.Objects;
import java.util.Set;

public class AsmCall extends AsmInstruction {
    private List<Register> paramRegList;
    private Register returnRegister;

    public AsmCall(Label label, List<Register> paramRegList, Register returnRegister) {
        super(Objects.requireNonNull(label), null, null);
        this.paramRegList = Objects.requireNonNull(paramRegList);
        this.returnRegister = returnRegister;
    }

    public List<Register> getParamRegList() {
        return paramRegList;
    }

    public Register getReturnRegister() {
        return returnRegister;
    }

    public void setParamRegList(List<Register> paramRegList) {
        this.paramRegList = Objects.requireNonNull(paramRegList);
    }

    public void setReturnRegister(Register returnRegister) {
        this.returnRegister = returnRegister;
    }

    @Override
    public String toString() {
        if (getReturnRegister() != null) {
            return String.format("AsmCall(%s, %s->%s)", getOperand(1), getParamRegList(), getReturnRegister());
        } else {
            return String.format("AsmCall(%s, %s)", getOperand(1), getParamRegList());
        }
    }

    @Override
    public String emit() {
        return String.format("\tcall %s\n", getOperand(1).emit());
    }

    @Override
    public Set<Register> getDef() {
        return Registers.CALLER_SAVED_REGISTERS;
    }

    @Override
    public Set<Register> getUse() {
        return Set.copyOf(getParamRegList());
    }

    @Override
    public boolean mayNotReturn() {
        return false;
    }

    @Override
    public boolean willNeverReturn() {
        return false;
    }

    @Override
    public boolean mayHaveSideEffects() {
        return true;
    }
}
