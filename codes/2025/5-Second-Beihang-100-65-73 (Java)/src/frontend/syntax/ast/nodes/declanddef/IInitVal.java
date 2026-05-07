package frontend.syntax.ast.nodes.declanddef;

/**
 * 变量初值 InitVal → Exp | '{' [ Exp { ',' Exp } ] '}' | StringConst
 * 常量初值 ConstInitVal → ConstExp | '{' [ ConstExp { ',' ConstExp } ] '}' | StringConst
 * 经过改写后包括 Exp | InitArray | StringConst
 */
public interface IInitVal {
}
