package frontend.ir.symbols;

import frontend.ir.DataType;
import frontend.ir.Value;
import frontend.ir.constvalue.ConstFloat;
import frontend.ir.constvalue.ConstInt;
import frontend.syntax.Ast;

import java.util.*;

public class SymTab {
    private final HashMap<String, Symbol> symbolMap = new HashMap<>();
    private final SymTab parent;
    private final HashMap<String, Symbol> abandonedSymbolMap = new HashMap<>();
    
    public SymTab(SymTab p) {
        parent = p;
    }
    
    public SymTab() {
        parent = null;
    }
    
    public boolean hasSym(String sym) {
        return symbolMap.containsKey(sym) || (parent != null && parent.hasSym(sym));
    }
    
    public Symbol getSym(String sym) {
        if (symbolMap.containsKey(sym)) {
            return symbolMap.get(sym);
        }
        if (parent == null) {
            if (abandonedSymbolMap.containsKey(sym)) {
                // 这里存的是被局部化的全局变量，保留这一级是为了有些编译时需要确定值的东西（比如数组长度）需要用到全局不可变对象的值
                // 其实主要是为了 printIR 准备的，正常是不需要保存这个表的
                return abandonedSymbolMap.get(sym);
            }
            throw new RuntimeException("No such symbol" + sym);
        }
        return parent.getSym(sym);
    }
    
    public void addSym(Symbol symbol) {
        if (symbol == null) {
            throw new NullPointerException();
        }
        String name = symbol.getName();
        if (symbolMap.containsKey(name)) {
            throw new RuntimeException("重名对象");
        }
        symbolMap.put(name, symbol);
    }
    
    public List<Symbol> parseNewSymbols(Ast.Decl decl) {
        ArrayList<Symbol> newSymList = new ArrayList<>();
        if (decl == null) {
            throw new NullPointerException();
        }
        boolean constant = decl.isConst();
        DataType dataType;
        switch (decl.getType().getType()) {
            case INT:   dataType = DataType.INT;    break;
            case FLOAT: dataType = DataType.FLOAT;  break;
            default : throw new RuntimeException("出现了意料之外的声明类型");
        }
        for (Ast.Def def : decl.getDefList()) {
            assert def.getType().equals(decl.getType().getType());
            String name = def.getIdent().getContent();
            List<Integer> limList = new ArrayList<>();
            for (Ast.Exp exp : def.getIndexList()) {
                if (exp.checkConstType(this) == DataType.INT) {
                    limList.add(exp.getConstInt(this));
                } else {
                    throw new RuntimeException("数组各维长度必须是整数");
                }
            }
            Value initVal;
            Ast.Init init = def.getInit();
            if (init != null) {
                initVal = createInitVal(dataType, init, limList);
            } else if (!limList.isEmpty()) {
                initVal = new ArrayInitVal(dataType, limList, false);
            } else if (isGlobal()) {
                initVal = dataType == DataType.FLOAT ? ConstFloat.Zero :
                                                       ConstInt.Zero;
            } else {
                initVal = null;
            }
            Symbol symbol = new Symbol(name, dataType, limList, constant, parent == null, initVal);
            newSymList.add(symbol);
            if (isGlobal() || constant) {
                // 对于全局对象和不可变对象，定义过程中可以直接算出结果，而且不涉及局部对全局的覆盖，故可以直接解析一个把一个加到表里
                this.addSym(symbol);
            }
        }
        return newSymList;
    }
    
    private Value createInitVal(DataType type, Ast.Init init, List<Integer> limList) {
        if (init instanceof Ast.Exp) {
            return dealNonArrayInit(type, (Ast.Exp) init);
        } else if (init instanceof Ast.InitArray) {
            return dealArrayInit(type, ((Ast.InitArray) init).getInitList(), limList);
        } else {
            throw new RuntimeException("奇怪的定义类型");
        }
    }
    
    private Value dealNonArrayInit(DataType type, Ast.Exp init) {
        if (type == null || init == null) {
            throw new NullPointerException();
        }
        switch (init.checkConstType(this)) {
            case INT:
                if (type == DataType.INT) {
                    return new ConstInt(init.getConstInt(this));
                } else if (type == DataType.FLOAT) {
                    return new ConstFloat(init.getConstInt(this).floatValue());
                }
                else {
                    throw new RuntimeException("你给我传了个什么鬼类型啊");
                }
            case FLOAT:
                if (type == DataType.INT) {
                    return new ConstInt(init.getConstFloat(this).intValue());
                } else if (type == DataType.FLOAT) {
                    return new ConstFloat(init.getConstFloat(this));
                } else {
                    throw new RuntimeException("你给我传了个什么鬼类型啊");
                }
            default:
                if (this.isGlobal()) {
                    throw new RuntimeException("全局变量初始值似乎不是确定值");
                }
                return new InitExpr(init);
        }
    }
    
    private Value dealArrayInit(DataType type, List<Ast.Init> initList, List<Integer> limList) {
        if (type == null || initList == null) {
            throw new NullPointerException();
        }
        int dim = limList.size();
        if (dim < 1) {
            throw new RuntimeException("不是，哥们，你这数组连一个维度都没有的吗？");
        }
        
        ArrayInitVal myInitVal = new ArrayInitVal(type, limList, true);
        Iterator<Ast.Init> it = initList.iterator();
        
        if (dim == 1) {
            while (it.hasNext() && !myInitVal.isFull()) {
                Ast.Init init = it.next();
                if (init instanceof Ast.Exp) {
                    myInitVal.addInitValue(dealNonArrayInit(type, (Ast.Exp) init));
                } else {
                    throw new RuntimeException("一维数组的初始化元素必须是表达式");
                }
                it.remove();
            }
        } else {
            List<Integer> nextLimList = limList.subList(1, limList.size());
            while (it.hasNext() && !myInitVal.isFull()) {
                Ast.Init init = it.next();
                if (init instanceof Ast.InitArray) {
                    List<Ast.Init> nextInitList = ((Ast.InitArray) init).getInitList();
                    myInitVal.addInitValue(dealArrayInit(type, nextInitList, nextLimList));
                    it.remove();
                } else {
                    myInitVal.addInitValue(dealArrayInit(type, initList, nextLimList));
                    it = initList.iterator();
                }
            }
        }
        
        return myInitVal;
    }
    
    public boolean isGlobal() {
        return parent == null;
    }

    public List<Symbol> getSymbolList() {
        return new ArrayList<>(symbolMap.values());
    }

    public List<Symbol> getAllSym() {
        return new ArrayList<>(symbolMap.values());
    }

    public void removeLocalizedSym() {
        Iterator<String> symbolIterator = symbolMap.keySet().iterator();
        while (symbolIterator.hasNext()) {
            String name = symbolIterator.next();
            Symbol symbol = symbolMap.get(name);
            if (symbol.isAbandoned()) {
                abandonedSymbolMap.put(name, symbol);
                symbolIterator.remove();
            }
        }
    }
}
