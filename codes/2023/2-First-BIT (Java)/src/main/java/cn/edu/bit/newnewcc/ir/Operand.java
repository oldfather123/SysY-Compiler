package cn.edu.bit.newnewcc.ir;

import cn.edu.bit.newnewcc.ir.exception.OperandTypeMismatchException;
import cn.edu.bit.newnewcc.ir.value.Instruction;

/**
 * 指令的操作数
 */
public class Operand {

    /**
     * 使用该操作数的指令
     */
    protected final Instruction instruction;

    /**
     * 操作数类型的限制
     * <p>
     * 值为null表示不限制类型
     */
    protected final Type type;

    /**
     * 操作数所使用的值
     */
    protected Value value;

    /**
     * 构造一个操作数
     *
     * @param instruction 操作数所属的指令
     * @param type        操作数的类型限制，null表示无限制
     * @param value       操作数的初始值，null表示不绑定初始值
     */
    public Operand(Instruction instruction, Type type, Value value) {
        this.instruction = instruction;
        this.type = type;
        this.value = null;
        this.setValue(value);
    }

    /**
     * @return 操作数所属的指令
     */
    public Instruction getInstruction() {
        return instruction;
    }

    /**
     * 判断当前操作数是否绑定了值
     *
     * @return 若绑定了值，返回true；否则返回false
     */
    public boolean hasValueBound() {
        return value != null;
    }

    /**
     * @return 操作数的值
     */
    public Value getValue() {
        return value;
    }

    /**
     * @return 操作数的类型
     */
    public Type getType() {
        return type;
    }

    /**
     * 修改操作数的值
     *
     * @param value 操作数的新值
     */
    public void setValue(Value value) {
        if (value != null) {
            if (type != null && value.getType() != type) {
                throw new OperandTypeMismatchException();
            }
        }
        if (this.value != null) {
            this.value.__removeUsage__(this);
        }
        this.value = value;
        if (this.value != null) {
            this.value.__addUsage__(this);
        }
    }

    /**
     * 移除当前操作数绑定的值
     */
    public void removeValue() {
        this.setValue(null);
    }

}
