package frontend.syntax.ast.nodes.block;

import frontend.syntax.ast.AST;
import frontend.syntax.ast.nodes.ASTStringConst;
import frontend.syntax.ast.nodes.expression.IExp;
import frontend.syntax.ast.nodes.expression.LVal;

import java.util.List;

/**
 * Stmt -> Assign | ExprStmt | Block | IfStmt | WhileStmt | Break | Continue | Return
 */
public interface IStmt extends IBlockItem {
    
    /**
     *  'if' '(' Cond ')' Stmt [ 'else' Stmt ]
     */
    class IfStmt extends AST.Node implements IStmt {
        private final IExp cond;
        private final IStmt thenStmt;
        private final IStmt elseStmt;
        
        public IfStmt(IExp cond, IStmt thenStmt, IStmt elseStmt, int lineno) {
            super(lineno);
            this.cond = cond;
            this.thenStmt = thenStmt;
            this.elseStmt = elseStmt;
        }
        
        public IExp getCond() {
            return cond;
        }
        
        public IStmt getThenStmt() {
            return thenStmt;
        }
        
        public IStmt getElseStmt() {
            return elseStmt;
        }
    }
    
    /**
     * 'while' '(' Cond ')' Stmt
     */
    class WhileLoopStmt extends AST.Node implements IStmt {
        private final IExp cond;
        private final IStmt body;
        
        public WhileLoopStmt(IExp cond, IStmt body, int lineno) {
            super(lineno);
            this.cond = cond;
            this.body = body;
        }
        
        
        public IExp getCond() {
            return cond;
        }
        
        public IStmt getBody() {
            return body;
        }
    }
    
    /**
     *  'return' [Exp] ';'
     */
    class ReturnStmt extends AST.Node implements IStmt {
        private final IExp retVal;
        
        public ReturnStmt(IExp retVal, int lineno) {
            super(lineno);
            this.retVal = retVal;
        }
        
        public IExp getRetVal() {
            return retVal;
        }
    }
    
    /**
     *  'break' ';'
     */
    class BreakStmt extends AST.Node implements IStmt {
        
        public BreakStmt(int lineno) {
            super(lineno);
        }
    }
    
    /**
     *  'continue' ';'
     */
    class ContinueStmt extends AST.Node implements IStmt {
        
        public ContinueStmt(int lineno) {
            super(lineno);
        }
    }
    
    /**
     *  LVal '=' Exp ';'
     *       LVal '=' 'getint''('')'';'
     *       LVal '=' 'getchar''('')'';'
     */
    class AssignStmt extends AST.Node implements IStmt {
        private final LVal lVal;
        private final IExp exp;
        
        public AssignStmt(LVal lVal, IExp exp, int lineno) {
            super(lineno);
            this.lVal = lVal;
            this.exp = exp;
        }
        
        public LVal getlVal() {
            return lVal;
        }
        
        public IExp getExp() {
            return exp;
        }
    }
    
    /**
     *  [Exp] ';'
     */
    class ExpStmt extends AST.Node implements IStmt {
        private final IExp exp; // 可以为空，也就是单独一个分号的语句
        
        public ExpStmt(IExp exp, int lineno) {
            super(lineno);
            this.exp = exp;
        }
        
        public IExp getExp() {
            return exp;
        }
    }
}
