package frontend.AST;

//If -> 'if' '(' Cond ')' Stmt [ Else ]
//Cond -> Exp
//Else -> 'else' Stmt
public class If implements Stmt {
    private Exp condition;
    private Stmt thenStatement;
    private boolean hasElse;
    private Stmt elseStatement;

    public If(Exp condition, Stmt thenStatement, boolean hasElse, Stmt elseStatement) {
        this.condition = condition;
        this.thenStatement = thenStatement;
        this.hasElse = hasElse;
        this.elseStatement = elseStatement;
    }

    public Exp getCond() {
        return condition;
    }

    public Stmt getIfStmt() {
        return thenStatement;
    }

    public boolean getHaveElse() {
        return hasElse;
    }

    public Stmt getElseStmt() {
        return elseStatement;
    }
}

