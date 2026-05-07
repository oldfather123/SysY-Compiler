package frontend.ir.instr.convop;

import frontend.ir.Value;
import frontend.ir.objecttype.Type;
import frontend.ir.objecttype.arithmetic.TChar;
import frontend.ir.objecttype.derived.TPointer;
import frontend.ir.structure.BasicBlock;

/**
 * 用于将整数、浮点数的（数组）指针转化为 i8*，用于 memset
 */
public class Bitcast extends ConversionOperation {
    public Bitcast(int regIdx, Value value, BasicBlock parentBB) {
        super(regIdx, new TPointer(new TChar()), value, "bitcast", parentBB);
    }

    public Bitcast(int regIdx, Value value, Type targetType, BasicBlock parentBB) {
        super(regIdx, targetType, value, "bitcast", parentBB);
    }
}
