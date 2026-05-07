package frontend.ir.structure;

import frontend.ir.lib.Lib;
import frontend.ir.symbols.SymTab;
import frontend.ir.symbols.Symbol;
import frontend.syntax.Ast;

import java.io.IOException;
import java.io.Writer;
import java.util.*;

public class Program {
    private final SymTab globalSymTab = new SymTab();
//    private final HashMap<String, Function> functions = new HashMap<>();
    private final HashSet<String> funcNames = new HashSet<>();
    private final ArrayList<Function> functionList = new ArrayList<>();
    
    public Program(Ast ast) {
        if (ast == null) {
            throw new NullPointerException();
        }
        for (Ast.CompUnit compUnit : ast.getUnits()) {
            if (compUnit instanceof Ast.FuncDef) {
                String funcName = ((Ast.FuncDef) compUnit).getIdent().getContent();
                if (funcNames.contains(funcName)) {
                    throw new RuntimeException("重复的函数命名");
                }
                if (globalSymTab.hasSym(funcName)) {
                    throw new RuntimeException("函数命名与全局变量名重复");
                }
                Function newFunc = new Function((Ast.FuncDef) compUnit, globalSymTab);
                funcNames.add(funcName);
                functionList.add(newFunc);
            } else if (compUnit instanceof Ast.Decl) {
                globalSymTab.parseNewSymbols((Ast.Decl) compUnit);
            } else {
                throw new RuntimeException("未定义的编译单元");
            }
        }
    }
    
    public void printIR(Writer writer) throws IOException {
        if (writer == null) {
            throw new NullPointerException();
        }
        writer.write("");
        writeGlobalDecl(writer);
        Lib.getInstance().declareUsedFunc(writer);
        int i = 0;
        for (Function function : functionList) {
            function.printIR(writer);
            if (++i < functionList.size()) {
                writer.append("\n");
            }
        }
    }

    public List<Symbol> getGlobalVars() {
        return globalSymTab.getSymbolList();
    }
    
    public SymTab getGlobalSymTab() {
        return globalSymTab;
    }
    
    public void deleteUselessGlobalVars() {
        globalSymTab.removeLocalizedSym();
    }
    
    public ArrayList<Function> getFunctionList(){
        return functionList;
    }

    private void writeGlobalDecl(Writer writer) throws IOException {
        if (writer == null) {
            throw new NullPointerException();
        }
        if (globalSymTab.getAllSym().isEmpty()) {
            return;
        }
        for (Symbol symbol : globalSymTab.getAllSym()) {
            writer.append("@").append(symbol.getName()).append(" = global ");
            if (symbol.isArray()) {
                writer.append(symbol.printArrayTypeName());
            } else {
                writer.append(symbol.getType().toString());
            }
            writer.append(" ");
            writer.append(symbol.getInitVal().value2string()).append("\n");
        }
        writer.append("\n");
    }
    
    public void removeUselessFunc() {
        Iterator<Function> iterator = functionList.iterator();
        while (iterator.hasNext()) {
            Function function = iterator.next();
            function.updateUse();
            if (function.noUse()) {
                iterator.remove();
                functionList.remove(function);
                funcNames.remove(function.getName());
            }
        }
    }
}
