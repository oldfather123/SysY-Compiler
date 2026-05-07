package frontend.syntax.ast.nodes.expression;

import frontend.syntax.ast.nodes.declanddef.IInitVal;

/**
 * 修改过的抽象层次，详见 BinaryExp
 * PrimaryExp -> Call | '(' Exp ')' | LVal | Number
 * Init -> Exp | InitArray
 * Expr -> BinaryExp | UnaryExp
 */
public interface IExp extends IInitVal, IPrimaryExp {
}
