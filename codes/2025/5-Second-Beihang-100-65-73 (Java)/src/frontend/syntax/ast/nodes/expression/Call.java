package frontend.syntax.ast.nodes.expression;

import frontend.syntax.ast.AST;

import java.util.List;

/**
 * 原本不存在的抽象层次，详见 UnaryExp
 * Call -> Ident '(' [FuncRParams] ')'
 *  函数实参表 FuncRParams → Exp { ',' Exp }
 */
public class Call extends AST.Node implements IPrimaryExp {
    private final String ident;
    private final List<BinaryExp> rParams;
    
    public Call(String ident, List<BinaryExp> rParams, int lineno) {
        super(lineno);
        this.ident = ident;
        this.rParams = rParams;
    }
    
    public String getIdent() {
        return ident;
    }
    
    public List<BinaryExp> getRParams() {
        return rParams;
    }
}
