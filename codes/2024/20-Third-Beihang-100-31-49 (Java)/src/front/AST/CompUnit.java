package front.AST;

import mid.IntermediatePresentation.Function.LibFunction;
import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.Value;
import utils.type.SyntaxType;

import java.util.ArrayList;

// CompUnit ==> { VarDecl | ConstDecl } { FuncDef } MainFuncDef
public class CompUnit extends Node {

    public CompUnit(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }

    public Value toIR() {
        new LibFunction.Getint();
        new LibFunction.Getch();
        new LibFunction.Getfloat();
        new LibFunction.Getarray();
        new LibFunction.Getfarray();
        new LibFunction.Putint();
        new LibFunction.Putch();
        new LibFunction.Putfloat();
        new LibFunction.Putarray();
        new LibFunction.Putfarray();
        new LibFunction.Starttime();
        new LibFunction.Stoptime();
        new LibFunction.Memset();
        super.toIR();
        IRManager.getModule().check();
        return null;
    }

}
