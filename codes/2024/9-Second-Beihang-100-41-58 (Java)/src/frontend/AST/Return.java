package frontend.AST;

//Return -> 'return' [Exp] ';'
public class Return implements Stmt {
    private Exp exp;

    public Return(Exp exp) {
        this.exp = exp;
    }

    public Exp getExp() {
        return this.exp;
    }
}
