package frontend.AST;

//Assign -> LVal '=' Exp ';'
public class Assign implements Stmt {
    private Lval lval;
    private Exp exp;

    public Assign(Lval lval, Exp exp) {
        this.lval = lval;
        this.exp = exp;
    }

    public Lval getLval() {
        return this.lval;
    }

    public Exp getExp() {
        return this.exp;
    }
}
