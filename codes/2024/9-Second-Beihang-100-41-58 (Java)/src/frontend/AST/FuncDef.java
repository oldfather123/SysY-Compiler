package frontend.AST;

import java.util.ArrayList;

//FuncDef -> FuncType Ident '(' [FuncFParams] ')' Block
//FuncType -> void | int | float
public class FuncDef implements CompUnit {

    private final ArrayList<FuncFParam> parameters;
    private final Block body;
    private final String returnType; // void | int | float
    private final String name;


    public FuncDef(String returnType, String name, ArrayList<FuncFParam> parameters, Block body) {
        this.returnType = returnType;
        this.name = name;
        this.parameters = parameters;
        this.body = body;
    }

    public String getFuncType() {
        return returnType;
    }

    public String getIdent() {
        return name;
    }

    public ArrayList<FuncFParam> getFuncFParams() {
        return parameters;
    }

    public Block getBlock() {
        return body;
    }
}
