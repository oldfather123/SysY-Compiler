package frontend.ir.constant;

import frontend.ir.objecttype.TVoid;
import frontend.ir.objecttype.derived.TPointer;

public class PointerConst extends IRConst {
    private final int constPos;
    public static final PointerConst NULL = new PointerConst(0);

    public PointerConst(int constPos) {
        super(new TPointer(new TVoid()));
        this.constPos = constPos;
    }

    @Override
    public String value2string() {
        return String.valueOf(constPos);
    }

    @Override
    public Number getConstVal() {
        throw new RuntimeException("PointerConst cannot be used as constant");
    }

    @Override
    public IRConst clone() {
        return new PointerConst(constPos);
    }
}