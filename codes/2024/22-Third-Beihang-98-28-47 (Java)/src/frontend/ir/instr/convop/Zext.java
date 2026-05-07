package frontend.ir.instr.convop;

import frontend.ir.DataType;
import frontend.ir.Value;
import frontend.ir.constvalue.ConstBool;
import frontend.ir.constvalue.ConstInt;
import frontend.ir.instr.Instruction;
import frontend.ir.structure.Function;
import frontend.ir.structure.Procedure;

/**
 * 目前认为这个指令就是用来从 i1 拓展到 i32 的。
 */
public class Zext extends ConversionOperation {
    public Zext(int result, Value value) {
        super(result, DataType.BOOL, DataType.INT, value, "zext");
    }
    
    @Override
    public Instruction cloneShell(Procedure procedure) {
        return new Zext(procedure.getAndAddRegIndex(), this.value);
    }
    
    @Override
    public Value operationSimplify() {
        if (this.value instanceof ConstBool) {
            return new ConstInt(value.getNumber().intValue());
        }
        return null;
    }
}
