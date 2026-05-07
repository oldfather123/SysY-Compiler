package IR.Value.Instructions;

import IR.Type.IntegerType;
import IR.Value.BasicBlock;
import IR.Value.Value;

public class CmpInst extends BinaryInst{

    public CmpInst(OP op, Value left, Value right) {
        super(op, left, right, IntegerType.I1);
    }
}
