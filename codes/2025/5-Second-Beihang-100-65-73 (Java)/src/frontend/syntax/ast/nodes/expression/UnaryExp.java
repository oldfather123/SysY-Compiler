package frontend.syntax.ast.nodes.expression;

import frontend.ir.symbol.SymTab;
import frontend.syntax.ast.AST;

import java.util.List;

/**
 * 一元表达式 UnaryExp → PrimaryExp | Ident '(' [FuncRParams] ')' | UnaryOp UnaryExp
 * 其中第二种可能性可以称为 call，直接归入第一种
 * 化简后即 UnaryExp → {UnaryOp} PrimaryExp
 *  基本表达式 PrimaryExp → '(' Exp ')' | LVal | Number | Character
 *      增加后：PrimaryExp → '(' Exp ')' | LVal | Number | Character | call
 */
public class UnaryExp extends AST.Node implements IExp {
    private final IPrimaryExp primaryExp;
    private final int sign;
    private final boolean finalNot;
    private final boolean existsNot;
    
    public UnaryExp(List<AST.OperatorName> ops, IPrimaryExp primaryExp, int lineno) {
        super(lineno);
        this.primaryExp = primaryExp;
        this.sign = initSign(ops);
        this.finalNot = initNot(ops);
        this.existsNot = ops.contains(AST.OperatorName.NOT);
    }
    
    private int initSign(List<AST.OperatorName> ops) {
        assert ops != null;
        int sign = 1;
        for (AST.OperatorName op : ops) {
            if (op == AST.OperatorName.SUB) {
                sign *= -1;
            } else if (op == AST.OperatorName.NOT) {
                break;
            }
        }
        return sign;
    }
    
    private boolean initNot(List<AST.OperatorName> ops) {
        boolean not = false;
        for (AST.OperatorName op : ops) {
            if (op == AST.OperatorName.NOT) {
                not = !not;
            }
        }
        return not;
    }
    
    public IPrimaryExp getPrimaryExp() {
        return primaryExp;
    }
    
    public int getSign() {
        return sign;
    }
    
    public boolean checkFinalNot() {
        return finalNot;
    }
    
    public boolean checkExistingNot() {
        return existsNot;
    }
}
