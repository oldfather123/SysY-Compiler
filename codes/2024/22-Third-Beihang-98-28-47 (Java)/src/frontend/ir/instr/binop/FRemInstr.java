package frontend.ir.instr.binop;

import frontend.ir.DataType;
import frontend.ir.Value;
import frontend.ir.constvalue.ConstFloat;
import frontend.ir.instr.Instruction;
import frontend.ir.structure.Function;
import frontend.ir.structure.Procedure;

public class FRemInstr extends BinaryOperation {
    public FRemInstr(int result, Value op1, Value op2) {
        super(result, op1, op2, "frem", DataType.FLOAT);
        assert op1.getDataType() == DataType.FLOAT;
        assert op2.getDataType() == DataType.FLOAT;
    }
    
    @Override
    public Instruction cloneShell(Procedure procedure) {
        return new FRemInstr(procedure.getAndAddRegIndex(), this.op1, this.op2);
    }
    
    @Override
    public Value operationSimplify() {
        if (op1 instanceof ConstFloat && op2 instanceof ConstFloat) {
            return new ConstFloat(((ConstFloat) op1).getNumber() % ((ConstFloat) op2).getNumber());
        }
        return null;
    }
}
