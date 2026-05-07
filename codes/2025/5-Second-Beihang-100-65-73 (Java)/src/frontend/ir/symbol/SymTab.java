package frontend.ir.symbol;

import frontend.ir.structure.Function;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

public class SymTab {
    private final SymTab parent;
    private final List<String> symNames;
    private final HashMap<String, Symbol> name2sym;
    private final List<String> funcNames;
    private final HashMap<String, Function> name2func;
    
    public SymTab(SymTab parent) {
        
        this.parent   = parent;
        this.name2sym = new HashMap<>();
        this.symNames = new ArrayList<>();
        
        if (parent == null) {
            funcNames = new ArrayList<>();
            name2func = new HashMap<>();
        } else {
            funcNames = null;
            name2func = null;
        }
    }
    
    public Symbol getObject(String name, int lineno) {
        if (this.name2sym.containsKey(name)) {
            return this.name2sym.get(name);
        }
        if (this.parent == null) {
            throw new RuntimeException("UNDEFINED OBJ NAME at line " + lineno);
        }
        return parent.getObject(name, lineno);
    }
    
    public Function getFunction(String name, int lineno) {
        if (this.name2func != null) {
            if (this.name2func.containsKey(name)) {
                return this.name2func.get(name);
            } else {
                throw new RuntimeException("UNDEFINED FUNC NAME at line " + lineno);
            }
        }
        return parent.getFunction(name, lineno);
    }
    
    public void addObject(Symbol object, int lineno) {
        if (object == null) {
            throw new NullPointerException();
        }
        String name = object.getIdent();
        
        if (symNames.contains(name) || (funcNames != null && funcNames.contains(name))) {
            throw new RuntimeException("NAME REDEFINITION at line " + lineno + ": " + name);
        }
        
        this.symNames.add(name);
        this.name2sym.put(name, object);
    }
    
    public void addFunc(Function function, int lineno) {
        if (function == null || funcNames == null || name2func == null) {
            throw new NullPointerException();
        }
        String name = function.getName();
        
        if (symNames.contains(name) || funcNames.contains(name)) {
            throw new RuntimeException("NAME REDEFINITION at line " + lineno + ": " + name);
        }
        
        this.funcNames.add(name);
        this.name2func.put(name, function);
    }
    
    public void addStringConst(Symbol symbol) {
        if (symbol == null) {
            throw new NullPointerException();
        }
        if (this.parent != null) {
            parent.addStringConst(symbol);
        } else {
            this.addObject(symbol, -1);
        }
    }
    
    public boolean isGlobal() {
        return this.parent == null;
    }
    
    public List<Symbol> getObjectList() {
        List<Symbol> objects = new ArrayList<>();
        for (String name : symNames) {
            objects.add(name2sym.get(name));
        }
        return objects;
    }
}
