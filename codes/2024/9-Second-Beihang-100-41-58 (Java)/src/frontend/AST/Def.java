package frontend.AST;

import java.util.ArrayList;

//Def -> Ident { '[' Exp ']' } '=' InitVal
public class Def {
    private String ident;
    private ArrayList<Exp> array;
    private InitVal initVal;

    public Def(String ident, ArrayList<Exp> array, InitVal initVal) {
        this.ident = ident;
        this.array = array;
        this.initVal = initVal;
    }

    public String getIdent() {
        return this.ident;
    }

    public ArrayList<Exp> getArray() {
        return this.array;
    }

    public InitVal getInitVal() {
        return this.initVal;
    }
}
