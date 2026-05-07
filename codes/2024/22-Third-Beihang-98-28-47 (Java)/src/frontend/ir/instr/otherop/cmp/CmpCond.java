package frontend.ir.instr.otherop.cmp;

public enum CmpCond {
    EQ,
    NE,
    GT,
    GE,
    LT,
    LE;
    
    public CmpCond getReverse() {
        switch (this) {
            case EQ -> {
                return NE;
            }
            case NE -> {
                return EQ;
            }
            case GT -> {
                return LE;
            }
            case GE -> {
                return LT;
            }
            case LT -> {
                return GE;
            }
            case LE -> {
                return GT;
            }
            default -> {
                throw new RuntimeException("情况穷尽了");
            }
        }
    }
}
