package frontend.ir.instr.binop;

import frontend.ir.DataType;
import frontend.ir.Value;
import frontend.ir.instr.Instruction;

/**
 * 实际上包括了常规运算和位运算
 */
public abstract class BinaryOperation extends Instruction {
    private final int result; // 新分配一个寄存器用来存结果
    protected Value op1;
    protected Value op2;
    private final DataType type;
    private final String operationName;
    
    public BinaryOperation(int result, Value op1, Value op2, String operationName, DataType type) {
        this.result = result;
        this.op1 = op1;
        this.op2 = op2;
        this.type = type;
        this.operationName = operationName;
        setUse(op1);
        setUse(op2);
    }

    public String getOperationName() {
        return operationName;
    }

    @Override
    public Integer getNumber() {
        return result;
    }
    
    @Override
    public DataType getDataType() {
        return type;
    }
    
    @Override
    public String print() {
        return "%reg_" + result + " = " +
                operationName + " " + type + " " +
                op1.value2string() + ", " + op2.value2string();
    }

    @Override
    public void modifyValue(Value from, Value to) {
        if (op1 == from) {
            op1 = to;
            return;
        }
        if (op2 == from) {
            op2 = to;
            return;
        }
        throw new RuntimeException();
    }

    public Value getOp1() {
        return op1;
    }
    
    public Value getOp2() {
        return op2;
    }
    
    public void setOp1(Value op1) {
        this.op1 = op1;
    }
    
    public void setOp2(Value op2) {
        this.op2 = op2;
    }
    
    @Override
    public String myHash() {
        if (this instanceof Swappable) {
            boolean bothIns = op1 instanceof Instruction && op2 instanceof Instruction;
            boolean outOfOrder = op1.value2string().compareTo(op2.value2string()) > 0;
            if (bothIns && outOfOrder) {
                return op2.value2string() + op1.value2string() + operationName;
            }
        }
        return op1.value2string() + op2.value2string() + operationName;
    }
}
