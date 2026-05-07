package frontend.ir.instr.binop;

import frontend.ir.DataType;
import frontend.ir.Value;
import frontend.ir.constvalue.ConstInt;
import frontend.ir.instr.Instruction;
import frontend.ir.structure.Function;
import frontend.ir.structure.Procedure;

public class ShlInstr extends BinaryOperation {
    public ShlInstr(int result, Value op1, Value op2) {
        super(result, op1, op2, "shl", DataType.INT);
        assert op1.getDataType() == DataType.INT;
        assert op2.getDataType() == DataType.INT;
    }
    
    @Override
    public Instruction cloneShell(Procedure procedure) {
        return new ShlInstr(procedure.getAndAddRegIndex(), this.op1, this.op2);
    }
    
    @Override
    public Value operationSimplify() {
        if (op1 instanceof ConstInt && op2 instanceof ConstInt) {
            return new ConstInt(((ConstInt) op1).getNumber() << ((ConstInt) op2).getNumber());
        }
        return null;
    }
}
