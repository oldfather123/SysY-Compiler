package Utils;

import midend.Value;

import java.util.HashMap;
import java.util.Stack;

public class SymbolTable {
    private Stack<HashMap<String, Value>> symbolTable = new Stack<>();

    public SymbolTable() {

    }

    public void pushsymbol() {
        symbolTable.push(new HashMap<>());
    }

    public void pushsymbol(String ident, Value value) {
        symbolTable.peek().put(ident, value);
    }

    public void popsymbol() {
        symbolTable.pop();
    }

    public Value find(String ident) {
        for (int count = symbolTable.size() - 1; count >= 0; count--) {
            HashMap<String, Value> tmp = symbolTable.get(count);
            if (tmp.containsKey(ident)) {
                return tmp.get(ident);
            }
        }

        return LibFunction.findFunc(ident);
    }
}
