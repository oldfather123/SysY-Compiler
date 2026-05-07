package frontend.syntax.ast.nodes.declanddef;

import frontend.syntax.ast.AST;
import frontend.syntax.ast.nodes.block.Block;

import java.util.List;

/**
 * FuncDef -> FuncType Ident '(' [FuncFParams] ')' Block
 *  FuncFParams -> FuncFParam {',' FuncFParam}
 * 特殊的函数定义——主函数：
 *  主函数定义 MainFuncDef → 'int' 'main' '(' ')' Block
 */
public class FuncDef extends AST.Node {
    private final AST.FuncType type;
    private final String ident;
    private final List<FuncFParam> fParams;
    private final Block body;
    private final int endLineno;
    
    public FuncDef(AST.FuncType type, String ident, List<FuncFParam> fParams, Block body, int lineno, int endLineno) {
        super(lineno);
        this.type = type;
        this.ident = ident;
        this.fParams = fParams;
        this.body = body;
        this.endLineno = endLineno;
    }
    
    public AST.FuncType getType() {
        return type;
    }
    
    public String getIdent() {
        return ident;
    }
    
    public List<FuncFParam> getFParams() {
        return fParams;
    }
    
    public Block getBody() {
        return body;
    }
    
    public int getEndLineno() {
        return endLineno;
    }
}
