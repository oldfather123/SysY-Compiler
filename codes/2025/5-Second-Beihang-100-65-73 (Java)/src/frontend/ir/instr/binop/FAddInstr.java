package frontend.ir.instr.binop;

import frontend.ir.Value;
import frontend.ir.constant.FloatConstHandler;
import frontend.ir.objecttype.arithmetic.TFloat;
import frontend.ir.structure.BasicBlock;


public class FAddInstr extends BinaryOperation {
    public FAddInstr(int regIdx, Value op1, Value op2, BasicBlock parentBB) {
        super(new TFloat(), regIdx, op1, op2, "fadd", parentBB);
    }
    
    @Override
    public Value simplify() {
        if (isRemoved) {
            return this;
        }
        return this.simplifyForAdd(new FloatConstHandler(), FAddInstr.class, FSubInstr.class);
    }

@Override
    protected boolean isSwappable() {
        return true;
    }
}
