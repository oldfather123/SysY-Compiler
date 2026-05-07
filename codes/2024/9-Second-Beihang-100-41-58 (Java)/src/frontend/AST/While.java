package frontend.AST;

//While -> 'while' '(' Cond ')' Stmt
public class While implements Stmt {
    private Stmt body;
    private Exp condition;


    public While(Exp condition, Stmt body) {
        this.condition = condition;
        this.body = body;
    }

    public Exp getCond() {
        return condition;
    }

    public Stmt getWhileStmt() {
        return body;
    }
}

