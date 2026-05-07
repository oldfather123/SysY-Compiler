package frontend.ir.instr.binop;

import frontend.ir.Value;
import frontend.ir.constant.IntConstHandler;
import frontend.ir.objecttype.arithmetic.TInt;
import frontend.ir.structure.BasicBlock;

public class AddInstr extends BinaryOperation {
    public AddInstr(int regIdx, Value op1, Value op2, BasicBlock parentBB) {
        super(new TInt(), regIdx, op1, op2, "add", parentBB);
    }
    
    @Override
    protected boolean isSwappable() {
        return true;
    }
    
    @Override
    public Value simplify() {
        if (isRemoved) {
            return this;
        }
        return this.simplifyForAdd(new IntConstHandler(), AddInstr.class, SubInstr.class);
    }
}
