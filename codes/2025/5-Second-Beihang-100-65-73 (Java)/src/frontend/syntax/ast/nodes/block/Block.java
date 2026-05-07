package frontend.syntax.ast.nodes.block;

import frontend.syntax.ast.AST;

import java.util.List;

/**
 * 语句块 Block → '{' { BlockItem } '}'
 *  语句块项 BlockItem → Decl | Stmt
 */
public class Block extends AST.Node implements IStmt {
    private final List<IBlockItem> blockItems;
    
    public Block(List<IBlockItem> blockItems, int lineno) {
        super(lineno);
        this.blockItems = blockItems;
    }
    
    public boolean endsWithReturn() {
        return !this.blockItems.isEmpty() && this.blockItems.get(blockItems.size() - 1) instanceof ReturnStmt;
    }
    
    public List<IBlockItem> getBlockItems() {
        return blockItems;
    }
}
