package frontend.AST;

import java.util.ArrayList;

//InitArray -> '{' [ InitVal { ',' InitVal } ] '}'
public class InitArray implements InitVal{
    private ArrayList<InitVal> valArrayList;
    //TODO:nowidx?

    public InitArray(ArrayList<InitVal> initVals) {
        this.valArrayList = initVals;
    }

    public ArrayList<InitVal> getValArrayList() {
        return this.valArrayList;
    }
}