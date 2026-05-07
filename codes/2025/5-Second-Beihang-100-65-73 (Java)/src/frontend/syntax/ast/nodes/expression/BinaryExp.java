package frontend.syntax.ast.nodes.expression;

import frontend.syntax.ast.AST;

import java.util.List;

/**
 * 原本不存在的抽象层次，所有双目运算符连结的表达式，包括算术、关系、逻辑
 *  表达式 Exp → AddExp
 *  常量表达式 ConstExp → AddExp 注：使用的 Ident 必须是常量
 *  条件表达式 Cond → LOrExp
 *      逻辑或表达式 LOrExp → LAndExp | LOrExp '||' LAndExp
 *      逻辑与表达式 LAndExp → EqExp | LAndExp '&&' EqExp
 *      相等性表达式 EqExp → RelExp | EqExp ('==' | '!=') RelExp
 *      关系表达式 RelExp → AddExp | RelExp ('<' | '>' | '<=' | '>=') AddExp
 *      加减表达式 AddExp → MulExp | AddExp ('+' | '−') MulExp
 *      乘除模表达式 MulExp → UnaryExp | MulExp ('*' | '/' | '%') UnaryExp
 */
public class BinaryExp extends AST.Node implements IExp {
    private final IExp firstExp;
    private final List<AST.OperatorName> ops;
    private final List<IExp> restExps;
    
    public BinaryExp(IExp firstExp, List<AST.OperatorName> ops, List<IExp> restExps, int lineno) {
        super(lineno);
        this.firstExp = firstExp;
        this.ops = ops;
        this.restExps = restExps;
    }
    
    public IExp getFirstExp() {
        return firstExp;
    }
    
    public List<AST.OperatorName> getOps() {
        return ops;
    }
    
    public List<IExp> getRestExps() {
        return restExps;
    }
}
