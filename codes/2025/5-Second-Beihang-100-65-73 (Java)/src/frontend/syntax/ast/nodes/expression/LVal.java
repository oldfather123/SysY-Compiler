package frontend.syntax.ast.nodes.expression;

import frontend.syntax.ast.AST;

import java.util.List;

/**
 * 左值表达式 LVal → Ident {'[' Exp ']'}
 */
public class LVal extends AST.Node implements IPrimaryExp {
    private final String ident;
    private final List<IExp> indexList;
    
    public LVal(String ident, List<IExp> indexList, int lineno) {
        super(lineno);
        this.ident = ident;
        this.indexList = indexList;
    }
    
    public String getIdent() {
        return ident;
    }
    
    public List<IExp> getIndexList() {
        return indexList;
    }
}
