package frontend.ir.constant;

import frontend.ir.objecttype.arithmetic.TFloat;

public class FloatConst extends IRConst {
    private final float constFloat;
    public static final FloatConst Zero = new FloatConst(0.0f);
    
    public FloatConst(float constFloat) {
        super(new TFloat());
        this.constFloat = constFloat;
    }

    @Override
    public Number getConstVal() {
        return constFloat;
    }
    
    @Override
    public String value2string() {
//        return "" + constFloat;
        return "0x" + Long.toHexString(Double.doubleToLongBits(constFloat));
    }
    
    @Override
    public String toString() {
        return "float " + this.value2string();
    }

    @Override
    public FloatConst clone() {
        return new FloatConst(constFloat);
    }
}
