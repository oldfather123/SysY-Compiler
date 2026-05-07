package frontend.AST;

import java.lang.reflect.Array;
import java.util.ArrayList;

//Func -> Ident '(' [FuncRParams] ')'
public class Func implements PrimaryExp {
    private final ArrayList<Exp> parameters;
    private final int lineNumber;
    private final String identifier;


    public Func(String identifier, ArrayList<Exp> parameters, int lineNumber) {
        this.identifier = identifier;
        this.parameters = parameters;
        this.lineNumber = lineNumber;
    }

    public String getIdent() {
        return identifier;
    }

    public ArrayList<Exp> getFuncRParams() {
        return parameters;
    }

    public int getLinuNum() {
        return lineNumber;
    }
}

