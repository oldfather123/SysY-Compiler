package entities;

import java.util.ArrayList;
import java.util.List;

public class TreeNode {

    private final List<TreeNode> sons = new ArrayList<>();
    private final Type type;
    private Word word;
    private int index = 0;

    public TreeNode(Type type) {
        this.type = type;
    }

    public void addSon(TreeNode son) {
        this.sons.add(son);
    }

    public TreeNode initIterator() {
        index = 0;
        return nextSon();
    }

    public TreeNode currentSon() {
        if (index < sons.size()) {
            return sons.get(index);
        }
        return null;
    }

    public TreeNode nextSon() {
        if (index < sons.size()) {
            return sons.get(index++);
        }
        return null;
    }

    public enum Type {
        EMPTY,// empty
        IDENT,// an ident
        COMP_UNIT,// { ConstDecl | ValDecl | FuncDef | MainFuncDef }+
        B_TYPE,// int or float
        //Decl == ( ConstDecl | ValDecl )
        CONST_DECL,// BType { ConstDef }+
        CONST_DEF,// Ident { AddExp } ( ConstInitVal | AddExp )
        CONST_INIT_VAL,// { AddExp | ConstInitVal }+
        VAR_DECL,// BType { VarDef }+
        VAR_DEF,// Ident { AddExp } [ InitVal | AddExp ]
        INIT_VAL,// { InitVal | ConstInitVal }+
        FUNC_DEF,// FuncType Ident { FuncFParam } Block
        FUNC_TYPE,// int or float or void
        FUNC_F_PARAM,// BType Ident { AddExp }
        BLOCK,// { BlockItem }
        // BlockItem = ConstDecl | ValDecl | Stmt
        STMT,
        // LVal AddExp
        // AddExp
        // Empty
        // Block
        // If LOrExp Stmt [Stmt]
        // While LOrExp Stmt
        // Break
        // Continue
        // Return [AddExp]
        L_VAL,// Ident { AddExp }
        PRIMARY_EXP,// IntConst | FloatConst | AddExp | LVal
        INT_CONST,// an int const
        FLOAT_CONST,// a float const
        //FuncRParam == Exp == ConstExp == AddExp
        UNARY_EXP,// ( UnaryOp UnaryExp ) | PrimaryExp | ( Ident { AddExp } )
        UNARY_OP,// + or - or !
        MUL_EXP,// UnaryExp { ( * | / | % ) UnaryExp }
        ADD_EXP,// MulExp { ( + | - ) MulExp }
        // Cond = LOrExp
        REL_EXP,// AddExp { ( < | <= | > | >= ) AddExp }
        EQ_EXP,// RelExp { ( == | != ) RelExp }
        L_AND_EXP,// EqExp { EqExp }
        L_OR_EXP,// LAndExp { LAndExp }
        IF,// if
        WHILE,// while
        BREAK,// break
        CONTINUE,// continue
        RETURN,// return
        OPERATOR,// + or - or * or / or % or && or || or < or <= or > or >= or == or !=
    }

    public Word getWord() {
        return word;
    }

    public TreeNode word(Word word) {
        this.word = word;
        return this;
    }

    public boolean typeIs(Type type) {
        return this.type == type;
    }

    public int getSonCount() {
        return sons.size();
    }
}
