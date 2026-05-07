package frontend.syntax.ast.nodes.expression;

import frontend.syntax.ast.AST;

/**
 * 数值 ASTNumber → IntNumber | FloatNumber
 */
public abstract class ASTNumber extends AST.Node implements IPrimaryExp {
    private final Number constNumber;    // 这里放的是整数的无符号部分，可能出现 0xFFFF_FFFF 这种整型放不下的东西
    
    public ASTNumber(Number constNumber, int lineno) {
        super(lineno);
        this.constNumber = constNumber;
    }
    
    protected Number getConstNumber() {
        return this.constNumber;
    }
    
    public static class IntNumber extends ASTNumber {
        public IntNumber(long constInt, int lineno) {
            super(constInt, lineno);
        }
        
        @Override
        public Long getConstNumber() {
            return (Long) super.getConstNumber();
        }
    }
    
    public static class FloatNumber extends ASTNumber {
        public FloatNumber(float constFloat, int lineno) {
            super(constFloat, lineno);
        }
        
        @Override
        public Float getConstNumber() {
            return (Float) super.getConstNumber();
        }
    }
}
