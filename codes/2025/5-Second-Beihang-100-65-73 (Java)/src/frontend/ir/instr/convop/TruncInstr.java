package frontend.ir.instr.convop;

import frontend.ir.Value;
import frontend.ir.objecttype.arithmetic.TChar;
import frontend.ir.structure.BasicBlock;

/**
 * 截断操作，用来将 int 截成 char
 */
public class TruncInstr extends ConversionOperation {
    public TruncInstr(int regIdx, Value value, BasicBlock parentBB) {
        super(regIdx, new TChar(), value, "trunc", parentBB);
    }
}
