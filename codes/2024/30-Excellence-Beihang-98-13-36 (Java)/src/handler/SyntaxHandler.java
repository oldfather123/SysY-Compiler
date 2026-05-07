package handler;

import entities.TreeNode;
import entities.Word;

import java.util.List;

public class SyntaxHandler {

    private static List<Word> words;
    private static int index = 0;

    private static Word next() {
        return words.get(index++);
    }

    private static Word getNext(int i) {
        return words.get(index + i);
    }

    private static Word current() {
        return words.get(index);
    }

    public static TreeNode buildSyntaxTree(List<Word> words_) {
        words = words_;
        index = 0;
        return CompUnit();
    }

    private static boolean isConstDecl() {
        return current().is(Word.Type.CONST);
    }

    private static boolean isVarDecl() {
        return current().isBType() && (index + 2 < words.size() && !getNext(2).is(Word.Type.LPARENT));
    }

    private static boolean isFuncDef() {
        return current().isFuncType();
    }

    private static TreeNode CompUnit() {
        TreeNode compUnit = new TreeNode(TreeNode.Type.COMP_UNIT);
        while (index < words.size()) {
            if (isConstDecl()) {
                compUnit.addSon(constDecl());
            } else if (isVarDecl()) {
                compUnit.addSon(varDecl());
            } else if (isFuncDef()) {
                compUnit.addSon(funcDef());
            } else {
                break;
            }
        }
        return compUnit;
    }

    private static TreeNode constDecl() {
        next();// const
        TreeNode constDecl = new TreeNode(TreeNode.Type.CONST_DECL);
        constDecl.addSon(bType());
        do {
            constDecl.addSon(constDef());
        } while (next().is(Word.Type.COMMA));
        return constDecl;
    }

    private static TreeNode bType() {
        return new TreeNode(TreeNode.Type.B_TYPE).word(next());
    }

    private static TreeNode constDef() {
        TreeNode constDef = new TreeNode(TreeNode.Type.CONST_DEF);
        constDef.addSon(ident());
        while (current().is(Word.Type.LBRACK)) {
            next();// [
            constDef.addSon(addExp());
            next();// ]
        }
        next();// =
        constDef.addSon(constInitVal());
        return constDef;
    }

    private static TreeNode ident() {
        return new TreeNode(TreeNode.Type.IDENT).word(next());
    }

    private static TreeNode constInitVal() {
        if (current().is(Word.Type.LBRACE)) {
            TreeNode constInitVal = new TreeNode(TreeNode.Type.CONST_INIT_VAL);
            next();// {
            while (!current().is(Word.Type.RBRACE)) {
                constInitVal.addSon(constInitVal());
                if (current().is(Word.Type.COMMA)) {
                    next();// ,
                }
            }
            next();// }
            return constInitVal;
        } else {
            return addExp();
        }
    }

    private static TreeNode varDecl() {
        TreeNode varDecl = new TreeNode(TreeNode.Type.VAR_DECL);
        varDecl.addSon(bType());
        do {
            varDecl.addSon(varDef());
        } while (next().is(Word.Type.COMMA));
        return varDecl;
    }

    private static TreeNode varDef() {
        TreeNode varDef = new TreeNode(TreeNode.Type.VAR_DEF);
        varDef.addSon(ident());
        while (current().is(Word.Type.LBRACK)) {
            next();// [
            varDef.addSon(addExp());
            next();// ]
        }
        if (current().is(Word.Type.ASSIGN)) {
            next();// =
            varDef.addSon(initVal());
        } else if (varDef.getSonCount() == 1) {
            varDef.addSon(buildZeroAddExp());
        } else {
            varDef.addSon(buildZeroInitVal());
        }
        return varDef;
    }

    private static TreeNode initVal() {
        if (current().is(Word.Type.LBRACE)) {
            TreeNode initVal = new TreeNode(TreeNode.Type.INIT_VAL);
            next();// {
            while (!current().is(Word.Type.RBRACE)) {
                initVal.addSon(initVal());
                if (current().is(Word.Type.COMMA)) {
                    next();// ,
                }
            }
            next();// }
            return initVal;
        } else {
            return addExp();
        }
    }

    private static TreeNode funcDef() {
        TreeNode funcDef = new TreeNode(TreeNode.Type.FUNC_DEF);
        funcDef.addSon(funcType());
        funcDef.addSon(ident());
        next();// (
        if (current().isBType()) {
            do {
                funcDef.addSon(funcFParam());
            } while (next().is(Word.Type.COMMA));
        } else {
            next();// )
        }
        funcDef.addSon(block());
        return funcDef;
    }

    private static TreeNode funcType() {
        return new TreeNode(TreeNode.Type.FUNC_TYPE).word(next());
    }

    private static TreeNode funcFParam() {
        TreeNode funcFParam = new TreeNode(TreeNode.Type.FUNC_F_PARAM);
        funcFParam.addSon(bType());
        funcFParam.addSon(ident());
        if (current().is(Word.Type.LBRACK)) {
            next();// [
            funcFParam.addSon(new TreeNode(TreeNode.Type.EMPTY));
            next();// ]
        }
        while (current().is(Word.Type.LBRACK)) {
            next();// [
            funcFParam.addSon(addExp());
            next();// ]
        }
        return funcFParam;
    }

    private static TreeNode block() {
        TreeNode block = new TreeNode(TreeNode.Type.BLOCK);
        next();// {
        while (!current().is(Word.Type.RBRACE)) {
            block.addSon(blockItem());
        }
        next();// }
        return block;
    }

    private static TreeNode blockItem() {
        if (isConstDecl()) {
            return constDecl();
        } else if (isVarDecl()) {
            return varDecl();
        } else {
            return stmt();
        }
    }

    private static TreeNode stmt() {
        TreeNode stmt = new TreeNode(TreeNode.Type.STMT);
        if (current().is(Word.Type.IF)) {
            stmt.addSon(new TreeNode(TreeNode.Type.IF).word(next()));
            next();// (
            stmt.addSon(lOrExp());
            next();// )
            stmt.addSon(stmt());
            if (current().is(Word.Type.ELSE)) {
                next();// else
                stmt.addSon(stmt());
            }
        } else if (current().is(Word.Type.WHILE)) {
            stmt.addSon(new TreeNode(TreeNode.Type.WHILE).word(next()));
            next();// (
            stmt.addSon(lOrExp());
            next();// )
            stmt.addSon(stmt());
        } else if (current().is(Word.Type.BREAK)) {
            stmt.addSon(new TreeNode(TreeNode.Type.BREAK).word(next()));
            next();// ;
        } else if (current().is(Word.Type.CONTINUE)) {
            stmt.addSon(new TreeNode(TreeNode.Type.CONTINUE).word(next()));
            next();// ;
        } else if (current().is(Word.Type.RETURN)) {
            stmt.addSon(new TreeNode(TreeNode.Type.RETURN).word(next()));
            if (!current().is(Word.Type.SEMICN)) {
                stmt.addSon(addExp());
            }
            next();// ;
        } else if (current().is(Word.Type.LBRACE)) {
            stmt.addSon(block());
        } else if (current().is(Word.Type.SEMICN)) {
            stmt.addSon(new TreeNode(TreeNode.Type.EMPTY));
            next();// ;
        } else {
            int i = 0;
            while (!getNext(i).is(Word.Type.SEMICN) && !getNext(i).is(Word.Type.ASSIGN)) {
                i++;
            }
            if (getNext(i).is(Word.Type.ASSIGN)) {
                stmt.addSon(lVal());
                next();// =
            }
            stmt.addSon(addExp());
            next();// ;
        }
        return stmt;
    }

    private static TreeNode lVal() {
        TreeNode lVal = new TreeNode(TreeNode.Type.L_VAL);
        lVal.addSon(ident());
        while (current().is(Word.Type.LBRACK)) {
            next();// [
            lVal.addSon(addExp());
            next();// ]
        }
        return lVal;
    }

    private static TreeNode primaryExp() {
        TreeNode primaryExp = new TreeNode(TreeNode.Type.PRIMARY_EXP);
        if (current().is(Word.Type.LPARENT)) {
            next();// (
            primaryExp.addSon(addExp());
            next();// )
        } else if (current().is(Word.Type.IDENT)) {
            primaryExp.addSon(lVal());
        } else {
            primaryExp.addSon(number());
        }
        return primaryExp;
    }

    private static TreeNode number() {
        if (current().is(Word.Type.FLOAT_CONST)) {
            return new TreeNode(TreeNode.Type.FLOAT_CONST).word(next());
        }
        return new TreeNode(TreeNode.Type.INT_CONST).word(next());
    }

    private static TreeNode unaryExp() {
        TreeNode unaryExp = new TreeNode(TreeNode.Type.UNARY_EXP);
        if (current().is(Word.Type.PLUS) || current().is(Word.Type.MINU) || current().is(Word.Type.NOT)) {
            unaryExp.addSon(unaryOp());
            unaryExp.addSon(unaryExp());
        } else if (current().is(Word.Type.IDENT) && getNext(1).is(Word.Type.LPARENT)) {
            unaryExp.addSon(ident());
            next();// (
            while (!current().is(Word.Type.RPARENT)) {
                unaryExp.addSon(addExp());
                if (current().is(Word.Type.COMMA)) {
                    next();// ,
                }
            }
            next();// )
        } else {
            unaryExp.addSon(primaryExp());
        }
        return unaryExp;
    }

    private static TreeNode unaryOp() {
        return new TreeNode(TreeNode.Type.UNARY_OP).word(next());
    }

    private static TreeNode operator() {
        return new TreeNode(TreeNode.Type.OPERATOR).word(next());
    }

    private static TreeNode mulExp() {
        TreeNode mulExp = new TreeNode(TreeNode.Type.MUL_EXP);
        mulExp.addSon(unaryExp());
        while (current().is(Word.Type.MULT) || current().is(Word.Type.DIV) || current().is(Word.Type.MOD)) {
            mulExp.addSon(operator());
            mulExp.addSon(unaryExp());
        }
        return mulExp;
    }

    private static TreeNode addExp() {
        TreeNode addExp = new TreeNode(TreeNode.Type.ADD_EXP);
        addExp.addSon(mulExp());
        while (current().is(Word.Type.PLUS) || current().is(Word.Type.MINU)) {
            addExp.addSon(operator());
            addExp.addSon(mulExp());
        }
        return addExp;
    }

    private static TreeNode relExp() {
        TreeNode relExp = new TreeNode(TreeNode.Type.REL_EXP);
        relExp.addSon(addExp());
        while (current().is(Word.Type.LSS) || current().is(Word.Type.GRE) ||
                current().is(Word.Type.LEQ) || current().is(Word.Type.GEQ)) {
            relExp.addSon(operator());
            relExp.addSon(addExp());
        }
        return relExp;
    }

    private static TreeNode eqExp() {
        TreeNode eqExp = new TreeNode(TreeNode.Type.EQ_EXP);
        eqExp.addSon(relExp());
        while (current().is(Word.Type.EQL) || current().is(Word.Type.NEQ)) {
            eqExp.addSon(operator());
            eqExp.addSon(relExp());
        }
        return eqExp;
    }

    private static TreeNode lAndExp() {
        TreeNode lAndExp = new TreeNode(TreeNode.Type.L_AND_EXP);
        lAndExp.addSon(eqExp());
        while (current().is(Word.Type.AND)) {
            operator();
            lAndExp.addSon(eqExp());
        }
        return lAndExp;
    }

    private static TreeNode lOrExp() {
        TreeNode lOrExp = new TreeNode(TreeNode.Type.L_OR_EXP);
        lOrExp.addSon(lAndExp());
        while (current().is(Word.Type.OR)) {
            operator();
            lOrExp.addSon(lAndExp());
        }
        return lOrExp;
    }

    private static TreeNode buildZeroAddExp() {
        TreeNode addExp = new TreeNode(TreeNode.Type.ADD_EXP);
        TreeNode mulExp = new TreeNode(TreeNode.Type.MUL_EXP);
        TreeNode unaryExp = new TreeNode(TreeNode.Type.UNARY_EXP);
        TreeNode primaryExp = new TreeNode(TreeNode.Type.PRIMARY_EXP);
        TreeNode number = new TreeNode(TreeNode.Type.INT_CONST).word(Word.getWord(Word.Type.INT_CONST, "0"));
        primaryExp.addSon(number);
        unaryExp.addSon(primaryExp);
        mulExp.addSon(unaryExp);
        addExp.addSon(mulExp);
        return addExp;
    }

    private static TreeNode buildZeroInitVal() {
        TreeNode initVal = new TreeNode(TreeNode.Type.INIT_VAL);
        initVal.addSon(buildZeroAddExp());
        return initVal;
    }

}
