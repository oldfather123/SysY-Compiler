package cn.edu.nju.software.ir.instruction;

import cn.edu.nju.software.ir.generator.InstructionVisitor;
import cn.edu.nju.software.ir.value.ValueRef;

public class IntToFloat extends Instruction {
    public IntToFloat(ValueRef lVal, ValueRef intVal) {
        this.lVal = lVal;
        operator = "fptosi";
        operands = new ValueRef[]{intVal};
        intVal.addUser(this);
    }

    @Override
    public String toString() {
        return lVal + " = sitofp i32 " + operands[0] + " to float";
    }

    @Override
    public void accept(InstructionVisitor visitor) {
        visitor.visit(this);
    }

}
