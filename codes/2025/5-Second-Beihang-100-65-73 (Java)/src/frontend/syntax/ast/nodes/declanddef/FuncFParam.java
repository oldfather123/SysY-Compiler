package frontend.syntax.ast.nodes.declanddef;

import frontend.syntax.ast.AST;
import frontend.syntax.ast.nodes.expression.IExp;

import java.util.List;

/**
 * 函数形参 FuncFParam → BType Ident ['[' ']']
 */
public class FuncFParam extends AST.Node {
    private final AST.BType type;
    private final String ident;
    private final List<IExp> limList;   // 为 null 表示并非数组，否则是数组，从第二维开始记录， empty 表示 1 维数组
    
    public FuncFParam(AST.BType type, String ident, List<IExp> limList, int lineno) {
        super(lineno);
        this.type = type;
        this.ident = ident;
        this.limList = limList;
    }
    
    public AST.BType getType() {
        return type;
    }
    
    public String getIdent() {
        return ident;
    }
    
    public List<IExp> getLimList() {
        return limList;
    }
}
