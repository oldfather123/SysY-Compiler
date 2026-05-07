package frontend.syntax.ast.nodes.declanddef;

import frontend.syntax.ast.AST;
import frontend.syntax.ast.nodes.expression.BinaryExp;

import java.util.List;

/**
 *  常量定义 ConstDef → Ident { '[' ConstExp ']' } '=' ConstInitVal
 *  变量定义 VarDef   → Ident { '[' ConstExp ']' } | Ident { '[' ConstExp ']' } '=' InitVal
 */
public class Def extends AST.Node {
    private final String ident;
    private final List<BinaryExp> limitList;
    private final IInitVal initVal;
    
    public Def(boolean constant, String ident, List<BinaryExp> limitList, IInitVal initVal, int lineno) {
        super(lineno);
        this.ident = ident;
        this.limitList = limitList;
        this.initVal = initVal;
        if (ident == null) {
            throw new RuntimeException("标识符不能为空");
        }
        if (initVal == null && constant) {
            throw new RuntimeException("常量定义初始值不能为空");
        }
    }
    
    public String getIdent() {
        return ident;
    }
    
    public List<BinaryExp> getLimitList() {
        return limitList;
    }
    
    public IInitVal getInitVal() {
        return initVal;
    }
}
