package cn.edu.bit.newnewcc.ir.value.instruction;

import cn.edu.bit.newnewcc.ir.Operand;
import cn.edu.bit.newnewcc.ir.type.DummyType;
import cn.edu.bit.newnewcc.ir.value.Instruction;

import java.util.List;

public class DummyInstruction extends Instruction {
    public DummyInstruction() {
        super(DummyType.getInstance());
    }

    @Override
    public List<Operand> getOperandList() {
        throw new UnsupportedOperationException();
    }

    @Override
    public void emitIr(StringBuilder builder) {
        throw new UnsupportedOperationException();
    }
}
