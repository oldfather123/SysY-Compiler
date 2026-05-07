package frontend.syntax;

import frontend.lexer.Token;
import frontend.lexer.TokenList;
import frontend.lexer.TokenType;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;

public class Parser {
    private final TokenList tokenList;
    private static final HashMap<BinaryExpType, BinaryExpType> downMap = new HashMap<>();

    static {
        downMap.put(BinaryExpType.LOR, BinaryExpType.LAND);
        downMap.put(BinaryExpType.LAND, BinaryExpType.EQ);
        downMap.put(BinaryExpType.EQ, BinaryExpType.REL);
        downMap.put(BinaryExpType.REL, BinaryExpType.ADD);
        downMap.put(BinaryExpType.ADD, BinaryExpType.MUL);
    }

    public Parser(TokenList tokenList) {
        this.tokenList = tokenList;
    }

    public Ast parseAst() {
        ArrayList<Ast.CompUnit> nodes = new ArrayList<>();
        while (tokenList.hasNext()) {
            if (tokenList.checkAheadChar(2).getType() == TokenType.LPARENT) {
                nodes.add(parseFuncDef());
            } else {
                nodes.add(parseDecl());
            }
        }
        return new Ast(nodes);
    }

    private Ast.FuncDef parseFuncDef() {
        Token type = tokenList.getCharWithJudge(TokenType.INT, TokenType.VOID, TokenType.FLOAT);
        Token ident = tokenList.getCharWithJudge(TokenType.IDENT);
        ArrayList<Ast.FuncFParam> params = new ArrayList<>();
        tokenList.getCharWithJudge(TokenType.LPARENT);
        if (!(tokenList.checkAheadChar(0).getType() == TokenType.RPARENT)) {
            params = parseFuncFParams();
        }
        tokenList.getCharWithJudge(TokenType.RPARENT);
        Ast.Block block = parseBlock();
        return new Ast.FuncDef(type, ident, params, block);
    }

    private ArrayList<Ast.FuncFParam> parseFuncFParams() {
        ArrayList<Ast.FuncFParam> params = new ArrayList<>();
        params.add(parseSingleParam());
        while (tokenList.hasNext() && tokenList.checkAheadChar(0).getType() == TokenType.COMMA) {
            tokenList.getChar();
            params.add(parseSingleParam());
        }
        return params;
    }

    private Ast.FuncFParam parseSingleParam() {
        Token type = tokenList.getCharWithJudge(TokenType.INT, TokenType.VOID, TokenType.FLOAT);
        Token ident = tokenList.getCharWithJudge(TokenType.IDENT);
        boolean isArrayItem = false;
        ArrayList<Ast.Exp> arrayItemList = new ArrayList<>();
        while (tokenList.hasNext() && tokenList.checkAheadChar(0).getType() == TokenType.LBRACK) {
            isArrayItem = true;
            //TODO: 一维索引有数的情况是否需要处理？
            tokenList.getCharWithJudge(TokenType.LBRACK);
            tokenList.getCharWithJudge(TokenType.RBRACK);
            while (tokenList.hasNext() && tokenList.checkAheadChar(0).getType() == TokenType.LBRACK) {
                tokenList.getCharWithJudge(TokenType.LBRACK);
                arrayItemList.add(parseBinaryExp(BinaryExpType.ADD));
                tokenList.getCharWithJudge(TokenType.RBRACK);
            }
        }
        return new Ast.FuncFParam(type, ident, isArrayItem, arrayItemList);
    }

    private Ast.BinaryExp parseBinaryExp(BinaryExpType type) {
        Ast.Exp firstExp = (type == BinaryExpType.MUL) ? parseUnaryExp() : parseBinaryExp(downMap.get(type));
        ArrayList<Token> ops = new ArrayList<>();
        ArrayList<Ast.Exp> RestExps = new ArrayList<>();
        while (tokenList.hasNext() && type.contains(tokenList.checkAheadChar(0).getType())) {
            ops.add(tokenList.getChar());
            RestExps.add((type == BinaryExpType.MUL) ? parseUnaryExp() : parseBinaryExp(downMap.get(type)));
        }
        return new Ast.BinaryExp(firstExp, ops, RestExps);
    }

    private Ast.UnaryExp parseUnaryExp() {
        ArrayList<Token> ops = new ArrayList<>();
        while (tokenList.checkAheadChar(0).getType() == TokenType.ADD ||
                tokenList.checkAheadChar(0).getType() == TokenType.SUB ||
                tokenList.checkAheadChar(0).getType() == TokenType.NOT) {
            ops.add(tokenList.getChar());
        }
        Ast.PrimaryExp primaryExp = parsePrimaryExp();
        return new Ast.UnaryExp(ops, primaryExp);
    }

    private Ast.PrimaryExp parsePrimaryExp() {
        Token firstToken = tokenList.checkAheadChar(0);
        if (firstToken.getType() == TokenType.LPARENT) {
            tokenList.getChar();
            Ast.Exp exp = parseBinaryExp(BinaryExpType.ADD);
            tokenList.getCharWithJudge(TokenType.RPARENT);
            return exp;
        } else if (firstToken.getType() == TokenType.HEX_INT ||
                firstToken.getType() == TokenType.OCT_INT ||
                firstToken.getType() == TokenType.DEC_INT ||
                firstToken.getType() == TokenType.HEX_FLOAT ||
                firstToken.getType() == TokenType.DEC_FLOAT) {
            Token num = tokenList.getChar();
            return new Ast.Number(num);
        } else if (firstToken.getType() == TokenType.IDENT && tokenList.checkAheadChar(1).getType() == TokenType.LPARENT) {
            return parseFuncCall();
        } else {
            return parseLVal();
        }
    }

    private Ast.Call parseFuncCall() {
        Token ident = tokenList.getCharWithJudge(TokenType.IDENT);
        ArrayList<Ast.Exp> args = new ArrayList<>();
        tokenList.getCharWithJudge(TokenType.LPARENT);
        if (tokenList.checkAheadChar(0).getType() != TokenType.RPARENT) {
            args.add(parseBinaryExp(BinaryExpType.ADD));
            while (tokenList.checkAheadChar(0).getType() == TokenType.COMMA) {
                tokenList.getChar();
                args.add(parseBinaryExp(BinaryExpType.ADD));
            }
        }
        int lineno = tokenList.getCharWithJudge(TokenType.RPARENT).getLineno();
        return new Ast.Call(ident, args, lineno);
    }

    private Ast.LVal parseLVal() {
        Token ident = tokenList.getCharWithJudge(TokenType.IDENT);
        ArrayList<Ast.Exp> indexList = new ArrayList<>();
        while (tokenList.hasNext() && tokenList.checkAheadChar(0).getType() == TokenType.LBRACK) {
            tokenList.getCharWithJudge(TokenType.LBRACK);
            indexList.add(parseBinaryExp(BinaryExpType.ADD));
            tokenList.getCharWithJudge(TokenType.RBRACK);
        }
        return new Ast.LVal(ident, indexList);
    }

    private Ast.Block parseBlock() {
        ArrayList<Ast.BlockItem> blockItems = new ArrayList<>();
        tokenList.getCharWithJudge(TokenType.LBRACE);
        while (tokenList.checkAheadChar(0).getType() != TokenType.RBRACE) {
            blockItems.add(parseBlockItem());
        }
        tokenList.getCharWithJudge(TokenType.RBRACE);
        return new Ast.Block(blockItems);
    }

    private Ast.BlockItem parseBlockItem() {
        if (tokenList.checkAheadChar(0).getType() == TokenType.INT ||
                tokenList.checkAheadChar(0).getType() == TokenType.FLOAT ||
                tokenList.checkAheadChar(0).getType() == TokenType.CONST) {
            return parseDecl();
        } else {
            return parseStmt();
        }
    }

    private Ast.Decl parseDecl() {
        ArrayList<Ast.Def> defs = new ArrayList<>();
        boolean isConst = false;
        if (tokenList.checkAheadChar(0).getType() == TokenType.CONST) {
            isConst = true;
            tokenList.getChar();
        }
        Token type = tokenList.getCharWithJudge(TokenType.INT, TokenType.FLOAT);
        defs.add(parseDef(type, isConst));
        while (tokenList.checkAheadChar(0).getType() == TokenType.COMMA) {
            tokenList.getChar();
            defs.add(parseDef(type, isConst));
        }
        tokenList.getCharWithJudge(TokenType.SEMI);
        return new Ast.Decl(isConst, type, defs);
    }

    private Ast.Def parseDef(Token type, boolean isConst) {
        Token ident = tokenList.getCharWithJudge(TokenType.IDENT);
        ArrayList<Ast.Exp> indexList = new ArrayList<>();
        Ast.Init init = null;
        while (tokenList.checkAheadChar(0).getType() == TokenType.LBRACK) {
            tokenList.getCharWithJudge(TokenType.LBRACK);
            indexList.add(parseBinaryExp(BinaryExpType.ADD));
            tokenList.getCharWithJudge(TokenType.RBRACK);
        }
        //TODO: 常量赋值是否需要异常判断？如果需要，怎么判断？
        if (isConst) {
            tokenList.getCharWithJudge(TokenType.ASSIGN);
            init = parseInit();
        } else {
            if (tokenList.hasNext() && tokenList.checkAheadChar(0).getType() == TokenType.ASSIGN) {
                tokenList.getChar();
                init = parseInit();
            }
        }
        return new Ast.Def(type.getType(), ident, indexList, init);
    }

    private Ast.Init parseInit() {
        if (tokenList.checkAheadChar(0).getType() == TokenType.LBRACE) {
            return parseInitArray();
        } else {
            return parseBinaryExp(BinaryExpType.ADD);
        }
    }

    private Ast.InitArray parseInitArray() {
        ArrayList<Ast.Init> inits = new ArrayList<>();
        tokenList.getCharWithJudge(TokenType.LBRACE);
        if (tokenList.checkAheadChar(0).getType() != TokenType.RBRACE) {
            inits.add(parseInit());
            while (tokenList.checkAheadChar(0).getType() == TokenType.COMMA) {
                tokenList.getChar();
                inits.add(parseInit());
            }
        }
        tokenList.getCharWithJudge(TokenType.RBRACE);
        return new Ast.InitArray(inits);
    }

    private Ast.Stmt parseStmt() {
        TokenType type = tokenList.checkAheadChar(0).getType();
        Ast.Exp condition = null;
        switch (type) {
            case LBRACE:
                return parseBlock();
            case IF:
                tokenList.getChar();
                tokenList.getCharWithJudge(TokenType.LPARENT);
                condition = parseBinaryExp(BinaryExpType.LOR);
                tokenList.getCharWithJudge(TokenType.RPARENT);
                Ast.Stmt thenStmt = parseStmt();
                Ast.Stmt elseStmt = null;
                if (tokenList.hasNext() && tokenList.checkAheadChar(0).getType() == TokenType.ELSE) {
                    tokenList.getChar();
                    elseStmt = parseStmt();
                }
                return new Ast.IfStmt(condition, thenStmt, elseStmt);
            case WHILE:
                tokenList.getChar();
                tokenList.getCharWithJudge(TokenType.LPARENT);
                condition = parseBinaryExp(BinaryExpType.LOR);
                tokenList.getCharWithJudge(TokenType.RPARENT);
                Ast.Stmt body = parseStmt();
                return new Ast.WhileStmt(condition, body);
            case RETURN:
                tokenList.getCharWithJudge(TokenType.RETURN);
                Ast.Exp returnValue = null;
                if (tokenList.checkAheadChar(0).getType() != TokenType.SEMI) {
                    returnValue = parseBinaryExp(BinaryExpType.ADD);
                }
                tokenList.getCharWithJudge(TokenType.SEMI);
                return new Ast.Return(returnValue);
            case BREAK:
                tokenList.getCharWithJudge(TokenType.BREAK);
                tokenList.getCharWithJudge(TokenType.SEMI);
                return new Ast.Break();
            case CONTINUE:
                tokenList.getCharWithJudge(TokenType.CONTINUE);
                tokenList.getCharWithJudge(TokenType.SEMI);
                return new Ast.Continue();
            case IDENT:
                Ast.Exp tryExp = parseBinaryExp(BinaryExpType.ADD);
                Ast.LVal tryLVal = tryGetLVal(tryExp);
                if (tryLVal == null) {
                    tokenList.getCharWithJudge(TokenType.SEMI);
                    return new Ast.ExpStmt(tryExp);
                } else {
                    if (tokenList.checkAheadChar(0).getType() == TokenType.ASSIGN) {
                        tokenList.getChar();
                        Ast.Exp right = parseBinaryExp(BinaryExpType.ADD);
                        tokenList.getCharWithJudge(TokenType.SEMI);
                        return new Ast.Assign(tryLVal, right);
                    } else {
                        tokenList.getCharWithJudge(TokenType.SEMI);
                        return new Ast.ExpStmt(tryExp);
                    }
                }
            case SEMI:
                tokenList.getChar();
                return new Ast.ExpStmt(null);
            default:
                Ast.Exp exp = parseBinaryExp(BinaryExpType.ADD);
                return new Ast.ExpStmt(exp);
        }
    }

    private Ast.LVal tryGetLVal(Ast.Exp exp) {
        Ast.Exp temp = exp;
        while (temp instanceof Ast.BinaryExp) {
            if (!((Ast.BinaryExp) temp).getRestExps().isEmpty()) {
                return null;
            }
            temp = ((Ast.BinaryExp) temp).getFirstExp();
        }
        if (!(((Ast.UnaryExp) temp).getUnaryOps().isEmpty())) {
            return null;
        }
        Ast.PrimaryExp primaryExp = ((Ast.UnaryExp) temp).getPrimaryExp();
        if (primaryExp instanceof Ast.LVal) {
            return (Ast.LVal) primaryExp;
        } else {
            return null;
        }
    }

    private enum BinaryExpType {
        LOR(TokenType.LOR),
        LAND(TokenType.LAND),
        EQ(TokenType.EQ, TokenType.NE),
        REL(TokenType.GT, TokenType.LT, TokenType.GE, TokenType.LE),
        ADD(TokenType.ADD, TokenType.SUB),
        MUL(TokenType.MUL, TokenType.DIV, TokenType.MOD),
        ;

        private final List<TokenType> types;

        BinaryExpType(TokenType... types) {
            // todo: 这里原本用的是 List.of
            this.types = Arrays.asList(types);
        }

        public boolean contains(TokenType type) {
            return types.contains(type);
        }
    }
}
