package frontend;

import frontend.AST.*;
import frontend.AST.Number;

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;

import static frontend.Lexer.*;

public class Parser {
    //private TokenList tokenList;
    private final Lexer lexer;
    private final HashMap<Integer, String> ALUOpMap = new HashMap<>();
    private final HashMap<String, String> operation = new HashMap<>();
    private ArrayList<CompUnit> units = new ArrayList<>();
    public Parser(Lexer lexer) {
        initALUOpMap();
        this.lexer = lexer;
        this.units = new ArrayList<CompUnit>();
    }

//    public AST parseAST() throws IOException {
//        ArrayList<CompUnit> units = new ArrayList<>();
//        while (lexer.hasNext()) {
//            if (lexer.get(2).getType() == LPARENT) {
//                units.add(parseFuncDef());
//            } else {
//                units.add(parseDecl());
//            }
//        }
//        return new AST(units);
//    }

    public AST parseAST() throws IOException {
        units.clear();
        while (lexer.hasNext()) {
            units.add(parseCompUnit());
        }
        return new AST(units);
    }

    private CompUnit parseCompUnit() throws IOException {
        if (lexer.get(2).getType() == LPARENT) {
            return parseFuncDef();
        } else {
            return parseDecl();
        }
    }

    public void initALUOpMap() {
        ALUOpMap.put(NE, "EQ");
        ALUOpMap.put(AND, "LAND");
        ALUOpMap.put(OR, "LOR");
        ALUOpMap.put(EQUAL, "EQ");
        ALUOpMap.put(LESS, "REL");
        ALUOpMap.put(LE, "REL");
        ALUOpMap.put(MORE, "REL");
        ALUOpMap.put(ME, "REL");
        ALUOpMap.put(SUB, "ADD");
        ALUOpMap.put(MULT, "MUL");
        ALUOpMap.put(ADD, "ADD");
        ALUOpMap.put(DIV, "MUL");
        ALUOpMap.put(MOD, "MUL");

        operation.put("LAND", "EQ");
        operation.put("LOR", "LAND");
        operation.put("REL", "ADD");
        operation.put("EQ", "REL");
        operation.put("ADD", "MUL");
        operation.put("MUL", "UNARY");
    }

    public FuncDef parseFuncDef() throws IOException {
        assert lexer.get(0).getType() == INT || lexer.get(0).getType() == FLOAT || lexer.get(0).getType() == VOID;
        String funcType = lexer.after().getVal();
        String ident = lexer.after().getVal();
        ArrayList<FuncFParam> funcFParams = new ArrayList<>();
        lexer.after();
        if (lexer.get(0).getType() != RPARENT) {
            funcFParams.add(parseFuncFParam());
            while (lexer.get(0).getType() != RPARENT) {
                assert lexer.get(0).getType() == COMMA;
                lexer.after();
                funcFParams.add(parseFuncFParam());
            }
        }
        lexer.after(); //跳过）,还剩下Block
        Block block = parseBlock();
        return new FuncDef(funcType, ident, funcFParams, block);
    }

    public FuncFParam parseFuncFParam() throws IOException {
        Token firstToken = lexer.get(0);
        if (firstToken.getType() != INT && firstToken.getType() != FLOAT) {
            throw new IOException("Expected INT or FLOAT");
        }
        String bType = lexer.after().getVal();
        String ident = lexer.after().getVal();
        ArrayList<Exp> array = parseArrayDimensions();
        boolean isArray = !array.isEmpty();
        if (isArray) {
            array.remove(0);
        }
        return new FuncFParam(bType, ident, isArray, array);
    }

    private ArrayList<Exp> parseArrayDimensions() throws IOException {
        ArrayList<Exp> array = new ArrayList<>();
        while (lexer.get(0).getType() == LBRACKET) {
            lexer.after();
            if (lexer.get(0).getType() == RBRACKET) {
                array.add(null);
                lexer.after();
            }
            else {
                array.add(parseExp());
                if (lexer.get(0).getType() != RBRACKET) {
                    throw new IOException("Expected ']'");
                }
                lexer.after();
            }
        }
        return array;
    }

    private ArrayList<Exp> parseArrayDims() throws IOException {
        ArrayList<Exp> exps = new ArrayList<>();
        while (lexer.get(0).getType() == LBRACKET) {
            lexer.after();
            exps.add(parseExp());
            lexer.after();
        }
        return exps;
    }

    public Def parseDef(boolean isConst) throws IOException {
        String ident = lexer.after().getVal();
        ArrayList<Exp> exps = parseArrayDims();
        InitVal initVal = parseInitializer(isConst);
        return new Def(ident, exps, initVal);
    }

    private InitVal parseInitializer(boolean isConst) throws IOException {
        if (isConst || (lexer.hasNext() && lexer.get(0).getType() == ASSIGN)) {
            lexer.after();
            return parseInitVal();
        }
        return null;
    }

    public Decl parseDecl() throws IOException {
        boolean isConst = parseConstFlag();
        String bType = lexer.after().getVal();
        ArrayList<Def> defs = parseDefList(isConst);
        lexer.after(); // Move past the SEMICOLON token
        return new Decl(isConst, bType, defs);
    }

    private boolean parseConstFlag() throws IOException {
        if (lexer.get(0).getType() == CONST) {
            lexer.after(); // Move past the CONST token
            return true;
        }
        return false;
    }
    private ArrayList<Def> parseDefList(boolean isConst) throws IOException {
        ArrayList<Def> defs = new ArrayList<>();
        defs.add(parseDef(isConst));
        while (lexer.get(0).getType() == COMMA) {
            lexer.after();
            defs.add(parseDef(isConst));
        }
        return defs;
    }

    public InitVal parseInitVal() throws IOException {
        if (lexer.get(0).getType() == LBRACE) { //数组
            return parseInitArray();
        } else {
            return parseExp();
        }
    }

    public InitArray parseInitArray() throws IOException {
        //目前是{
        lexer.after();
        ArrayList<InitVal> initVals = new ArrayList<>();
        if (lexer.get(0).getType() != RBRACE) {
            initVals.add(parseInitVal());
            while (lexer.get(0).getType() == COMMA) {
                lexer.after();
                initVals.add(parseInitVal());
            }
        }
        assert lexer.get(0).getType() == RBRACE;
        lexer.after();
        return new InitArray(initVals);
    }

    public Block parseBlock() throws IOException {
        lexer.after();
        ArrayList<BlockItem> blockItems = new ArrayList<>();
        while (lexer.get(0).getType() != RBRACE) {
            blockItems.add(parseBlockItem());
        }
        lexer.after();
        return new Block(blockItems);
    }

    public BlockItem parseBlockItem() throws IOException {
        if (lexer.get(0).getType() == CONST || lexer.get(0).getType() == INT || lexer.get(0).getType() == FLOAT) {
            return parseDecl();
        } else {
            return parseStmt();
        }
    }

    public Stmt parseStmt() throws IOException {
        int type = lexer.get(0).getType();
        switch (type) {
            case IDENTF -> {
                //TODO: 关于是expStmt和Assign的判断
                if (lexer.get(1).getType() == LPARENT) {
                    return paeseExpStmt();
                } else {
                    Exp exp = parseExp();
                    if (lexer.get(0).getType() == ASSIGN) {
                        lexer.after();
                        BinaryExp exp0 = parseExp();
                        while (exp instanceof BinaryExp) {
                            exp = ((BinaryExp) exp).getFirst();
                        }
                        UnaryExp exp1 = (UnaryExp) exp;
                        PrimaryExp lval = exp1.getPrimaryExp();
                        lexer.after();
                        return new Assign((Lval) lval, exp0);
                    } else {
                        lexer.after();
                        return new ExpStmt(exp);
                    }
                }
            }
            case LBRACE -> {
//                System.out.println(tokenList.getPos());
                return parseBlock();
            }
            case IF -> {
                return parseIf();
            }
            case WHILE -> {
                return parseWhile();
            }
            case BREAK -> {
                lexer.after();
                lexer.after();
                return new Break();
            }
            case CONTINUE -> {
                lexer.after();
                lexer.after();
                return new Continue();
            }
            case RETURN -> {
                return parseReturn();
            }
            default -> {
                return paeseExpStmt();
            }
        }
    }

    public Assign parseAssign() throws IOException {
        Lval lval = parseLval();
        lexer.after();
        Exp exp = parseExp();
        lexer.after();
        return new Assign(lval, exp);
    }

//    public Lval parseLval() throws IOException {
//        String ident = lexer.after().getVal();
//        boolean isArray = false;
//        ArrayList<Exp> indexs = new ArrayList<>();
//        if (lexer.get(0).getType() == LBRACKET) {
//            isArray = true;
//            lexer.after();
//            indexs.add(parseExp());
//            lexer.after();
//            while (lexer.get(0).getType() == LBRACKET) {
//                lexer.after();
//                indexs.add(parseExp());
//                lexer.after();
//            }
//        }
//        return new Lval(ident, isArray, indexs);
//    }

    public Lval parseLval() throws IOException {
        String ident = parseIdentifier();
        boolean isArray = parseArrayAccess();
        ArrayList<Exp> indexs = new ArrayList<>();
        if (isArray) {
            indexs = parseArrayIndices();
        }
        return new Lval(ident, isArray, indexs);
    }

    private String parseIdentifier() throws IOException {
        return lexer.after().getVal();
    }

    private boolean parseArrayAccess() throws IOException {
        if (lexer.get(0).getType() == LBRACKET) {
            lexer.after();
            return true;
        }
        return false;
    }

    private ArrayList<Exp> parseArrayIndices() throws IOException {
        ArrayList<Exp> indices = new ArrayList<>();
        indices.add(parseExp());
        lexer.after();
        while (lexer.get(0).getType() == LBRACKET) {
            lexer.after();
            indices.add(parseExp());
            lexer.after();
        }
        return indices;
    }

    public If parseIf() throws IOException {
        lexer.after();
        lexer.after();
        Exp cond = parseCond();
        lexer.after();
        Stmt ifStmt = parseStmt();
        Stmt elseStmt = null;
        boolean haveElse = false;
        if (lexer.get(0).getType() == ELSE) {
            lexer.after();
            haveElse = true;
            elseStmt = parseStmt();
        }
        return new If(cond, ifStmt, haveElse, elseStmt);
    }

    public While parseWhile() throws IOException {
        lexer.after();
        lexer.after();
        Exp cond = parseCond();
        lexer.after();
        Stmt whileStmt = parseStmt();
        return new While(cond, whileStmt);
    }

    public Return parseReturn() throws IOException {
        lexer.after();
        Exp exp = null;
        if (lexer.get(0).getType() != SEMICOLON) {
            exp = parseExp();
        }
        lexer.after();
        return new Return(exp);
    }

    public ExpStmt paeseExpStmt() throws IOException {
        Exp exp = null;
        if (lexer.get(0).getType() != SEMICOLON) {
            exp = parseExp();
        }
        lexer.after();
        return new ExpStmt(exp);
    }


    public Exp parseSubBinaryExp(String op) throws IOException {
        if (operation.get(op).equals("UNARY")) {
            return parseUnaryExp();
        }
        return parseBinary(operation.get(op));
    }

    public BinaryExp parseExp() throws IOException {
        return parseBinary("ADD");
    }

    public BinaryExp parseCond() throws IOException {
        return parseBinary("LOR");
    }

    public BinaryExp parseBinary(String op) throws IOException {
        Exp first = parseSubBinaryExp(op);
        ArrayList<String> binaryOps = new ArrayList<>();
        ArrayList<Exp> exps = new ArrayList<>();
        while (ALUOpMap.get(lexer.get(0).getType()) != null && ALUOpMap.get(lexer.get(0).getType()).equals(op)) {
            binaryOps.add(lexer.after().getVal());
            exps.add(parseSubBinaryExp(op));
        }
        return new BinaryExp(first, binaryOps, exps);
    }

    public UnaryExp parseUnaryExp() throws IOException {
        ArrayList<String> unaryOps = new ArrayList<>();
        PrimaryExp primaryExp;
        while(lexer.get(0).getType() == ADD || lexer.get(0).getType() == SUB || lexer.get(0).getType() == NON) {
            unaryOps.add(lexer.after().getVal());
        }
        primaryExp = parsePrimaryExp();
        return new UnaryExp(unaryOps, primaryExp);
    }

//    public PrimaryExp parsePrimaryExp() throws IOException {
//        if (lexer.get(0).getType() == LPARENT) {
//            lexer.after();
//            Exp exp = parseExp();
//            lexer.after();
//            return exp;
//        } else if (lexer.get(0).getType() == IDENTF) {
////            String ident = tokenList.get(0).getVal();
//            if (lexer.get(1).getType() == LPARENT) {
//                return parseFunc();
//            } else {
//                return parseLval();
//            }
//        } else {
//            assert lexer.get(0).getType() == DECFLOATCON || lexer.get(0).getType() == HEXFLOATCON ||
//                    lexer.get(0).getType() == HEXCON || lexer.get(0).getType() == OCTCON || lexer.get(0).getType() == DECCON;
//            int type = lexer.get(0).getType();
//            String number = lexer.after().getVal();
//            //System.out.println(number);
//            return new Number(number, type);
//        }
//    }

    public PrimaryExp parsePrimaryExp() throws IOException {
        Token currentToken = lexer.get(0);
        if (currentToken.getType() == LPARENT) {
            lexer.after();
            Exp exp = parseExp(); // Parse the expression inside parentheses
            lexer.after();
            return exp;
        } else if (currentToken.getType() == IDENTF) {
            if (lexer.get(1).getType() != LPARENT) {
                return parseLval();
            } else {
                return parseFunc();
            }
        } else {
            int type = currentToken.getType();
            String value = lexer.after().getVal();
            return new Number(value, type);
        }
    }

    public Func parseFunc() throws IOException {
        String ident = lexer.get(0).getVal();
        lexer.after();
        lexer.after();
        ArrayList<Exp> funRParams = new ArrayList<>();
        int lineNum = lexer.get(0).getLineNum();
        if (lexer.get(0).getType() != RPARENT) {
            funRParams.add(parseExp());
            while (lexer.get(0).getType() != RPARENT) {
                assert lexer.get(0).getType() == COMMA;
                lexer.after();
                funRParams.add(parseExp());
            }
        }
        lexer.after();
        return new Func(ident, funRParams, lineNum);
    }
}

