package utils;

import ir.traversal.SymbolTable;

import java.util.HashMap;

public class GlobalCounter {
    private static final HashMap<String, Integer> counter = new HashMap<>();

    private static int get(String key) {
        if (!counter.containsKey(key)) {
            counter.put(key, 0);
        }
        counter.put(key, counter.get(key) + 1);
        return counter.get(key);
    }

    public static SymbolTable.SymbolId gen(String key) {
        return new SymbolTable.SymbolId(key, get(key));
    }
}
