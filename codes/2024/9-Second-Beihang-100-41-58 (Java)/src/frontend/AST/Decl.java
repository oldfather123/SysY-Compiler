package frontend.AST;

import java.util.ArrayList;

//Def -> Ident { '[' Exp ']' } '=' InitVal
public class Decl implements CompUnit, BlockItem {
    private String bType;
    private boolean isConst;
    private ArrayList<Def> defs;

    public Decl(boolean isConst, String bType, ArrayList<Def> defs) {
        this.bType = bType;
        this.isConst = isConst;
        this.defs = defs;
    }
    public String getbType() {
        return this.bType;
    }

    public boolean getIsConst() {
        return this.isConst;
    }

    public ArrayList<Def> getDefs() {
        return this.defs;
    }
}