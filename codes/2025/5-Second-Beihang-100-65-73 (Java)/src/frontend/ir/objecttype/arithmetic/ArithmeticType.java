package frontend.ir.objecttype.arithmetic;

import frontend.ir.Value;
import frontend.ir.objecttype.Type;
import frontend.syntax.ast.AST;

/**
 * 算术类型
 */
public abstract class ArithmeticType extends Type {
    public static ArithmeticType bType2AriType(AST.BType bType) {
        return switch (bType) {
            case INT   -> new TInt();
            case FLOAT -> new TFloat();
        };
    }
    
    public abstract Value getDefaultValue();
    public abstract int getByteSize();
}
