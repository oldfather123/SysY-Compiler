package frontend.syntax;

import frontend.lexer.Token;
import frontend.lexer.TokenList;
import frontend.lexer.TokenType;
import frontend.syntax.ast.*;
import frontend.syntax.ast.nodes.*;
import frontend.syntax.ast.nodes.block.Block;
import frontend.syntax.ast.nodes.block.IBlockItem;
import frontend.syntax.ast.nodes.block.IStmt;
import frontend.syntax.ast.nodes.declanddef.*;
import frontend.syntax.ast.nodes.expression.*;
import frontend.syntax.ast.nodes.expression.ASTNumber;

import java.util.ArrayList;
import java.util.List;

public class SyntaxParser {
    private static final SyntaxParser SYNTAX_PARSER = new SyntaxParser();
    private TokenList tokenList;
    
    private SyntaxParser() {}
    
    public static SyntaxParser getInstance() {
        return SYNTAX_PARSER;
    }
    
    public AST parse(TokenList tokenList) {
        if (tokenList == null) {
            throw new NullPointerException();
        }
        this.tokenList = tokenList;
        ArrayList<Decl>    declList = new ArrayList<>();
        ArrayList<FuncDef> funcDefList = new ArrayList<>();
        FuncDef mainFuncDef = null;
        
        while (tokenList.hasNext()) {
            if (tokenList.preview(2).getType() == TokenType.LPARENT) {
                // 出现括号说明是函数定义
                if (tokenList.preview(1).getContent().equals("main")) {
                    // 主函数
                    mainFuncDef = parseMainFuncDef();
                    break;
                } else {
                    // 普通函数
                    funcDefList.add(parseFuncDef());
                }
            } else {
                // 对象声明
                declList.add(parseDecl());
            }
        }
        
        if (mainFuncDef == null) {
            throw new RuntimeException("No main function?");
        }
        
        return new AST(new CompUnit(declList, funcDefList, mainFuncDef, tokenList.getLatestLineno()));
    }
    
    private FuncDef parseMainFuncDef() {
        tokenList.passTokens(TokenType.INT, TokenType.IDENT, TokenType.LPARENT, TokenType.RPARENT);
        int lineno = tokenList.getLatestLineno();
        Block block = parseBlock();
        return new FuncDef(AST.FuncType.INT, "main", new ArrayList<>(), block, lineno, tokenList.getLatestLineno());
    }
    
    private FuncDef parseFuncDef() {
        Token typeToken = tokenList.getNextToken(TokenType.INT, TokenType.VOID, TokenType.FLOAT);
        AST.FuncType funcType = AST.FuncType.token2FuncType(typeToken);
        
        String ident = tokenList.getNextToken(TokenType.IDENT).getContent();
        int lineno = tokenList.getLatestLineno();
        
        ArrayList<FuncFParam> params = new ArrayList<>();
        tokenList.passNextToken(TokenType.LPARENT);
        TokenType curTT = tokenList.preview(0).getType();
        if (curTT != TokenType.RPARENT && curTT != TokenType.LBRACE) {
            params = parseFuncFParams();
        }
        tokenList.passNextToken(TokenType.RPARENT);
        
        Block block = parseBlock();
        return new FuncDef(funcType, ident, params, block, lineno, tokenList.getLatestLineno());
    }
    
    private Block parseBlock() {
        ArrayList<IBlockItem> blockItems = new ArrayList<>();
        tokenList.passNextToken(TokenType.LBRACE);
        while (tokenList.preview(0).getType() != TokenType.RBRACE) {
            blockItems.add(
                    switch (tokenList.preview(0).getType()) {
                        case INT, FLOAT, CONST -> parseDecl();
                        default -> parseStmt();
                    }
            );
        }
        tokenList.passNextToken(TokenType.RBRACE);
        return new Block(blockItems, tokenList.getLatestLineno());
    }
    
    private IStmt parseStmt() {
        TokenType firstTokenType = tokenList.preview(0).getType();
        return switch (firstTokenType) {
            case LBRACE     -> parseBlock();
            case IF         -> parseIfStmt();
            case WHILE      -> parseWhileLoopStmt();
            case RETURN     -> parseReturnStmt();
            case BREAK      -> parseBreakStmt();
            case CONTINUE   -> parseContinueStmt();
            case IDENT      -> parseAssignOrExpStmt();    // assign中包括了两个输入函数
            default         -> parseExpStmt();          // exp 部分可以为空
        };
    }
    
    private IStmt.ExpStmt parseExpStmt() {
        IExp exp = null;
        if (tokenList.preview(0).getType() != TokenType.SEMI) {
            try {
                exp =  parseBinaryExp(AST.BinaryExpType.ADD);
            } catch (UnexpectedToken unexpectedToken) {
                throw new RuntimeException(unexpectedToken.toString());
            }
        }
        tokenList.passNextToken(TokenType.SEMI);
        return new IStmt.ExpStmt(exp, tokenList.getLatestLineno());
    }
    
    private IStmt parseAssignOrExpStmt() {
        int step = 0;
        if (tokenList.preview(step).getType() != TokenType.IDENT) {
            throw new RuntimeException("这个方法就得满足下一个 token 是标识符");
        }
        step++;
        TokenType type = tokenList.preview(step).getType();
        while (type != TokenType.ASSIGN && type != TokenType.SEMI) {
            step++;
            type = tokenList.preview(step).getType();
        }
        if (type == TokenType.ASSIGN) {
            return parseAssignStmt();
        } else {
            return parseExpStmt();
        }
    }
    
    private IStmt.AssignStmt parseAssignStmt() {
        LVal lVal = parseLVal();
        final int lineno = tokenList.getLatestLineno();
        tokenList.passNextToken(TokenType.ASSIGN);
        IExp exp;
        tokenList.preview(0);
        try {
            exp = parseBinaryExp(AST.BinaryExpType.ADD);
        } catch (UnexpectedToken unexpectedToken) {
            throw new RuntimeException(unexpectedToken.toString());
        }
        tokenList.passNextToken(TokenType.SEMI);
        return new IStmt.AssignStmt(lVal, exp, lineno);
    }
    
    private IStmt.ContinueStmt parseContinueStmt() {
        tokenList.passTokens(TokenType.CONTINUE, TokenType.SEMI);
        return new IStmt.ContinueStmt(tokenList.getLatestLineno());
    }
    
    private IStmt.BreakStmt parseBreakStmt() {
        tokenList.passTokens(TokenType.BREAK, TokenType.SEMI);
        return new IStmt.BreakStmt(tokenList.getLatestLineno());
    }
    
    private IStmt.ReturnStmt parseReturnStmt() {
        tokenList.passNextToken(TokenType.RETURN);
        final int lineno = tokenList.getLatestLineno();
        IExp retVal = null;
        if (tokenList.preview(0).getType() != TokenType.SEMI) {
            int curHeadPos = tokenList.getCurHead();
            try {
                retVal = parseBinaryExp(AST.BinaryExpType.ADD);
            } catch (UnexpectedToken unexpectedToken) {
                tokenList.setCurHead(curHeadPos);
            }
        }
        tokenList.passNextToken(TokenType.SEMI);
        return new IStmt.ReturnStmt(retVal, lineno);
    }
    
    private IStmt.WhileLoopStmt parseWhileLoopStmt() {
        tokenList.passNextToken(TokenType.WHILE);
        tokenList.passNextToken(TokenType.LPARENT);
        
        IExp cond;
        try {
            cond = parseBinaryExp(AST.BinaryExpType.LOR);
        } catch (UnexpectedToken unexpectedToken) {
            throw new RuntimeException(unexpectedToken.toString());
        }
        tokenList.passNextToken(TokenType.RPARENT);
        IStmt body = parseStmt();
        
        return new IStmt.WhileLoopStmt(cond, body, tokenList.getLatestLineno());
    }
    
    private IStmt.IfStmt parseIfStmt() {
        tokenList.passNextToken(TokenType.IF);
        tokenList.passNextToken(TokenType.LPARENT);
        BinaryExp cond;
        try {
            cond = parseBinaryExp(AST.BinaryExpType.LOR);
        } catch (UnexpectedToken unexpectedToken) {
            throw new RuntimeException(unexpectedToken.toString());
        }
        tokenList.passNextToken(TokenType.RPARENT);
        
        IStmt thenStmt = parseStmt();
        IStmt elseStmt = null;
        if (tokenList.preview(0).getType() == TokenType.ELSE) {
            tokenList.passNextToken(TokenType.ELSE);
            elseStmt = parseStmt();
        }
        return new IStmt.IfStmt(cond, thenStmt, elseStmt, tokenList.getLatestLineno());
    }
    
    private ArrayList<FuncFParam> parseFuncFParams() {
        ArrayList<FuncFParam> params = new ArrayList<>();
        params.add(parseFuncFParam());
        while (tokenList.preview(0).getType() == TokenType.COMMA) {
            tokenList.passNextToken(TokenType.COMMA);
            params.add(parseFuncFParam());
        }
        return params;
    }
    
    private FuncFParam parseFuncFParam() {
        AST.BType type = AST.BType.token2Btype(tokenList.getNextToken(TokenType.INT, TokenType.FLOAT));
        String ident = tokenList.getNextToken(TokenType.IDENT).getContent();
        final int lineno = tokenList.getLatestLineno();
        List<IExp> limList = null;
        if (tokenList.preview(0).getType() == TokenType.LBRACK) {
            tokenList.passNextToken(TokenType.LBRACK);
            limList = new ArrayList<>();
            tokenList.passNextToken(TokenType.RBRACK);
            while (tokenList.preview(0).getType() == TokenType.LBRACK) {
                tokenList.passNextToken(TokenType.LBRACK);
                try {
                    limList.add(parseBinaryExp(AST.BinaryExpType.ADD));
                } catch (UnexpectedToken e) {
                    throw new RuntimeException(e);
                }
                tokenList.passNextToken(TokenType.RBRACK);
            }
        }
        return new FuncFParam(type, ident, limList, lineno);
    }
    
    private Decl parseDecl() {
        ArrayList<Def> defs = new ArrayList<>();
        boolean isConst = false;
        if (tokenList.preview(0).getType() == TokenType.CONST) {
            isConst = true;
            tokenList.passNextToken(TokenType.CONST);
        }
        
        Token type = tokenList.getNextToken(TokenType.INT, TokenType.FLOAT);
        AST.BType bType = AST.BType.token2Btype(type);
        
        defs.add(parseDef(isConst));
        while (tokenList.preview(0).getType() == TokenType.COMMA) {
            tokenList.passNextToken(TokenType.COMMA);
            defs.add(parseDef(isConst));
        }
        
        tokenList.passNextToken(TokenType.SEMI);
        
        return new Decl(isConst, bType, defs, tokenList.getLatestLineno());
    }
    
    private Def parseDef(boolean isConst) {
        String ident = tokenList.getNextToken(TokenType.IDENT).getContent();
        final int lineno = tokenList.getLatestLineno();
        List<BinaryExp> limitList = new ArrayList<>();
        IInitVal initVal = null;
        
        while (tokenList.preview(0).getType() == TokenType.LBRACK) {
            tokenList.passNextToken(TokenType.LBRACK);
            try {
                limitList.add(parseBinaryExp(AST.BinaryExpType.ADD));
            } catch (UnexpectedToken unexpectedToken) {
                throw new RuntimeException(unexpectedToken.toString());
            }
            tokenList.passNextToken(TokenType.RBRACK);
        }
        
        if (tokenList.preview(0).getType() == TokenType.ASSIGN) {
            tokenList.passNextToken(TokenType.ASSIGN);
            initVal = parseInitVal(isConst);
        }
        
        return new Def(isConst, ident, limitList, initVal, lineno);
    }
    
    private IInitVal parseInitVal(boolean isConst) {
        TokenType type = tokenList.preview(0).getType();
        IInitVal initVal;
        if (type == TokenType.LBRACE) {
            initVal = parseInitArray(isConst);
        } else {
            try {
                initVal = parseBinaryExp(AST.BinaryExpType.ADD);
            } catch (UnexpectedToken unexpectedToken) {
                throw new RuntimeException(unexpectedToken.toString());
            }
        }
        return initVal;
    }
    
    private IInitVal parseInitArray(boolean isConst) {
        ArrayList<IInitVal> initValList = new ArrayList<>();
        tokenList.passNextToken(TokenType.LBRACE);
        if (tokenList.preview(0).getType() != TokenType.RBRACE) {
            initValList.add(parseInitVal(isConst));
            while (tokenList.preview(0).getType() == TokenType.COMMA) {
                tokenList.passNextToken(TokenType.COMMA);
                initValList.add(parseInitVal(isConst));
            }
        }
        tokenList.passNextToken(TokenType.RBRACE);
        return new InitArray(initValList, tokenList.getLatestLineno());
    }
    
    private BinaryExp parseBinaryExp(AST.BinaryExpType binaryExpType) throws UnexpectedToken {
        IExp firstExp = (binaryExpType == AST.BinaryExpType.MUL) ?
                parseUnaryExp() : parseBinaryExp(binaryExpType.getDownType());
        ArrayList<AST.OperatorName> ops = new ArrayList<>();
        ArrayList<IExp> restExps = new ArrayList<>();
        while (binaryExpType.containsOp(tokenList.preview(0).getType())) {
            ops.add(AST.OperatorName.token2OperatorName(tokenList.getNextToken()));
            restExps.add((binaryExpType == AST.BinaryExpType.MUL) ?
                    parseUnaryExp() : parseBinaryExp(binaryExpType.getDownType()));
        }
        return new BinaryExp(firstExp, ops, restExps, tokenList.getLatestLineno());
    }
    
    private IExp parseUnaryExp() throws UnexpectedToken {
        ArrayList<AST.OperatorName> ops = new ArrayList<>();
        Token nextTk = tokenList.preview(0);
        while (nextTk.isUnaryOpTk()) {
            ops.add(AST.OperatorName.token2OperatorName(tokenList.getNextToken()));
            nextTk = tokenList.preview(0);
        }
        IPrimaryExp primaryExp = parsePrimaryExp();
        return new UnaryExp(ops, primaryExp, tokenList.getLatestLineno());
    }
    
    private IPrimaryExp parsePrimaryExp() throws UnexpectedToken {
        TokenType firstTokenType = tokenList.preview(0).getType();
        IPrimaryExp primaryExp;
        if (firstTokenType == TokenType.LPARENT) {
            tokenList.passNextToken(TokenType.LPARENT);
            BinaryExp exp = parseBinaryExp(AST.BinaryExpType.ADD);
            tokenList.passNextToken(TokenType.RPARENT);
            primaryExp = exp;
        } else if (firstTokenType.isNumConst()) {
            primaryExp = parseNumConst();
        } else if (firstTokenType == TokenType.IDENT) {
            if (tokenList.preview(1).getType() == TokenType.LPARENT) {
                return parseCall(); // 文法定义中并没有把函数调用归类为基础表达式
            } else {
                primaryExp = parseLVal();
            }
        } else {
            throw new UnexpectedToken(tokenList.preview(0), "基本表达式");
        }
        return primaryExp;
    }
    
    private LVal parseLVal() {
        String ident = tokenList.getNextToken(TokenType.IDENT).getContent();
        int lineno = tokenList.getLatestLineno();
        List<IExp> limitList = new ArrayList<>();
        while (tokenList.preview(0).getType() == TokenType.LBRACK) {
            tokenList.passNextToken(TokenType.LBRACK);
            if (tokenList.preview(0).getType() != TokenType.SEMI) { // 特判一下 `x[;`的错误处理
                try {
                    limitList.add(parseBinaryExp(AST.BinaryExpType.ADD));
                } catch (UnexpectedToken unexpectedToken) {
                    throw new RuntimeException(unexpectedToken.toString());
                }
            }
            tokenList.passNextToken(TokenType.RBRACK);
        }
        return new LVal(ident, limitList, lineno);
    }
    
    private Call parseCall() {
        String ident = tokenList.getNextToken(TokenType.IDENT).getContent();
        int lineno = tokenList.getLatestLineno();
        ArrayList<BinaryExp> rParams = new ArrayList<>();
        tokenList.passNextToken(TokenType.LPARENT);
        TokenType nextType = tokenList.preview(0).getType();
        if (nextType != TokenType.RPARENT && nextType != TokenType.SEMI) {  // 特判一下 `f(;` 的错误处理
            int curHeadPos = tokenList.getCurHead();
            try {
                rParams.add(parseBinaryExp(AST.BinaryExpType.ADD));
            } catch (UnexpectedToken unexpectedToken) {
                tokenList.setCurHead(curHeadPos);
                tokenList.passNextToken(TokenType.RPARENT);
                return new Call(ident, rParams, lineno);
            }
            while (tokenList.preview(0).getType() == TokenType.COMMA) {
                tokenList.passNextToken(TokenType.COMMA);
                curHeadPos = tokenList.getCurHead();
                try {
                    rParams.add(parseBinaryExp(AST.BinaryExpType.ADD));
                } catch (UnexpectedToken unexpectedToken) {
                    tokenList.setCurHead(curHeadPos);
                    break;
                }
            }
        }
        tokenList.passNextToken(TokenType.RPARENT);
        return new Call(ident, rParams, lineno);
    }
    
    private ASTNumber parseNumConst() {
        Token number = tokenList.getNextToken();
        TokenType numType = number.getType();
        
        if (numType == TokenType.HEX_FLOAT_CON || numType == TokenType.DEC_FLOAT_CON) {
            float floatConst = Float.parseFloat(number.getContent());
            return new ASTNumber.FloatNumber(floatConst, tokenList.getLatestLineno());
        } else {
            long intConst = switch (numType) {
                case HEX_INT_CON -> Long.parseLong(number.getContent().substring(2), 16);
                case OCT_INT_CON -> Long.parseLong(number.getContent().substring(1), 8);
                case DEC_INT_CON -> Long.parseLong(number.getContent(), 10);
                default -> throw new RuntimeException("你是什么数字" + number);
            };
            return new ASTNumber.IntNumber(intConst, tokenList.getLatestLineno());
        }
        
        
    }
    
    public static class UnexpectedToken extends Exception {
        private final Token token;
        private final String expected;
        
        public UnexpectedToken(Token token, String expected) {
            this.token = token;
            this.expected = expected;
        }
        
        @Override
        public String toString() {
            return "unexpected " + token.getType() + " at line " + token.getLineno()
                    + " when expecting " + expected;
        }
    }
}
