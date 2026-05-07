package frontend.syntax.ast.nodes.expression;

import frontend.syntax.ast.AST;

/**
 * 字符 Character → CharConst
 */
public class Character extends AST.Node implements IPrimaryExp {
    private final int ascii;
    
    public Character(int ascii, int lineno) {
        super(lineno);
        this.ascii = ascii;
    }
    
    public int getAscii() {
        return ascii;
    }
}
