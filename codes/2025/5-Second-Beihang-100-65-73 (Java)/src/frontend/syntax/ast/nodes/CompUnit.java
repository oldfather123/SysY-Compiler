package frontend.syntax.ast.nodes;

import frontend.syntax.ast.AST;
import frontend.syntax.ast.nodes.declanddef.Decl;
import frontend.syntax.ast.nodes.declanddef.FuncDef;

import java.util.ArrayList;

/**
 * 编译单元 CompUnit → {Decl} {FuncDef} MainFuncDef
 */
public class CompUnit extends AST.Node {
    private final ArrayList<Decl>    declList;
    private final ArrayList<FuncDef> funcDefList;
    private final FuncDef            mainFuncDef;
    
    public CompUnit(ArrayList<Decl> declList, ArrayList<FuncDef> funcDefList, FuncDef mainFuncDef, int lineno) {
        super(lineno);
        this.declList    = declList;
        this.funcDefList = funcDefList;
        this.mainFuncDef = mainFuncDef;
    }
    
    public ArrayList<Decl> getDeclList() {
        return declList;
    }
    
    public ArrayList<FuncDef> getFuncDefList() {
        return funcDefList;
    }
    
    public FuncDef getMainFuncDef() {
        return mainFuncDef;
    }
}
