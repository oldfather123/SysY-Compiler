package frontend.AST;

import midend.Value;

import java.util.ArrayList;
import java.util.SplittableRandom;

public class AST {
    //CompUnit -> Decl | FuncDef
    private ArrayList<CompUnit> units;
    //Decl -> ['const'] Btype Def { ',' Def ',' } ';'
    //Def -> Ident { '[' Exp ']' } '=' InitVal
    //InitVal -> Exp | InitArray
    //InitArray -> '{' [ InitVal { ',' InitVal } ] '}'
    //FuncDef -> FuncType Ident '(' [FuncFParams] ')' Block
    //FuncType -> void | int | float
    //FuncFParams -> FuncFParam { ',' FuncFParam }
    //FuncFParam -> Btype Ident [ '[' ']' { '[' Exp ']'} ]
    //Block -> '{' { BlockItem } '}'
    //Blockitem -> Decl | Stmt=
    //Stmt -> Assign | Exp | Block | If | Else | While | Break | Continue | Return
    //Assign -> LVal '=' Exp ';'
    //If -> 'if' '(' Cond ')' Stmt [ Else ]
    //Cond -> Exp
    //Else -> 'else' Stmt
    //While -> 'while' '(' Cond ')' Stmt
    //Return -> 'return' [Exp] ';'
    //Exp -> BinaryExp | UnaryExp
    //BinaryExp -> Exp {BinaryOp Exp}
    //BinaryOp -> + - > >= < <= == && != ||
    //UnaryExp -> {UnaryOp} PrimaryExp
    //PrimaryExp -> '('Exp')' | LVal | Number | Func
    //LVal -> Ident { '[' Exp ''] }
    //Number -> IntConst | FloatConst
    //Func -> Ident '(' [FuncRParams] ')'
    //FuncRParams -> Exp {',' Exp}
    public AST(ArrayList<CompUnit> units) {
        this.units = units;
    }

    public ArrayList<CompUnit> getUnits() {
        return this.units;
    }

//    public Value genIR() {
//        if (units == null || units.isEmpty()) {
//            return null;
//        }
//
//    }
}
