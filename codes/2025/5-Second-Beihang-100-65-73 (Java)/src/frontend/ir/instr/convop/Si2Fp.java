package frontend.ir.instr.convop;

import frontend.ir.Value;
import frontend.ir.objecttype.arithmetic.TFloat;
import frontend.ir.structure.BasicBlock;

public class Si2Fp extends ConversionOperation {
    public Si2Fp(int regIdx, Value value, BasicBlock parentBB) {
        super(regIdx, new TFloat(), value, "sitofp", parentBB);
    }
}
