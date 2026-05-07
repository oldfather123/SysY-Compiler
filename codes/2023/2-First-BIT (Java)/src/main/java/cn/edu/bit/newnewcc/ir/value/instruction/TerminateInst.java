package cn.edu.bit.newnewcc.ir.value.instruction;

import cn.edu.bit.newnewcc.ir.Operand;
import cn.edu.bit.newnewcc.ir.Type;
import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.type.VoidType;
import cn.edu.bit.newnewcc.ir.value.BasicBlock;
import cn.edu.bit.newnewcc.ir.value.Instruction;

import java.util.Collection;

/**
 * 基本块终止语句
 */
public abstract class TerminateInst extends Instruction {

    protected class ExitOperand extends Operand {
        public ExitOperand(TerminateInst instruction, Type type, Value value) {
            super(instruction, type, value);
        }

        @Override
        public void setValue(Value value) {
            if (instruction.getBasicBlock() != null && this.value != null) {
                ((BasicBlock) this.value).__removeEntryBlock__(instruction.getBasicBlock());
            }
            super.setValue(value);
            if (instruction.getBasicBlock() != null && this.value != null) {
                ((BasicBlock) this.value).__addEntryBlock__(instruction.getBasicBlock());
            }
        }

    }

    public TerminateInst() {
        super(VoidType.getInstance());
    }

    @Override
    public void removeFromBasicBlock() {
        getBasicBlock().setTerminateInstruction(null);
    }

    @Override
    public VoidType getType() {
        return (VoidType) super.getType();
    }

    @Override
    public String getValueName() {
        throw new UnsupportedOperationException();
    }

    /**
     * @return 该语句可能跳转到的所有基本块
     */
    public abstract Collection<BasicBlock> getExits();
}
