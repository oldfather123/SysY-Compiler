package frontend.ir.symbols;

import frontend.ir.DataType;
import frontend.ir.Value;
import frontend.syntax.Ast;

public class InitExpr extends Value {
    private final Ast.Exp exp;
    
    public InitExpr(Ast.Exp exp) {
        super();
        this.exp = exp;
    }
    
    public Ast.Exp getExp() {
        return exp;
    }
    
    @Override
    public Number getNumber() {
        throw new RuntimeException("占位类，该方法不应该被调用");
    }
    
    @Override
    public DataType getDataType() {
        throw new RuntimeException("占位类，该方法不应该被调用");
    }
    
    @Override
    public String value2string() {
        throw new RuntimeException("占位类，该方法不应该被调用");
    }
}
