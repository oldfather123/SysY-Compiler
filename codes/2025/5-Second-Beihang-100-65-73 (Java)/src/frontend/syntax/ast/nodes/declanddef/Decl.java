package frontend.syntax.ast.nodes.declanddef;

import frontend.syntax.ast.AST;
import frontend.syntax.ast.nodes.block.IBlockItem;

import java.util.List;

/**
 * 声明 Decl → ConstDecl | VarDecl
 *  常量声明 ConstDecl → 'const' BType ConstDef { ',' ConstDef } ';'
 *  变量声明 VarDecl → BType VarDef { ',' VarDef } ';'
 */
public class Decl extends AST.Node implements IBlockItem {
    private final boolean constant;
    private final AST.BType bType;
    private final List<Def> defs;
    
    public Decl(boolean constant, AST.BType bType, List<Def> defs, int lineno) {
        super(lineno);
        this.constant = constant;
        this.bType = bType;
        this.defs = defs;
        if (bType == null) {
            throw new RuntimeException("类型不能为空");
        }
        if (this.defs == null || this.defs.isEmpty()) {
            throw new RuntimeException("至少要有一个定义");
        }
    }
    
    public AST.BType getbType() {
        return bType;
    }
    
    public boolean isConstant() {
        return constant;
    }
    
    public List<Def> getDefs() {
        return defs;
    }
}
