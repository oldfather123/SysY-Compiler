package frontend.semantic.symbol;

import java.util.HashMap;

public class SymbolTable {
    private final HashMap<String, Symbol> symbolMap;
    private final SymbolTable father;
    public SymbolTable() {
        symbolMap = new HashMap<>();
        this.father = null;
    }

    public SymbolTable(SymbolTable father) {
        symbolMap = new HashMap<>();
        this.father = father;
    }

    public SymbolTable getFather() {
        return father;
    }

    public void addSymbol(Symbol symbol) {
        if (symbolMap.containsKey(symbol.getName())) {
            throw new IllegalArgumentException("Symbol already exists: " + symbol.getName());
        }
        symbolMap.put(symbol.getName(), symbol);
    }

    public Symbol getSymbol(String name) {
        Symbol symbol = symbolMap.get(name);
        if (symbol == null && father != null) {
            return father.getSymbol(name);
        }
        return symbol;
    }
}
