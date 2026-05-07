package cn.edu.bit.newnewcc.backend.asm.instruction;

import cn.edu.bit.newnewcc.backend.asm.operand.*;
import cn.edu.bit.newnewcc.ir.exception.IllegalArgumentException;
import cn.edu.bit.newnewcc.ir.exception.IndexOutOfBoundsException;

import java.util.Objects;
import java.util.Set;

/**
 * 汇编指令基类
 */
public abstract class AsmInstruction {
    private AsmOperand operand1, operand2, operand3;

    protected AsmInstruction(AsmOperand operand1, AsmOperand operand2, AsmOperand operand3) {
        this.operand1 = operand1;
        this.operand2 = operand2;
        this.operand3 = operand3;
    }

    public void setOperand(int index, AsmOperand operand) {
        switch (index) {
            case 1 -> {
                if (operand1 == null) throw new IndexOutOfBoundsException();
                if (operand.getClass() != operand1.getClass()) throw new IllegalArgumentException();
                operand1 = Objects.requireNonNull(operand);;
            }
            case 2 -> {
                if (operand2 == null) throw new IndexOutOfBoundsException();
                if (operand.getClass() != operand2.getClass()) throw new IllegalArgumentException();
                operand2 = Objects.requireNonNull(operand);;
            }
            case 3 -> {
                if (operand3 == null) throw new IndexOutOfBoundsException();
                if (operand.getClass() != operand3.getClass()) throw new IllegalArgumentException();
                operand3 = Objects.requireNonNull(operand);;
            }
            default -> throw new IndexOutOfBoundsException();
        }
    }

    /**
     * 获取第index个参数
     * @param index 下标（1 <= index <= 3）
     * @return 参数值
     */
    public AsmOperand getOperand(int index) {
        return switch (index) {
            case 1 -> operand1;
            case 2 -> operand2;
            case 3 -> operand3;
            default -> null;
        };
    }

    public abstract String emit();

    public abstract Set<Register> getDef();

    public abstract Set<Register> getUse();

    public abstract boolean mayNotReturn();

    public abstract boolean willNeverReturn();

    public abstract boolean mayHaveSideEffects();
}
