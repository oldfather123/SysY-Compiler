package frontend.ir.instr.binop;

import frontend.ir.Value;
import frontend.ir.constant.FloatConstHandler;
import frontend.ir.objecttype.arithmetic.TFloat;
import frontend.ir.structure.BasicBlock;

public class FSubInstr extends BinaryOperation {
    public FSubInstr(int regIdx, Value op1, Value op2, BasicBlock parentBB) {
        super(new TFloat(), regIdx, op1, op2, "fsub", parentBB);
    }
    
    @Override
    public Value simplify() {
        if (isRemoved) {
            return this;
        }
        return this.simplifyForSub(new FloatConstHandler(), FAddInstr.class, FSubInstr.class);
    }
    
    @Override
    protected boolean isSwappable() {
        return false;
    }
}
