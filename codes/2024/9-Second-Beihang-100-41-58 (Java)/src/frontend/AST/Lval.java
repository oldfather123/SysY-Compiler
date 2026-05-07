package frontend.AST;

import java.util.ArrayList;

//LVal -> Ident { '[' Exp ''] }
public class Lval implements PrimaryExp {
    private String ident;
    private boolean isArray;
    private ArrayList<Exp> indexs;

    public Lval(String ident, boolean isArray, ArrayList<Exp> indexs) {
        this.ident = ident;
        this.isArray = isArray;
        this.indexs = indexs;
    }

    public String getIdent() {
        return this.ident;
    }

    public boolean getIsArray() {
        return this.isArray;
    }

    public ArrayList<Exp> getIndexs() {
        return this.indexs;
    }
}
