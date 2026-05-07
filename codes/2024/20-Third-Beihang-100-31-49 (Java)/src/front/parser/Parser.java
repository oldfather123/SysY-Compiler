package front.parser;

import front.AST.Node;
import front.AST.TokenNode;
import front.lexer.Token;
import front.lexer.TokenStream;
import utils.tools.NodeFactory;
import utils.tools.Printer;
import utils.type.ErrorType;
import utils.type.SyntaxType;
import utils.type.TokenType;

import java.util.ArrayList;

public class Parser {
    private final TokenStream tokenStream;
    private Token curToken;

    public Parser(TokenStream tokenStream) {
        this.tokenStream = tokenStream;
        curToken = tokenStream.read();
    }

    public void read() {
        curToken = tokenStream.read();
    }

    // CompUnit ==> { VarDecl | ConstDecl } { FuncDef } MainFuncDef
    public Node parseCompUnit() {
        ArrayList<Node> children = new ArrayList<>();
        while (curToken != null) {
            // MainFuncDef
            if (tokenStream.lookForward().getValue().equals("main")) {
                children.add(parseMainFuncDef());
            }
            // FuncDef
            else if (tokenStream.forwardTwice().getTokenType() == TokenType.LPARENT) {
                children.add(parseFuncDef());
            }
            // ConstDecl
            else if (curToken.getTokenType() == TokenType.CONSTTK) {
                children.add(parseConstDecl());
            }
            // VarDecl
            else {
                children.add(parseVarDecl());
            }
        }
        return NodeFactory.createNode(SyntaxType.COMP_UNIT, children);
    }

    // MainFuncDef ==> 'int' 'main' '(' ')' Block
    public Node parseMainFuncDef() {
        ArrayList<Node> children = new ArrayList<>();
        // 'int'
        children.add(parseBType());
        // 'main'
        children.add(NodeFactory.createNode(curToken));
        read();
        // '('
        children.add(NodeFactory.createNode(curToken));
        read();
        // ')'
        checkRParent(children);
        // Block
        children.add(parseBlock());
        return NodeFactory.createNode(SyntaxType.MAIN_FUNC_DEF, children);
    }

    // Block ==> '{' { VarDecl | ConstDecl | Stmt } '}'
    public Node parseBlock() {
        ArrayList<Node> children = new ArrayList<>();
        // '{'
        children.add(NodeFactory.createNode(curToken));
        read();
        // { VarDecl | ConstDecl | Stmt }
        // ! '}'
        while (curToken.getTokenType() != TokenType.RBRACE) {
            // ConstDecl
            // 'const'
            if (curToken.getTokenType() == TokenType.CONSTTK) {
                children.add(parseConstDecl());
            }
            // VarDecl
            // 'int' | 'float'
            else if (curToken.getTokenType() == TokenType.INTTK || curToken.getTokenType() == TokenType.FLOATTK) {
                children.add(parseVarDecl());
            }
            // Stmt
            else {
                children.add(parseStmt());
            }
        }
        // '}'
        children.add(NodeFactory.createNode(curToken));
        read();
        return NodeFactory.createNode(SyntaxType.BLOCK, children);
    }

    // Stmt ==> AssignStmt | ExpStmt | BlockStmt | IfStmt | BreakStmt |
    // ReturnStmt | GetIntStmt | ContinueStmt | GetArrayStmt | GetChStmt | GetFArrayStmt
    // GetFloatStmt | PutArrayStmt | PutChStmt | PutFArrayStmt | PutFloatStmt | PutFStmt | PutIntStmt
    // StartTimeStmt | StopTimeStmt | WhileStmt
    public Node parseStmt() {
        if (curToken.getTokenType() == TokenType.GETCHTK || curToken.getTokenType() == TokenType.GETARRAYTK ||
                curToken.getTokenType() == TokenType.GETINTTK || curToken.getTokenType() == TokenType.GETFARRAY || curToken.getTokenType() == TokenType.GETFLOATTK) {
            while(curToken.getTokenType() != TokenType.SEMICN) {
                read();
            }
        }
        // WhileStmt
        // 'while'
        if (curToken.getTokenType() == TokenType.WHILETK) {
            return parseWhileStmt();
        }
        // StopTimeStmt
        // 'stoptime'
        /*else if (curToken.getTokenType() == TokenType.STOPTIMETK) {
            return parseStopTimeStmt();
        }
        // StartTimeStmt
        // 'starttime'
        else if (curToken.getTokenType() == TokenType.STARTTIMETK) {
            return parseStartTimeStmt();
        }
        // PutFStmt
        // 'putf'
        else if (curToken.getTokenType() == TokenType.PUTFTK) {
            return parsePutFStmt();
        }
        // PutFloatStmt
        // 'putfloat'
        else if (curToken.getTokenType() == TokenType.PUTFLOATTK) {
            return parsePutFloatStmt();
        }
        // PutIntStmt
        // 'putint'
        else if (curToken.getTokenType() == TokenType.PUTINTTK) {
            return parsePutIntStmt();
        }
        // PutFArrayStmt
        // 'purfarray'
        else if (curToken.getTokenType() == TokenType.PUTFARRAYTK) {
            return parsePutFArrayStmt();
        }
        // PutChStmt
        // 'putch'
        else if (curToken.getTokenType() == TokenType.PUTCHTK) {
            return parsePutChStmt();
        }
        // PutArrayStmt
        // 'putarray'
        else if (curToken.getTokenType() == TokenType.PUTARRAYTK) {
            return parsePutArrayStmt();
        }*/

        // IfStmt
        // 'if'
        else if (curToken.getTokenType() == TokenType.IFTK) {
            return parseIfStmt();
        }
        // BlockStmt
        // '{'
        else if (curToken.getTokenType() == TokenType.LBRACE) {
            return parseBlockStmt();
        }
        // BreakStmt
        // 'break'
        else if (curToken.getTokenType() == TokenType.BREAKTK) {
            return parseBreakStmt();
        }
        // ContinueStmt
        // 'continue'
        else if (curToken.getTokenType() == TokenType.CONTINUETK) {
            return parseContinueStmt();
        }
        // ReturnStmt
        // 'return'
        else if (curToken.getTokenType() == TokenType.RETURNTK) {
            return parseReturnStmt();
        }
        // ExpStmt
        // [ Exp ] ;
        // ';'
        // Have to check ExpStmt ==> ;
        // so that we can make sure the follow steps are parsing ExpStmt ==> Exp ;   AssignStmt or  GetIntStmt
        // all of their first has Ident
        else if (curToken.getTokenType() == TokenType.SEMICN || curToken.getTokenType() == TokenType.PUTARRAYTK || curToken.getTokenType() == TokenType.PUTFARRAYTK ||
                curToken.getTokenType() == TokenType.PUTFLOATTK || curToken.getTokenType() == TokenType.PUTINTTK ||
                curToken.getTokenType() == TokenType.PUTCHTK || curToken.getTokenType() == TokenType.STARTTIMETK || curToken.getTokenType() == TokenType.STOPTIMETK) {
            return parseExpStmt();
        }
        // AssignStmt | GetIntStmt | ExpStmt
        // pretend to read
        tokenStream.sentryStand();
        Printer.switchOff();
        parseExp();
//        Printer.switchOn();
        // Order is certain !!!
        // GetIntStmt -> AssignStmt -> ExpStmt
        // GetIntStmt
        if (curToken.getTokenType() == TokenType.ASSIGN
                && tokenStream.lookForward().getTokenType() == TokenType.GETINTTK) {
            curToken = tokenStream.backToSentry();
            return parseGetIntStmt();
        }
        // GetArrayStmt
        // 'getarray'
        else if (curToken.getTokenType() == TokenType.ASSIGN
                && tokenStream.lookForward().getTokenType() == TokenType.GETARRAYTK) {
            curToken = tokenStream.backToSentry();
            return parseGetArrayStmt();
        }
        // GetFArrayStmt
        // 'getfarray'
        else if (curToken.getTokenType() == TokenType.ASSIGN
                && tokenStream.lookForward().getTokenType() == TokenType.GETFARRAY) {
            curToken = tokenStream.backToSentry();
            return parseGetFArrayStmt();
        }
        // GetChStmt
        // 'getch'
        else if (curToken.getTokenType() == TokenType.ASSIGN
                && tokenStream.lookForward().getTokenType() == TokenType.GETCHTK) {
            curToken = tokenStream.backToSentry();
            return parseGetChStmt();
        }
        // GetFloatStmt
        // 'getfloat'
        else if (curToken.getTokenType() == TokenType.ASSIGN
                && tokenStream.lookForward().getTokenType() == TokenType.GETFLOATTK) {
            curToken = tokenStream.backToSentry();
            return parseGetFloatStmt();
        }
        // AssignStmt
        else if (curToken.getTokenType() == TokenType.ASSIGN) {
            curToken = tokenStream.backToSentry();
            return parseAssignStmt();
        }
        // ExpStmt
        else {
            curToken = tokenStream.backToSentry();
            return parseExpStmt();
        }

    }

    // WhileStmt ==> 'while' '(' Cond ')' Stmt
    public Node parseWhileStmt() {
        ArrayList<Node> children = new ArrayList<>();
        // 'while'
        children.add(NodeFactory.createNode(curToken));
        read();
        // '('
        children.add(NodeFactory.createNode(curToken));
        read();
        // Cond
        children.add(parseCond());
        // ')'
        checkRParent(children);
        // Stmt
        children.add(parseStmt());
        return NodeFactory.createNode(SyntaxType.WHILE_STMT, children);
    }

    // StopTimeStmt ==> 'stoptime' '(' ')' ';'
    public void parseStopTimeStmt(ArrayList<Node> children) {
        // 'stoptime'
        children.add(NodeFactory.createNode(curToken));
        read();
        // '('
        children.add(NodeFactory.createNode(curToken));
        read();
        // ')'
        checkRParent(children);
    }

    // StartTimeStmt ==> 'starttime' '(' ')' ';'
    public void parseStartTimeStmt(ArrayList<Node> children) {
        // 'starttime'
        children.add(NodeFactory.createNode(curToken));
        read();
        // '('
        children.add(NodeFactory.createNode(curToken));
        read();
        // ')'
        checkRParent(children);
    }

    // PutFStmt ==> 'putf' '(' FormatString { ',' Exp } ')' ';'
    public void parsePutFStmt(ArrayList<Node> children) {
        // 'putf'
        children.add(NodeFactory.createNode(curToken));
        read();
        // '('
        children.add(NodeFactory.createNode(curToken));
        read();
        // FormatString
        children.add(parseFormatString());
        // { ',' Exp }
        // ','
        while (curToken.getTokenType() == TokenType.COMMA) {
            // ','
            children.add(NodeFactory.createNode(curToken));
            read();
            // Exp
            children.add(parseExp());
        }
        // ')'
        checkRParent(children);
    }

    // PutFloatStmt ==> 'putfloat' '(' Exp ')' ';'
    public void parsePutFloatStmt(ArrayList<Node> children) {
        // 'putfloat'
        children.add(NodeFactory.createNode(curToken));
        read();
        // '('
        children.add(NodeFactory.createNode(curToken));
        read();
        // Exp
        children.add(parseExp());
        // ')'
        checkRParent(children);
    }

    // PutIntStmt ==> 'putint' '(' Exp ')' ';'
    public void parsePutIntStmt(ArrayList<Node> children) {
        // 'putint'
        children.add(NodeFactory.createNode(curToken));
        read();
        // '('
        children.add(NodeFactory.createNode(curToken));
        read();
        // Exp
        children.add(parseExp());
        // ')'
        checkRParent(children);
    }

    // PutFArrayStmt ==> 'putfarray' '(' Exp ',' Exp { ',' Exp } ')' ';'
    public void parsePutFArrayStmt(ArrayList<Node> children) {
        // 'putfarray'
        children.add(NodeFactory.createNode(curToken));
        read();
        // '('
        children.add(NodeFactory.createNode(curToken));
        read();
        // Exp
        children.add(parseExp());
        // ','
        children.add(NodeFactory.createNode(curToken));
        read();
        // Exp
        children.add(parseExp());
        // { ','  Exp}
        while (curToken.getTokenType() == TokenType.COMMA) {
            // ','
            children.add(NodeFactory.createNode(curToken));
            read();
            // Exp
            children.add(parseExp());
        }
        // ')'
        checkRParent(children);
    }

    // PutArrayStmt ==> 'putarray' '(' Exp ',' Exp { ',' Exp} ')' ';'
    public void parsePutArrayStmt(ArrayList<Node> children) {
        // 'putarray'
        children.add(NodeFactory.createNode(curToken));
        read();
        // '('
        children.add(NodeFactory.createNode(curToken));
        read();
        // Exp
        children.add(parseExp());
        // ','
        children.add(NodeFactory.createNode(curToken));
        read();
        // Exp
        children.add(parseExp());
        // { ','  Exp}
        while (curToken.getTokenType() == TokenType.COMMA) {
            // ','
            children.add(NodeFactory.createNode(curToken));
            read();
            // Exp
            children.add(parseExp());
        }
        // ')'
        checkRParent(children);
    }


    // PutChStmt ==> 'putch' '(' Exp ')' ';'
    public void parsePutChStmt(ArrayList<Node> children) {
        // 'putch'
        read();
        // '('
        children.add(NodeFactory.createNode(curToken));
        read();
        // Exp
        children.add(parseExp());
        // ')'
        checkRParent(children);
    }


    // GetFloatStmt ==> LVal '=' 'getfloat' '(' ')' ';'
    public Node parseGetFloatStmt() {
        ArrayList<Node> children = new ArrayList<>();
        // LVal
        children.add(parseLVal());
        // '='
        children.add(NodeFactory.createNode(curToken));
        read();
        // 'getfloat'
        children.add(NodeFactory.createNode(curToken));
        read();
        // '('
        children.add(NodeFactory.createNode(curToken));
        read();
        // ')'
        checkRParent(children);
        // ';'
        checkSemicn(children);
        return NodeFactory.createNode(SyntaxType.ASSIGN_STMT, children);
    }

    // GetFArrayStmt ==> LVal '=' 'getfarray' '(' Exp ')' ';'
    public Node parseGetFArrayStmt() {
        ArrayList<Node> children = new ArrayList<>();
        // LVal
        children.add(parseLVal());
        // '='
        children.add(NodeFactory.createNode(curToken));
        read();
        // 'getfarray'
        children.add(NodeFactory.createNode(curToken));
        read();
        // '('
        children.add(NodeFactory.createNode(curToken));
        read();
        // Exp
        children.add(parseExp());
        // ')'
        checkRParent(children);
        // ';'
        checkSemicn(children);
        return NodeFactory.createNode(SyntaxType.ASSIGN_STMT, children);
    }

    // GetChStmt ==> LVal '=' 'getch' '(' ')' ';'
    public Node parseGetChStmt() {
        ArrayList<Node> children = new ArrayList<>();
        // LVal
        children.add(parseLVal());
        // '='
        children.add(NodeFactory.createNode(curToken));
        read();
        // 'getch'
        children.add(NodeFactory.createNode(curToken));
        read();
        // '('
        children.add(NodeFactory.createNode(curToken));
        read();
        // ')'
        checkRParent(children);
        // ';'
        checkSemicn(children);
        return NodeFactory.createNode(SyntaxType.ASSIGN_STMT, children);
    }


    // GetArrayStmt ==> LVal '=' 'getarray' '(' Exp ')' ';'
    public Node parseGetArrayStmt() {
        ArrayList<Node> children = new ArrayList<>();
        // LVal
        children.add(parseLVal());
        // '='
        children.add(NodeFactory.createNode(curToken));
        read();
        // 'getarray'
        children.add(NodeFactory.createNode(curToken));
        read();
        // '('
        children.add(NodeFactory.createNode(curToken));
        read();
        // Exp
        children.add(parseExp());
        // ')'
        checkRParent(children);
        // ';'
        checkSemicn(children);
        return NodeFactory.createNode(SyntaxType.ASSIGN_STMT, children);
    }


    // IfStmt ==> 'if' '(' Cond ')' Stmt [ 'else' Stmt ]
    public Node parseIfStmt() {
        ArrayList<Node> children = new ArrayList<>();
        // 'if'
        children.add(NodeFactory.createNode(curToken));
        read();
        // '('
        children.add(NodeFactory.createNode(curToken));
        read();
        // Cond
        children.add(parseCond());
        // ')'
        checkRParent(children);
        // Stmt
        children.add(parseStmt());
        // [ 'else' Stmt ]
        // 'else'
        if (curToken.getTokenType() == TokenType.ELSETK) {
            children.add(NodeFactory.createNode(curToken));
            read();
            children.add(parseStmt());
        }
        return NodeFactory.createNode(SyntaxType.IF_STMT, children);
    }

    // BlockStmt ==> Block
    public Node parseBlockStmt() {
        ArrayList<Node> children = new ArrayList<>();
        children.add(parseBlock());
        return NodeFactory.createNode(SyntaxType.BLOCK_STMT, children);
    }

    // BreakStmt ==> 'break' ';'
    public Node parseBreakStmt() {
        ArrayList<Node> children = new ArrayList<>();
        // 'break'
        children.add(NodeFactory.createNode(curToken));
        read();
        checkSemicn(children);
        return NodeFactory.createNode(SyntaxType.BREAK_STMT, children);
    }

    // ContinueStmt ==> 'continue' ';'
    public Node parseContinueStmt() {
        ArrayList<Node> children = new ArrayList<>();
        // 'continue'
        children.add(NodeFactory.createNode(curToken));
        read();
        // ';'
        checkSemicn(children);
        return NodeFactory.createNode(SyntaxType.CONTINUE_STMT, children);
    }

    // ReturnStmt ==> 'return' [ Exp ] ';'
    public Node parseReturnStmt() {
        ArrayList<Node> children = new ArrayList<>();
        // 'return'
        children.add(NodeFactory.createNode(curToken));
        read();
        // [ Exp ]
        // FIRST(Exp) = { '(' Ident IntConst '+' '-' }
        if (curToken.getTokenType() == TokenType.LPARENT
                || curToken.getTokenType() == TokenType.IDENFR
                || curToken.getTokenType() == TokenType.HEXFCON
                || curToken.getTokenType() == TokenType.DECFCON
                || curToken.getTokenType() == TokenType.HEXCON
                || curToken.getTokenType() == TokenType.OCTCON
                || curToken.getTokenType() == TokenType.DECCON
                || curToken.getTokenType() == TokenType.PLUS
                || curToken.getTokenType() == TokenType.MINU) {
            children.add(parseExp());
        }
        // ';'
        checkSemicn(children);
        return NodeFactory.createNode(SyntaxType.RETURN_STMT, children);
    }

    // ExpStmt ==> [ Exp ] ';'
    public Node parseExpStmt() {
        ArrayList<Node> children = new ArrayList<>();
        // [ Exp ]
        // FIRST(Exp) = { '(' Ident IntConst '+' '-' }
        if (curToken.getTokenType() == TokenType.LPARENT
                || curToken.getTokenType() == TokenType.IDENFR
                || curToken.getTokenType() == TokenType.HEXFCON
                || curToken.getTokenType() == TokenType.DECFCON
                || curToken.getTokenType() == TokenType.HEXCON
                || curToken.getTokenType() == TokenType.OCTCON
                || curToken.getTokenType() == TokenType.DECCON
                || curToken.getTokenType() == TokenType.PLUS
                || curToken.getTokenType() == TokenType.MINU) {
            children.add(parseExp());
        } else if (curToken.getTokenType() == TokenType.PUTARRAYTK || curToken.getTokenType() == TokenType.PUTFARRAYTK ||
                curToken.getTokenType() == TokenType.PUTFLOATTK || curToken.getTokenType() == TokenType.PUTINTTK ||
                curToken.getTokenType() == TokenType.PUTCHTK || curToken.getTokenType() == TokenType.STARTTIMETK ||
                curToken.getTokenType() == TokenType.STOPTIMETK || curToken.getTokenType() == TokenType.PUTFTK) {
            if (curToken.getTokenType() == TokenType.PUTARRAYTK) {
                parsePutArrayStmt(children);
            } else if (curToken.getTokenType() == TokenType.PUTFARRAYTK) {
                parsePutFArrayStmt(children);
            } else if (curToken.getTokenType() == TokenType.PUTFLOATTK) {
                parsePutFloatStmt(children);
            } else if (curToken.getTokenType() == TokenType.PUTINTTK) {
                parsePutIntStmt(children);
            }  else if (curToken.getTokenType() == TokenType.PUTCHTK) {
                parsePutChStmt(children);
            } else if (curToken.getTokenType() == TokenType.STARTTIMETK) {
                parseStartTimeStmt(children);
            } else if (curToken.getTokenType() == TokenType.STOPTIMETK) {
                parseStopTimeStmt(children);
            } else if (curToken.getTokenType() == TokenType.PUTFTK) {
                parsePutFStmt(children);
            }
        }
        // ';'
        checkSemicn(children);

        return NodeFactory.createNode(SyntaxType.EXP_STMT, children);
    }

    // GetIntStmt ==> LVal '=' 'getint' '(' ')' ';'
    public Node parseGetIntStmt() {
        ArrayList<Node> children = new ArrayList<>();
        curToken = tokenStream.backToSentry();
        // LVal
        children.add(parseLVal());
        // '='
        children.add(NodeFactory.createNode(curToken));
        read();
        // 'getint'
        children.add(NodeFactory.createNode(curToken));
        read();
        // '('
        children.add(NodeFactory.createNode(curToken));
        read();
        // ')'
        checkRParent(children);
        // ';'
        checkSemicn(children);
        return NodeFactory.createNode(SyntaxType.ASSIGN_STMT, children);
    }

    // AssignStmt ==> LVal '=' Exp ';'
    public Node parseAssignStmt() {
        ArrayList<Node> children = new ArrayList<>();
        // LVal
        children.add(parseLVal());
        // '='
        children.add(NodeFactory.createNode(curToken));
        read();
        // Exp
        children.add(parseExp());
        // ';'
        checkSemicn(children);
        return NodeFactory.createNode(SyntaxType.ASSIGN_STMT, children);
    }

    // Exp ==> AddExp
    public Node parseExp() {
        ArrayList<Node> children = new ArrayList<>();
        // AddExp
        children.add(parseAddExp());
        return NodeFactory.createNode(SyntaxType.EXP, children);
    }

    // Cond ==> LorExp
    public Node parseCond() {
        ArrayList<Node> children = new ArrayList<>();
        // LOrExp
        children.add(parseLOrExp());
        return NodeFactory.createNode(SyntaxType.COND, children);
    }

    // leaf !!!
    // with no children
    public Node parseFormatString() {
        // FormatString
        Node leaf = NodeFactory.createNode(curToken);
        read();
        return leaf;
    }

    // LVal ==> Ident { '[' Exp ']' }
    public Node parseLVal() {
        ArrayList<Node> children = new ArrayList<>();
        // Ident
        children.add(NodeFactory.createNode(curToken));
        read();
        // { '[' Exp ']' }
        // '['
        while (curToken.getTokenType() == TokenType.LBRACK) {
            // '['
            children.add(NodeFactory.createNode(curToken));
            read();
            // Exp
            children.add(parseExp());
            // ']'
            checkRBrack(children);
        }
        return NodeFactory.createNode(SyntaxType.LVAL, children);
    }

    // AddExp ==> MulExp { ( '+' | '-' ) MulExp }
    public Node parseAddExp() {
        ArrayList<Node> children = new ArrayList<>();
        // MulExp
        children.add(parseMulExp());
        // { ('+' | '-') }
        while (curToken.getTokenType() == TokenType.PLUS
                || curToken.getTokenType() == TokenType.MINU) {
            // upgrade to AddExp
            Printer.printSType(SyntaxType.ADD_EXP);
            // ('+' | '-')
            children.add(NodeFactory.createNode(curToken));
            read();
            // MulExp
            children.add(parseMulExp());
        }
        return NodeFactory.createNode(SyntaxType.ADD_EXP, children);
    }

    // MulExp ==> UnaryExp { ( '*' | '/' | '%' ) UnaryExp }
    public Node parseMulExp() {
        ArrayList<Node> children = new ArrayList<>();
        // UnaryExp
        children.add(parseUnaryExp());
        // { ( '*' | '/' | '%' ) UnaryExp }
        // ( '*' | '/' | '%' )
        while (curToken.getTokenType() == TokenType.MULT
                || curToken.getTokenType() == TokenType.DIV
                || curToken.getTokenType() == TokenType.MOD) {
            // upgrade to MulExp
            Printer.printSType(SyntaxType.MUL_EXP);
            // ( '*' | '/' | '%' )
            children.add(NodeFactory.createNode(curToken));
            read();
            // UnaryExp
            children.add(parseUnaryExp());
        }
        return NodeFactory.createNode(SyntaxType.MUL_EXP, children);
    }

    // UnaryExp ==> PrimaryExp | Ident '(' [ FuncRParams ] ')' | UnaryOp UnaryExp
    public Node parseUnaryExp() {
        ArrayList<Node> children = new ArrayList<>();
        // Ident '(' [ FuncRParams ] ')'
        if (curToken.getTokenType() == TokenType.IDENFR
                && tokenStream.lookForward().getTokenType() == TokenType.LPARENT) {
            // Ident
            children.add(NodeFactory.createNode(curToken));
            read();
            // '('
            children.add(NodeFactory.createNode(curToken));
            read();
            // [ FuncRParams ]
            // Exp
            // FIRST(Exp) = { '(' Ident IntConst '+' '-' }
            if (curToken.getTokenType() == TokenType.LPARENT
                    || curToken.getTokenType() == TokenType.IDENFR
                    || curToken.getTokenType() == TokenType.HEXFCON
                    || curToken.getTokenType() == TokenType.DECFCON
                    || curToken.getTokenType() == TokenType.HEXCON
                    || curToken.getTokenType() == TokenType.OCTCON
                    || curToken.getTokenType() == TokenType.DECCON
                    || curToken.getTokenType() == TokenType.PLUS
                    || curToken.getTokenType() == TokenType.MINU) {
                children.add(parseFuncRParams());
            }
            // ')'
            checkRParent(children);
        }
        // PrimaryExp
        // FIRST(PrimaryExp) =  { '(' Ident IntConst }
        else if (curToken.getTokenType() == TokenType.LPARENT
                || curToken.getTokenType() == TokenType.IDENFR
                || curToken.getTokenType() == TokenType.HEXFCON
                || curToken.getTokenType() == TokenType.DECFCON
                || curToken.getTokenType() == TokenType.HEXCON
                || curToken.getTokenType() == TokenType.OCTCON
                || curToken.getTokenType() == TokenType.DECCON) {
            children.add(parsePrimaryExp());
        }
        // UnaryOp UnaryExp
        else {
            children.add(parseUnaryOp());
            children.add(parseUnaryExp());
        }
        return NodeFactory.createNode(SyntaxType.UNARY_EXP, children);
    }

    // PrimaryExp ==> '(' Exp ')' | LVal | Number
    public Node parsePrimaryExp() {
        ArrayList<Node> children = new ArrayList<>();
        // '(' Exp ')'
        // '('
        if (curToken.getTokenType() == TokenType.LPARENT) {
            // '('
            children.add(NodeFactory.createNode(curToken));
            read();
            // Exp
            children.add(parseExp());
            // ')'
            checkRParent(children);
        }
        // DecConst
        else if (curToken.getTokenType() == TokenType.DECCON) {
            children.add(parseDecConst());
        }
        // OctConst
        else if (curToken.getTokenType() == TokenType.OCTCON) {
            children.add(parseOctConst());
        }
        // HexConst
        else if (curToken.getTokenType() == TokenType.HEXCON) {
            children.add(parseHexConst());
        }
        // DecFloatConst
        else if (curToken.getTokenType() == TokenType.DECFCON) {
            children.add(parseDecFloatConst());
        }
        // HexFloatConst
        else if (curToken.getTokenType() == TokenType.HEXFCON) {
            children.add(parseHexFloatConst());
        }
        // LVal
        else {
            children.add(parseLVal());
        }
        return NodeFactory.createNode(SyntaxType.PRIMARY_EXP, children);
    }

    // hexfloatconst
    public Node parseHexFloatConst() {
        ArrayList<Node> children = new ArrayList<>();
        // hexfloatconst
        children.add(NodeFactory.createNode(curToken));
        read();
        return NodeFactory.createNode(SyntaxType.HEX_FLOAT_CONST, children);
    }

    // decfloatconst
    public Node parseDecFloatConst() {
        ArrayList<Node> children = new ArrayList<>();
        // decfloatconst
        children.add(NodeFactory.createNode(curToken));
        read();
        return NodeFactory.createNode(SyntaxType.DEC_FLOAT_CONST, children);
    }

    // hexconst
    public Node parseHexConst() {
        ArrayList<Node> children = new ArrayList<>();
        // HexConst
        children.add(NodeFactory.createNode(curToken));
        read();
        return NodeFactory.createNode(SyntaxType.HEX_CONST, children);
    }

    // decconst
    public Node parseDecConst() {
        ArrayList<Node> children = new ArrayList<>();
        // DecConst
        children.add(NodeFactory.createNode(curToken));
        read();
        return NodeFactory.createNode(SyntaxType.DEC_CONST, children);
    }

    // octconst
    public Node parseOctConst() {
        ArrayList<Node> children = new ArrayList<>();
        // OctConst
        children.add(NodeFactory.createNode(curToken));
        read();
        return NodeFactory.createNode(SyntaxType.OCT_CONST, children);
    }

    // UnaryOp ==> '+' | '-' | '!'
    public Node parseUnaryOp() {
        ArrayList<Node> children = new ArrayList<>();
        // '+' | '-' | '!'
        children.add(NodeFactory.createNode(curToken));
        read();
        return NodeFactory.createNode(SyntaxType.UNARY_OP, children);
    }

    // LOrExp ==> LAndExp { '||' LAndExp }
    public Node parseLOrExp() {
        ArrayList<Node> children = new ArrayList<>();
        // LAndExp
        children.add(parseLAndExp());
        // { '||' LAndExp }
        // '||'
        while (curToken.getTokenType() == TokenType.OR) {
            // upgrade to LOrExp
            Printer.printSType(SyntaxType.LOR_EXP);
            // '||'
            children.add(NodeFactory.createNode(curToken));
            read();
            // LAndExp
            children.add(parseLAndExp());
        }
        return NodeFactory.createNode(SyntaxType.LOR_EXP, children);
    }

    // LAndExp ==> EqExp { '&&' EqExp }
    public Node parseLAndExp() {
        ArrayList<Node> children = new ArrayList<>();
        // EqExp
        children.add(parseEqExp());
        // { '&&' EqExp }
        // '&&'
        while (curToken.getTokenType() == TokenType.AND) {
            // upgrade to LAndExp
            Printer.printSType(SyntaxType.LAND_EXP);
            // '&&'
            children.add(NodeFactory.createNode(curToken));
            read();
            // EqExp
            children.add(parseEqExp());
        }
        return NodeFactory.createNode(SyntaxType.LAND_EXP, children);
    }

    // EqExp ==> RelExp { ( '==' | '!=' ) RelExp }
    public Node parseEqExp() {
        ArrayList<Node> children = new ArrayList<>();
        // RelExp
        children.add(parseRelExp());
        // { ( '==' | '!=' ) RelExp }
        // '==' | '!='
        while (curToken.getTokenType() == TokenType.EQL
                || curToken.getTokenType() == TokenType.NEQ) {
            // upgrade to EqExp
            Printer.printSType(SyntaxType.EQ_EXP);
            // '==' | '!='
            children.add(NodeFactory.createNode(curToken));
            read();
            // RelExp
            children.add(parseRelExp());
        }
        return NodeFactory.createNode(SyntaxType.EQ_EXP, children);
    }

    // RelExp ==> AddExp { ( '<' | '>' | '<=' | '>=' ) AddExp }
    public Node parseRelExp() {
        ArrayList<Node> children = new ArrayList<>();
        // AddExp
        children.add(parseAddExp());
        // { ( '<' | '>' | '<=' | '>=' ) AddExp }
        // '<' | '>' | '<=' | '>='
        while (curToken.getTokenType() == TokenType.LSS
                || curToken.getTokenType() == TokenType.LEQ
                || curToken.getTokenType() == TokenType.GRE
                || curToken.getTokenType() == TokenType.GEQ) {
            // upgrade to RelExp
            Printer.printSType(SyntaxType.REL_EXP);
            // '<' | '>' | '<=' | '>='
            children.add(NodeFactory.createNode(curToken));
            read();
            // AddExp
            children.add(parseAddExp());
        }
        return NodeFactory.createNode(SyntaxType.REL_EXP, children);
    }

    // FuncDef ==> FuncType Ident '(' [ FuncFParams ] ')' Block
    public Node parseFuncDef() {
        ArrayList<Node> children = new ArrayList<>();
        // FuncType
        children.add(parseFuncType());
        // Ident
        children.add(NodeFactory.createNode(curToken));
        read();
        // '('
        children.add(NodeFactory.createNode(curToken));
        read();
        // [ FuncFParams ]
        // FuncFParam
        // 'int'|'float'
        if (curToken.getTokenType() == TokenType.INTTK || curToken.getTokenType() == TokenType.FLOATTK) {
            children.add(parseFuncFParams());
        }
        // ')'
        checkRParent(children);
        // Block
        children.add(parseBlock());
        return NodeFactory.createNode(SyntaxType.FUNC_DEF, children);
    }

    // FuncType ==> 'void' | 'int'
    public Node parseFuncType() {
        ArrayList<Node> children = new ArrayList<>();
        // 'void' | 'int'
        children.add(NodeFactory.createNode(curToken));
        read();
        return NodeFactory.createNode(SyntaxType.FUNC_TYPE, children);
    }

    // FuncRParams ==> Exp { ',' Exp }
    public Node parseFuncRParams() {
        ArrayList<Node> children = new ArrayList<>();
        // Exp
        children.add(parseExp());
        // { ',' Exp }
        // ','
        while (curToken.getTokenType() == TokenType.COMMA) {
            // ','
            children.add(NodeFactory.createNode(curToken));
            read();
            // Exp
            children.add(parseExp());
        }
        return NodeFactory.createNode(SyntaxType.FUNC_R_PARAMS, children);
    }

    // FuncFParams ==> FuncFParam { ',' FuncFParam }
    public Node parseFuncFParams() {
        ArrayList<Node> children = new ArrayList<>();
        // FuncFParam
        children.add(parseFuncFParam());
        // { ',' FuncFParam }
        while (curToken.getTokenType() == TokenType.COMMA) {
            // ','
            children.add(NodeFactory.createNode(curToken));
            read();
            // FuncFParam
            children.add(parseFuncFParam());
        }
        return NodeFactory.createNode(SyntaxType.FUNC_F_PARAMS, children);
    }

    // FuncFParam ==> 'int' Ident [ '[' ']' { '[' ConstExp ']' } ]
    public Node parseFuncFParam() {
        ArrayList<Node> children = new ArrayList<>();
        // 'int'
        children.add(parseBType());
        // Ident
        children.add(NodeFactory.createNode(curToken));
        read();
        // [ '[' ']' { '[' ConstExp ']' } ]
        // '['
        if (curToken.getTokenType() == TokenType.LBRACK) {
            // '['
            children.add(NodeFactory.createNode(curToken));
            read();
            // ']'
            checkRBrack(children);
            // { '[' ConstExp ']' }
            // '['
            while (curToken.getTokenType() == TokenType.LBRACK) {
                // '['
                children.add(NodeFactory.createNode(curToken));
                read();
                // ConstExp
                children.add(parseConstExp());
                // ']'
                checkRBrack(children);
            }
        }
        return NodeFactory.createNode(SyntaxType.FUNC_F_PARAM, children);
    }

    // ConstDecl ==> 'const' 'int' ConstDef { ',' ConstDef } ';'
    public Node parseConstDecl() {
        ArrayList<Node> children = new ArrayList<>();
        // 'const'
        children.add(NodeFactory.createNode(curToken));
        read();
        // 'int'
        children.add(parseBType());
        // ConstDef
        children.add(parseConstDef());
        // { ',' ConstDef }
        // ','
        while (curToken.getTokenType() == TokenType.COMMA) {
            // ','
            children.add(NodeFactory.createNode(curToken));
            read();
            // ConstDef
            children.add(parseConstDef());
        }
        // ';'
        checkSemicn(children);
        return NodeFactory.createNode(SyntaxType.CONST_DECL, children);
    }

    // ConstExp ==> AddExp
    public Node parseConstExp() {
        ArrayList<Node> children = new ArrayList<>();
        // AddExp
        children.add(parseAddExp());
        return NodeFactory.createNode(SyntaxType.CONST_EXP, children);
    }

    // ConstDef ==> Ident { '[' ConstExp ']' } '=' ConstInitVal
    public Node parseConstDef() {
        ArrayList<Node> children = new ArrayList<>();
        // Ident
        children.add(NodeFactory.createNode(curToken));
        read();
        // { '[' ConstExp ']' }
        // '['
        while (curToken.getTokenType() == TokenType.LBRACK) {
            // '['
            children.add(NodeFactory.createNode(curToken));
            read();
            // ConstExp
            children.add(parseConstExp());
            // ']'
            checkRBrack(children);
        }
        // '='
        children.add(NodeFactory.createNode(curToken));
        read();
        // ConstInitVal
        children.add(parseConstInitVal());
        return NodeFactory.createNode(SyntaxType.CONST_DEF, children);
    }

    // ConstInitVal ==> ConstExp | '{' [ ConstInitVal { ',' ConstInitVal } ] '}'
    public Node parseConstInitVal() {
        ArrayList<Node> children = new ArrayList<>();
        // '{' [ ConstInitVal { ',' ConstInitVal } ] '}'
        // '{'
        if (curToken.getTokenType() == TokenType.LBRACE) {
            // '{'
            children.add(NodeFactory.createNode(curToken));
            read();
            // ! '}'
            // [ ConstInitVal { ',' ConstInitVal } ]
            if (curToken.getTokenType() != TokenType.RBRACE) {
                // ConstInitVal
                children.add(parseConstInitVal());
                // { ',' ConstInitVal }
                while (curToken.getTokenType() == TokenType.COMMA) {
                    // ','
                    children.add(NodeFactory.createNode(curToken));
                    read();
                    // ConstInitVal
                    children.add(parseConstInitVal());
                }
            }
            // '}'
            children.add(NodeFactory.createNode(curToken));
            read();
        }
        // ConstExp
        else {
            children.add(parseConstExp());
        }
        return NodeFactory.createNode(SyntaxType.CONST_INIT_VAL, children);
    }

    // VarDecl ==> BType VarDef { ',' VarDef } ';'
    public Node parseVarDecl() {
        ArrayList<Node> children = new ArrayList<>();
        // BType
        children.add(parseBType());
        // VarDef
        children.add(parseVarDef());
        // { ',' VarDef }
        // ','
        while (curToken.getTokenType() == TokenType.COMMA) {
            // ','
            children.add(NodeFactory.createNode(curToken));
            read();
            // VarDef
            children.add(parseVarDef());
        }
        // ';'
        checkSemicn(children);
        return NodeFactory.createNode(SyntaxType.VAR_DECL, children);
    }

    public void parseDeclGetArray(ArrayList<Node> children) {
        // 'getarray'
        children.add(NodeFactory.createNode(curToken));
        read();
        // '('
        children.add(NodeFactory.createNode(curToken));
        read();
        // Exp
        children.add(parseExp());
        // ')'
        checkRParent(children);
    }

    public void parseDeclGetFArray(ArrayList<Node> children) {
        // 'getfarray'
        children.add(NodeFactory.createNode(curToken));
        read();
        // '('
        children.add(NodeFactory.createNode(curToken));
        read();
        // Exp
        children.add(parseExp());
        // ')'
        checkRParent(children);
    }

    public void parseDeclGetFloat(ArrayList<Node> children) {
        // 'getfloat'
        children.add(NodeFactory.createNode(curToken));
        read();
        // '('
        children.add(NodeFactory.createNode(curToken));
        read();
        // ')'
        checkRParent(children);
    }

    public void parseDeclGetChtk(ArrayList<Node> children) {
        // 'getch'
        children.add(NodeFactory.createNode(curToken));
        read();
        // '('
        children.add(NodeFactory.createNode(curToken));
        read();
        // ')'
        checkRParent(children);
    }

    public void parseDeclGetInt(ArrayList<Node> children) {
        // 'getint'
        children.add(NodeFactory.createNode(curToken));
        read();
        // '('
        children.add(NodeFactory.createNode(curToken));
        read();
        // ')'
        checkRParent(children);
    }

    // VarDef ==> Ident { '[' ConstExp ']' } [ '=' InitVal ]
    public Node parseVarDef() {
        ArrayList<Node> children = new ArrayList<>();
        // Ident
        children.add(NodeFactory.createNode(curToken));
        read();
        // { '[' ConstExp ']' }
        // '['
        while (curToken.getTokenType() == TokenType.LBRACK) {
            // '['
            children.add(NodeFactory.createNode(curToken));
            read();
            // ConstExp
            children.add(parseConstExp());
            // ']'
            checkRBrack(children);
        }
        // [ '=' InitVal ]
        // '='
        if (curToken.getTokenType() == TokenType.ASSIGN) {
            // '='
            children.add(NodeFactory.createNode(curToken));
            read();
            // InitVal
            if (curToken.getTokenType() == TokenType.GETARRAYTK) {
                parseDeclGetArray(children);
            } else if (curToken.getTokenType() == TokenType.GETFARRAY) {
                parseDeclGetFArray(children);
            } else if (curToken.getTokenType() == TokenType.GETFLOATTK) {
                parseDeclGetFloat(children);
            } else if (curToken.getTokenType() == TokenType.GETCHTK) {
                parseDeclGetChtk(children);
            } else if (curToken.getTokenType() == TokenType.GETINTTK) {
                parseDeclGetInt(children);
            } else {
                children.add(parseInitVal());
            }
        }
        return NodeFactory.createNode(SyntaxType.VAR_DEF, children);
    }

    // InitVal ==> Exp | '{' [ InitVal { ',' InitVal } ] '}'
    public Node parseInitVal() {
        ArrayList<Node> children = new ArrayList<>();
        // '{' [ InitVal { ',' InitVal } ] '}'
        if (curToken.getTokenType() == TokenType.LBRACE) {
            // '{'
            children.add(NodeFactory.createNode(curToken));
            read();
            // [ InitVal { ',' InitVal } ]
            // ! '}'
            if (curToken.getTokenType() != TokenType.RBRACE) {
                // InitVal
                children.add(parseInitVal());
                // { ',' InitVal }
                while (curToken.getTokenType() == TokenType.COMMA) {
                    // ','
                    children.add(NodeFactory.createNode(curToken));
                    read();
                    // InitVal
                    children.add(parseInitVal());
                }
            }
            // '}'
            children.add(NodeFactory.createNode(curToken));
            read();
        }
        // Exp
        else {
            children.add(parseExp());
        }
        return NodeFactory.createNode(SyntaxType.INIT_VAL, children);
    }

    private void checkRParent(ArrayList<Node> children) {
        // ')'
        if (curToken.getTokenType() == TokenType.RPARENT) {
            children.add(NodeFactory.createNode(curToken));
            read();
        }
        // ! ')'
        else {
            Printer.recordError(getLine(children), ErrorType.j);
        }
    }

    private void checkSemicn(ArrayList<Node> children) {
        // ';'
        if (curToken.getTokenType() == TokenType.SEMICN) {
            children.add(NodeFactory.createNode(curToken));
            read();
        }
        // ! ';'
        else {
            Printer.recordError(getLine(children), ErrorType.i);
        }
    }

    private void checkRBrack(ArrayList<Node> children) {
        // ']'
        if (curToken.getTokenType() == TokenType.RBRACK) {
            children.add(NodeFactory.createNode(curToken));
            read();
        }
        // ! ']'
        else {
            Printer.recordError(getLine(children), ErrorType.k);
        }
    }

    public int getLine(ArrayList<Node> children) {
        Node regNode = children.get(children.size() - 1);
        while (!(regNode instanceof TokenNode)) {
            children = regNode.getChildren();
            regNode = children.get(children.size() - 1);
        }
        return ((TokenNode) regNode).getToken().getLine();
    }

    public Node parseBType() {
        ArrayList<Node> children = new ArrayList<>();
        // 'int' | 'float'
        children.add(NodeFactory.createNode(curToken));
        read();
        return NodeFactory.createNode(SyntaxType.B_TYPE, children);
    }

}
