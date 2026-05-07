package frontend.ir.constant;

import frontend.ir.objecttype.arithmetic.TBool;

public class BoolConst extends IRConst {
    private final int constBool;
    public static final BoolConst Zero  = new BoolConst(0);
    public static final BoolConst One  = new BoolConst(1);
    
    public BoolConst(int constBool) {
        super(new TBool());
        if (constBool == 0) {
            this.constBool = 0;
        } else {
            this.constBool = 1;
        }
    }
    
    @Override
    public String value2string() {
        return String.valueOf(constBool);
    }
    
    @Override
    public String toString() {
        return "i1 " + constBool;
    }
    
    @Override
    public Number getConstVal() {
        return constBool;
    }

    @Override
    public BoolConst clone() {
        return new BoolConst(constBool);
    }
}
