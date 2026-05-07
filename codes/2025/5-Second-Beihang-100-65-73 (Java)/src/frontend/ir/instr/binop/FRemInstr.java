package frontend.ir.instr.binop;

import frontend.ir.Value;
import frontend.ir.objecttype.arithmetic.TFloat;
import frontend.ir.structure.BasicBlock;

public class FRemInstr extends BinaryOperation {
    public FRemInstr(int regIdx, Value op1, Value op2, BasicBlock parentBB) {
        super(new TFloat(), regIdx, op1, op2, "srem", parentBB);
    }
    
    @Override
    public Value simplify() {
        if (isRemoved) {
            return this;
        }
        return this;
    }
    
    @Override
    protected boolean isSwappable() {
        return false;
    }
}
