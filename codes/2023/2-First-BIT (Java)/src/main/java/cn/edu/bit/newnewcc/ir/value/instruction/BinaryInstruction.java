package cn.edu.bit.newnewcc.ir.value.instruction;

import cn.edu.bit.newnewcc.ir.Operand;
import cn.edu.bit.newnewcc.ir.Type;
import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.value.Instruction;

import java.util.ArrayList;
import java.util.List;

/**
 * 二元指令
 */
abstract public class BinaryInstruction extends Instruction {
    protected final Operand operand1, operand2;

    /**
     * 根据类型创建一个二元运算指令
     * @param resultType 二元运算的结果类型
     * @param operand1Type 操作数1的类型
     * @param operand2Type 操作数2的类型
     */
    protected BinaryInstruction(Type resultType, Type operand1Type, Type operand2Type){
        super(resultType);
        this.operand1 = new Operand(this,operand1Type,null);
        this.operand2 = new Operand(this,operand2Type,null);
    }

    public Value getOperand1() {
        return operand1.getValue();
    }

    public void setOperand1(Value value) {
        operand1.setValue(value);
    }

    public Value getOperand2() {
        return operand2.getValue();
    }

    public void setOperand2(Value value) {
        operand2.setValue(value);
    }

    /**
     * 获取指令的名称，其中可以包含空格。指令将被打印为：
     * <p>
     * 返回值名称 = 指令名称 操作数类型 操作数1, 操作数2;
     * <p>
     * 例如：
     * <ul>
     *     <li>%2 = add i32 %1, 1</li>
     *     <li>%4 = fcmp oeq float 3.0, %3</li>
     * </ul>
     *
     * @return 指令的名称
     */
    protected abstract String getInstName();

    @Override
    public List<Operand> getOperandList() {
        var list = new ArrayList<Operand>();
        list.add(operand1);
        list.add(operand2);
        return list;
    }
}
