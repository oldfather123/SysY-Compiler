package frontend.ir.instr.convop;

import frontend.ir.Value;
import frontend.ir.objecttype.arithmetic.TInt;
import frontend.ir.structure.BasicBlock;

/**
 * 0 拓展，用来将 char 或 bool 转化为 int
 * 值得注意的是，BUAA_SysY 奇妙的文法规定 char 是无符号整数，因此可以直接用 zext
 */
public class ZextInstr extends ConversionOperation {
    public ZextInstr(int regIdx, Value value, BasicBlock parentBB) {
        super(regIdx, new TInt(), value, "zext", parentBB);
    }
}
