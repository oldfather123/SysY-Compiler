package frontend.AST;

import java.util.ArrayList;

//FuncFParams -> FuncFParam { ',' FuncFParam }
//FuncFParam -> Btype Ident [ '[' ']' { '[' Exp ']'} ]
public class FuncFParam {

    private boolean arrayFlag;
    private ArrayList<Exp> expArray;
    private String baseType; // int | float
    private String name;

    public FuncFParam(String baseType, String name, boolean arrayFlag, ArrayList<Exp> expArray) {
        this.baseType = baseType;
        this.name = name;
        this.arrayFlag = arrayFlag;
        this.expArray = expArray;
    }

    public String getbType() {
        return baseType;
    }

    public String getIdent() {
        return name;
    }

    public boolean getIsArray() {
        return arrayFlag;
    }

    public ArrayList<Exp> getArray() {
        return expArray;
    }
}


