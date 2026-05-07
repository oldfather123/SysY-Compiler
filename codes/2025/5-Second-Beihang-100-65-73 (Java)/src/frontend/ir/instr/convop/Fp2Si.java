package frontend.ir.instr.convop;

import frontend.ir.Value;
import frontend.ir.objecttype.arithmetic.TInt;
import frontend.ir.structure.BasicBlock;

public class Fp2Si extends ConversionOperation {
    public Fp2Si(int regIdx, Value value, BasicBlock parentBB) {
        super(regIdx, new TInt(), value, "fptosi", parentBB);
    }
}
