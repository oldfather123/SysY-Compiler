package frontend.ir.symbol;

import frontend.ir.Value;
import frontend.ir.objecttype.Type;

public class Symbol {
    private final String ident;
    private final boolean constant;
    private final boolean global;
    private final Type type;
    private final Value initVal;
    private final Value pointer;
    private boolean abandoned;
    
    public Symbol(String ident, boolean isConst, boolean isGlobal, Type type, Value initVal, Value pointer) {
        this.ident = ident;
        this.constant = isConst;
        this.global = isGlobal;
        this.type = type;
        this.initVal = initVal;
        this.pointer = pointer;
        this.abandoned = false;
    }
    
    public boolean isConst() {
        return constant;
    }
    
    public boolean isGlobal() {
        return global;
    }
    
    public Value getInitVal() {
        return initVal;
    }
    
    /**
     * 用来获取存储这个对象的地址的指针
     */
    public Value getPointer() {
        return pointer;
    }
    
    public Type getType() {
        return type;
    }
    
    public String getIdent() {
        return ident;
    }
    
    public void abandon() {
        this.abandoned = true;
    }
    
    public boolean isAbandoned() {
        return abandoned;
    }
}
