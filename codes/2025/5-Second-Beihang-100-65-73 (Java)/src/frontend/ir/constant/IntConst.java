package frontend.ir.constant;

import frontend.ir.objecttype.arithmetic.TInt;

public class IntConst extends IRConst {
    private final long constInt;
    public static final IntConst Zero = new IntConst(0);
    
    public IntConst(long constInt) {
        super(new TInt());
        this.constInt = constInt;
    }
    
    public Long getConstInt() {
        return constInt;
    }
    
    @Override
    public String value2string() {
        return String.valueOf(constInt);
    }
    
    @Override
    public String toString() {
        return "i32 " + constInt;
    }
    
    @Override
    public Number getConstVal() {
        return constInt;
    }

    @Override
    public IntConst clone() {
        return new IntConst(constInt);
    }
}
