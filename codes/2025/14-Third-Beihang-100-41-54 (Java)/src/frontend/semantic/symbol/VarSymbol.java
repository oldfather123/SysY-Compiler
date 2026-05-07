package frontend.semantic.symbol;

import backend.ir.entity.LiteralConst;

/**
 * &#064;Classname VarSymbol
 * &#064;Description  TODO
 * &#064;Date 2025/7/13 15:12
 * &#064;Created MuJue
 */
public class VarSymbol extends Symbol{
    private LiteralConst constInit = null;

    public boolean isConstInit(){return constInit != null;}

    public VarSymbol(String name, IRBasicType type, Boolean isConst, Boolean isGlobal) {
        super(name, type, isGlobal, isConst);
    }
    public LiteralConst getConstInit() {
        return constInit;
    }

    public void setConstInit(LiteralConst constInit) {
        this.constInit = constInit;
    }
}
