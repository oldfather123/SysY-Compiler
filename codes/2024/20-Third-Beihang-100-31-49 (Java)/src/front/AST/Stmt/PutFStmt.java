package front.AST.Stmt;

import front.AST.Exp.Exp;
import front.AST.Node;
import front.AST.TokenNode;
import front.lexer.Token;
import utils.tools.Printer;
import utils.type.ErrorType;
import utils.type.SyntaxType;

import java.util.ArrayList;

// PutFStmt ==> 'putf' '(' FormatString { ',' Exp } ')' ';'
public class PutFStmt extends Stmt {

    private final Token strToken;
    private final ArrayList<Exp> expLst;

    public PutFStmt(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
        // get strToken
        strToken = ((TokenNode) children.get(2)).getToken();
        // getExpList
        expLst = new ArrayList<>();
        for (Node child : children) {
            // search Exp
            if (child instanceof Exp exp) {
                expLst.add(exp);
            }
        }
    }

    public boolean checkFormatString() {
        // get formatString
        String formatString = strToken.getValue();
        // skip the '\"' in the start & end
        for (int i = 1; i < formatString.length() - 1; i++) {
            char chr = formatString.charAt(i);
            // check: basic character
            // ' ' || '!' but deal with '\' (92)
            if (chr == 32 || chr == 33 || chr >= 40 && chr <= 126 && chr != '\\') {
                continue;
            }
            // '\n'
            if (chr == '\\' && formatString.charAt(i + 1) == 'n') {
                continue;
            }
            // '%d'
            if (chr == '%' && formatString.charAt(i + 1) == 'd') {
                continue;
            }
            return false;
        }
        return true;
    }

    public void checkError() {
        // first get children info
        super.checkError();
        // check: formatString
        if (!checkFormatString()) {
            Printer.recordError(strToken.getLine(), ErrorType.a);
            return;
        }
        // check: count '%d'
        String fmtStr = strToken.getValue();
        int cnt = 0;
        for (int i = 1; i < fmtStr.length() - 1; i++) {
            if (fmtStr.charAt(i) == '%') {
                cnt++;
            }
        }
        if (cnt != expLst.size()) {
            Printer.recordError(strToken.getLine(), ErrorType.l);
        }
    }

}

