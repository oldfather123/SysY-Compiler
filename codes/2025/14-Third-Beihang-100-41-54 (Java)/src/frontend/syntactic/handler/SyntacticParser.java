package frontend.syntactic.handler;

import frontend.lexer.entity.Token;
import frontend.lexer.entity.TokenList;
import frontend.lexer.entity.TokenType;
import frontend.lexer.handler.LexerAnalyzer;
import frontend.syntactic.entity.ast.*;

import java.io.BufferedWriter;
import java.util.ArrayList;

public class SyntacticParser {

    public TokenList tokenList;

    public CompileUnitNode astEntry;

    public static boolean PRINT_DEBUG_MOD = false;
    public BufferedWriter bufferedWriter;

    private boolean isGlobal = true;

    public SyntacticParser(LexerAnalyzer lexerAnalyzer, BufferedWriter bufferedWriter){
        this.tokenList = lexerAnalyzer.tokenList;
        this.bufferedWriter = bufferedWriter;
    }

    public void parserAnalyze(){
        astEntry = parserCompileUnit();
    }

    public void putPutError(Token token){
        System.err.println(token.line);
    }

    public void syntacticAnalyze(){
        astEntry = parserCompileUnit();
    }

    public CompileUnitNode parserCompileUnit(){
        if(PRINT_DEBUG_MOD) System.out.println("<CompileUnit>" + tokenList.getNowToken().content);
        CompileUnitNode compileUnitNode = new CompileUnitNode();
        while(!tokenList.isEnd()) compileUnitNode.declList.add(parserCompileUnitDecl());
        return compileUnitNode;
    }

    public CompileUnitDeclNode parserCompileUnitDecl(){
        if(PRINT_DEBUG_MOD) System.out.println("<CompileUnitDecl>" + tokenList.getNowToken().content);
        CompileUnitDeclNode compileUnitDeclNode = new CompileUnitDeclNode();
        Token token = tokenList.getNowToken();
        if(token.content.equals("const")){
            compileUnitDeclNode.declNode = parserDeclNode();
            compileUnitDeclNode.nodeType = CompileUnitDeclNode.declType;
        }
        else {
            Token tokenOp = tokenList.preRead(2);
            if(tokenOp != null && tokenOp.wordType.equals(TokenType.L_PAREN)){
                compileUnitDeclNode.funcDefNode = parserFuncDefNode();
                compileUnitDeclNode.nodeType = CompileUnitDeclNode.funcDefType;
            }
            else{
                compileUnitDeclNode.nodeType = CompileUnitDeclNode.declType;
                compileUnitDeclNode.declNode = parserDeclNode();
            }
        }
        return compileUnitDeclNode;
    }

    public DeclNode parserDeclNode(){
        if(PRINT_DEBUG_MOD) System.out.println("<Decl>" + tokenList.getNowToken().content);
        DeclNode declNode = new DeclNode();declNode.isGlobal = isGlobal;
        Token token = tokenList.getNowToken();
        if(token.wordType.equals(TokenType.CONST)){
            declNode.constDeclNode = parserConstDeclNode();
            declNode.nodeType = DeclNode.constDeclType;
        }
        else{
            declNode.varDeclNode = parserVarDeclNode();
            declNode.nodeType = DeclNode.varDeclType;
        }
        return declNode;
    }

    public ConstDeclNode parserConstDeclNode(){
        if(PRINT_DEBUG_MOD) System.out.println("<ConstDecl>" + tokenList.getNowToken().content);
        ConstDeclNode constDeclNode = new ConstDeclNode();
        Token token = tokenList.getNowToken();tokenList.goToNextToken();
        constDeclNode.bTypeNode = parserBTypeNode();
        do{
            if(tokenList.getNowToken().wordType.equals(TokenType.COMMA)) tokenList.goToNextToken();
            constDeclNode.constDefNodeList.add(parserConstDefNode());
        }while(tokenList.getNowToken().wordType.equals(TokenType.COMMA));
        if(!tokenList.getNowToken().wordType.equals(TokenType.SEMI)) putPutError(tokenList.getNowToken());

        tokenList.goToNextToken();
        return constDeclNode;
    }

    public BTypeNode parserBTypeNode(){
        if(PRINT_DEBUG_MOD) System.out.println("<BType>" + tokenList.getNowToken().content);
        BTypeNode bTypeNode = new BTypeNode();
        Token token = tokenList.getNowToken();
        bTypeNode.tokenType = token.wordType;tokenList.goToNextToken();
        return bTypeNode;
    }

    public ConstDefNode parserConstDefNode(){
        if(PRINT_DEBUG_MOD) System.out.println("<ConstDef>" + tokenList.getNowToken().content);
        ConstDefNode constDefNode = new ConstDefNode();
        constDefNode.identToken = tokenList.getNowToken(); tokenList.goToNextToken();
        Token token = tokenList.getNowToken();
        if(token.wordType.equals(TokenType.L_BRACK)){
            do{
                tokenList.goToNextToken();
                constDefNode.dimensionExpList.add(parserConstExpNode());
                tokenList.goToNextToken();
            }while(tokenList.getNowToken().wordType.equals(TokenType.L_BRACK));
        }
        // =
        tokenList.goToNextToken();
        constDefNode.constInitValNode = parserConstInitValNode();
        return constDefNode;
    }

    public ConstInitValNode parserConstInitValNode(){
        if(PRINT_DEBUG_MOD) System.out.println("<CostInitVal>" + tokenList.getNowToken().content);
        ConstInitValNode constInitValNode = new ConstInitValNode();
        if(tokenList.getNowToken().wordType.equals(TokenType.L_BRACE)){
            tokenList.goToNextToken();
            if(!tokenList.getNowToken().wordType.equals(TokenType.R_BRACE)){
                do{
                    if(tokenList.getNowToken().wordType.equals(TokenType.COMMA)) tokenList.goToNextToken();
                    constInitValNode.constInitValNodeArrayList.add(parserConstInitValNode());
                }while(tokenList.getNowToken().wordType.equals(TokenType.COMMA));
            }
            tokenList.goToNextToken();
            constInitValNode.nodeType = ConstInitValNode.constInitValListType;
        }
        else {
            constInitValNode.constExpNode = parserConstExpNode();
            constInitValNode.nodeType = ConstInitValNode.constExpNodeType;
        }
        return constInitValNode;
    }

    public ConstExpNode parserConstExpNode(){
        if(PRINT_DEBUG_MOD) System.out.println("<ConstExp>" + tokenList.getNowToken().content);
        ConstExpNode constExpNode = new ConstExpNode();
        constExpNode.addExpNode = parserAddExpNode();
        return constExpNode;
    }

    public VarDeclNode parserVarDeclNode(){
        if(PRINT_DEBUG_MOD) System.out.println("<VarDecl>" + tokenList.getNowToken().content);
        VarDeclNode varDeclNode = new VarDeclNode();
        varDeclNode.bTypeNode = parserBTypeNode();
        do{
            if(tokenList.getNowToken().wordType.equals(TokenType.COMMA)) tokenList.goToNextToken();
            varDeclNode.varDefNodeList.add(parserVarDefNode());
        }while(tokenList.getNowToken().wordType.equals(TokenType.COMMA));
        tokenList.goToNextToken();
        return varDeclNode;
    }

    public VarDefNode parserVarDefNode(){
        if(PRINT_DEBUG_MOD) System.out.println("<VarDef>" + tokenList.getNowToken().content);
        VarDefNode varDefNode = new VarDefNode();
        varDefNode.identToken = tokenList.getNowToken();
        tokenList.goToNextToken();
        if(tokenList.getNowToken().wordType.equals(TokenType.ASSIGN)){
            tokenList.goToNextToken();
            varDefNode.initValNode = parserInitValNode();
        }
        else{
            if(tokenList.getNowToken().wordType.equals(TokenType.L_BRACK)){
                do{
                    tokenList.goToNextToken();
                    varDefNode.constExpNodeArrayList.add(parserConstExpNode());
                    tokenList.goToNextToken();
                }while(tokenList.getNowToken().wordType.equals(TokenType.L_BRACK));

                if(tokenList.getNowToken().wordType.equals(TokenType.ASSIGN)){
                    tokenList.goToNextToken();
                    varDefNode.initValNode = parserInitValNode();
                }
            }
        }
        return varDefNode;
    }

    public InitValNode parserInitValNode(){
        if(PRINT_DEBUG_MOD) System.out.println("<InitVal>" + tokenList.getNowToken().content);
        InitValNode initValNode = new InitValNode();
        if(tokenList.getNowToken().wordType.equals(TokenType.L_BRACE)){
            tokenList.goToNextToken();
            if(!tokenList.getNowToken().wordType.equals(TokenType.R_BRACE)){
                do{
                    if(tokenList.getNowToken().wordType.equals(TokenType.COMMA)) tokenList.goToNextToken();
                    initValNode.initValNodeArrayList.add(parserInitValNode());
                }while(tokenList.getNowToken().wordType.equals(TokenType.COMMA));
            }
            tokenList.goToNextToken();
            initValNode.nodeType = InitValNode.initValListType;
        }
        else {
            initValNode.expNode = parserExpNode();
            initValNode.nodeType = InitValNode.expNodeType;
        }
        return initValNode;
    }

    public FuncDefNode parserFuncDefNode(){
        if(PRINT_DEBUG_MOD) System.out.println("<FuncDef>" + tokenList.getNowToken().content);
        FuncDefNode funcDefNode = new FuncDefNode();
        funcDefNode.funcTypeNode = parserFuncTypeNode();
        funcDefNode.identToken = tokenList.getNowToken();
        tokenList.goToNextToken();
        tokenList.goToNextToken();
        if(!tokenList.getNowToken().wordType.equals(TokenType.R_PAREN)){
            do{
                if(tokenList.getNowToken().wordType.equals(TokenType.COMMA)) tokenList.goToNextToken();
                funcDefNode.funcFParamNodeArrayList.add(parserFuncFParamNode());
            }while(tokenList.getNowToken().wordType.equals(TokenType.COMMA));
        }
        tokenList.goToNextToken();

        isGlobal = false;
        funcDefNode.blockNode = parserBlockNode();
        isGlobal = true;
        return funcDefNode;
    }

    public FuncTypeNode parserFuncTypeNode(){
        if(PRINT_DEBUG_MOD) System.out.println("<FuncType>" + tokenList.getNowToken().content);
        FuncTypeNode funcTypeNode = new FuncTypeNode();
        funcTypeNode.typeToken = tokenList.getNowToken();
        tokenList.goToNextToken();
        return funcTypeNode;
    }

    public FuncFParamNode parserFuncFParamNode(){
        if(PRINT_DEBUG_MOD) System.out.println("<FuncFParam>" + tokenList.getNowToken().content);
        FuncFParamNode funcFParamNode = new FuncFParamNode();
        funcFParamNode.bTypeNode = parserBTypeNode();
        funcFParamNode.identToken = tokenList.getNowToken();
        tokenList.goToNextToken();
        if(tokenList.getNowToken().wordType.equals(TokenType.L_BRACK)){
            funcFParamNode.isArrayParam = true;
            tokenList.goToNextToken();tokenList.goToNextToken();
            while(tokenList.getNowToken().wordType.equals(TokenType.L_BRACK)){
                tokenList.goToNextToken();
                funcFParamNode.expNodeArrayList.add(parserExpNode());
                tokenList.goToNextToken();
            }
        }
        return funcFParamNode;
    }

    public BlockNode parserBlockNode(){
        if(PRINT_DEBUG_MOD) System.out.println("<Block>" + tokenList.getNowToken().content);
        BlockNode blockNode = new BlockNode();
        tokenList.goToNextToken();
        while(!tokenList.getNowToken().wordType.equals(TokenType.R_BRACE)){
            blockNode.blockItemNodeArrayList.add(parserBlockItemNode());
        }
        tokenList.goToNextToken();
        return blockNode;
    }

    public BlockItemNode parserBlockItemNode(){
        if(PRINT_DEBUG_MOD) System.out.println("<BlockItem>" + tokenList.getNowToken().content);
        BlockItemNode blockItemNode = new BlockItemNode();
        Token token = tokenList.getNowToken();
        if(token.wordType.equals(TokenType.CONST) || token.wordType.equals(TokenType.INT) || token.wordType.equals(TokenType.FLOAT)){
            blockItemNode.nodeType = BlockItemNode.declNodeType;
            blockItemNode.declNode = parserDeclNode();
        }
        else{
            blockItemNode.nodeType = BlockItemNode.stmtNodeType;
            blockItemNode.stmtNode = parserStmtNode();
        }
        return blockItemNode;
    }

    public StmtNode parserStmtNode(){
        if(PRINT_DEBUG_MOD) System.out.println("<Stmt>" + tokenList.getNowToken().content);
        StmtNode stmtNode = new StmtNode();
        Token token = tokenList.getNowToken();
        if(token.wordType.equals(TokenType.IF)){
            stmtNode.stmtType = StmtNode.StmtType.IF_STMT;
            stmtNode.ifStmtNode = parserIfStmtNode();
        }
        else if(token.wordType.equals(TokenType.WHILE)){
            stmtNode.stmtType = StmtNode.StmtType.WHILE_STMT;
            stmtNode.whileStmtNode = parserWhileStmtNode();
        }
        else if(token.wordType.equals(TokenType.BREAK)){
            stmtNode.stmtType = StmtNode.StmtType.BREAK_STMT;
            stmtNode.breakStmtNode = parserBreakStmtNode();
        }
        else if(token.wordType.equals(TokenType.CONTINUE)){
            stmtNode.stmtType = StmtNode.StmtType.CONTINUE_STMT;
            stmtNode.continueStmtNode = parserContinueStmtNode();
        }
        else if(token.wordType.equals(TokenType.RETURN)){
            stmtNode.stmtType = StmtNode.StmtType.RETURN_STMT;
            stmtNode.returnStmtNode = parserReturnStmtNode();
        }
        else if(token.wordType.equals(TokenType.L_BRACE)){
            stmtNode.stmtType = StmtNode.StmtType.BLOCK_STMT;
            stmtNode.blockStmtNode = parserBlockStmtNode();
        }
        else if(token.wordType.equals(TokenType.PUT_F)){
            stmtNode.stmtType = StmtNode.StmtType.PUT_F_STMT;
            stmtNode.putfStmtNode = parserPutFStmtNode();
        }
        else{
            boolean hasAssign = false;
            for(int index = 0;tokenList.preRead(index) != null && !tokenList.preRead(index).wordType.equals(TokenType.SEMI);index ++){
                if(tokenList.preRead(index).wordType.equals(TokenType.ASSIGN)){
                    hasAssign = true; break;
                }
            }
            if(hasAssign){
                stmtNode.stmtType = StmtNode.StmtType.LVAL_STMT;
                stmtNode.lValStmtNode = parserLValStmtNode();
            }
            else{
                stmtNode.stmtType = StmtNode.StmtType.EXP_STMT;
                stmtNode.expStmtNode = parserExpStmtNode();
            }
        }
        return stmtNode;
    }

    public StmtNode.ExpStmtNode parserExpStmtNode(){
        if(PRINT_DEBUG_MOD) System.out.println("<ExpStmt>" + tokenList.getNowToken().content);
        StmtNode.ExpStmtNode expStmtNode = new StmtNode.ExpStmtNode();
        if(!tokenList.getNowToken().wordType.equals(TokenType.SEMI)){
            expStmtNode.expNode = parserExpNode();
        }
        tokenList.goToNextToken();
        return expStmtNode;
    }

    public StmtNode.LValStmtNode parserLValStmtNode(){
        if(PRINT_DEBUG_MOD) System.out.println("<LValStmt>" + tokenList.getNowToken().content);
        StmtNode.LValStmtNode lValStmtNode = new StmtNode.LValStmtNode();
        lValStmtNode.lValNode = parserLValNode();
        tokenList.goToNextToken();
        lValStmtNode.expNode = parserExpNode();
        tokenList.goToNextToken();
        return lValStmtNode;
    }

    public StmtNode.PutFStmtNode parserPutFStmtNode(){
        if(PRINT_DEBUG_MOD) System.out.println("<PutFStmt>" + tokenList.getNowToken().content);
        StmtNode.PutFStmtNode putFStmtNode = new StmtNode.PutFStmtNode();
        tokenList.goToNextToken();
        tokenList.goToNextToken();
        putFStmtNode.strConstToken = tokenList.getNowToken();
        tokenList.goToNextToken();
        if(!tokenList.getNowToken().wordType.equals(TokenType.R_PAREN)){
            putFStmtNode.expParamNodeList = new ArrayList<>();
            while(true) {
                Token token = tokenList.getNowToken();
                if (token.wordType.equals(TokenType.COMMA)) tokenList.goToNextToken();
                else if (token.wordType.equals(TokenType.R_PAREN)) break;
                putFStmtNode.expParamNodeList.add(parserExpNode());
            }
        }
        tokenList.goToNextToken();
        return putFStmtNode;
    }

    public StmtNode.BlockStmtNode parserBlockStmtNode(){
        if(PRINT_DEBUG_MOD) System.out.println("<BlockStmt>" + tokenList.getNowToken().content);
        StmtNode.BlockStmtNode blockStmtNode = new StmtNode.BlockStmtNode();
        blockStmtNode.blockNode = parserBlockNode();
        return blockStmtNode;
    }

    public StmtNode.ReturnStmtNode parserReturnStmtNode(){
        if(PRINT_DEBUG_MOD) System.out.println("<ReturnStmt>" + tokenList.getNowToken().content);
        StmtNode.ReturnStmtNode returnStmtNode = new StmtNode.ReturnStmtNode();
        tokenList.goToNextToken();
        if(!tokenList.getNowToken().wordType.equals(TokenType.SEMI)){
            returnStmtNode.expNode = parserExpNode();
        }
        tokenList.goToNextToken();return returnStmtNode;
    }

    public StmtNode.BreakStmtNode parserBreakStmtNode(){
        if(PRINT_DEBUG_MOD) System.out.println("<BreakStmt>" + tokenList.getNowToken().content);
        StmtNode.BreakStmtNode breakStmtNode = new StmtNode.BreakStmtNode();
        tokenList.goToNextToken();tokenList.goToNextToken();
        return breakStmtNode;
    }

    public StmtNode.ContinueStmtNode parserContinueStmtNode(){
        if(PRINT_DEBUG_MOD) System.out.println("<ContinueStmt>" + tokenList.getNowToken().content);
        StmtNode.ContinueStmtNode continueStmtNode = new StmtNode.ContinueStmtNode();
        tokenList.goToNextToken();tokenList.goToNextToken();
        return continueStmtNode;
    }

    public StmtNode.WhileStmtNode parserWhileStmtNode(){
        if(PRINT_DEBUG_MOD) System.out.println("<WhileStmt>" + tokenList.getNowToken().content);
        StmtNode.WhileStmtNode whileStmtNode = new StmtNode.WhileStmtNode();
        tokenList.goToNextToken();tokenList.goToNextToken();
        whileStmtNode.condNode = parserCondNode();
        tokenList.goToNextToken();
        whileStmtNode.stmtNode = parserStmtNode();
        return whileStmtNode;
    }

    public StmtNode.IfStmtNode parserIfStmtNode(){
        if(PRINT_DEBUG_MOD) System.out.println("<IfStmt>" + tokenList.getNowToken().content);
        StmtNode.IfStmtNode ifStmtNode = new StmtNode.IfStmtNode();
        tokenList.goToNextToken();tokenList.goToNextToken();
        ifStmtNode.condNode = parserCondNode();
        tokenList.goToNextToken();
        ifStmtNode.stmtNode = parserStmtNode();
        if(tokenList.getNowToken().wordType.equals(TokenType.ELSE)){
            tokenList.goToNextToken();
            ifStmtNode.elseStmtNode = parserStmtNode();
        }
        return ifStmtNode;
    }

    public ExpNode parserExpNode(){
        if(PRINT_DEBUG_MOD) System.out.println("<ExpNode>" + tokenList.getNowToken().content);
        ExpNode expNode = new ExpNode();
        expNode.addExpNode = parserAddExpNode();
        return expNode;
    }

    public CondNode parserCondNode(){
        if(PRINT_DEBUG_MOD) System.out.println("<Cond>" + tokenList.getNowToken().content);
        CondNode condNode = new CondNode();
        condNode.lOrExpNode = parserLOrExpNode();
        return condNode;
    }

    public LValNode parserLValNode(){
        if(PRINT_DEBUG_MOD) System.out.println("<LVal>" + tokenList.getNowToken().content);
        LValNode lValNode = new LValNode();
        lValNode.identToken = tokenList.getNowToken();
        tokenList.goToNextToken();
        while(tokenList.getNowToken().wordType.equals(TokenType.L_BRACK)){
            tokenList.goToNextToken();
            lValNode.expNodeArrayList.add(parserExpNode());
            tokenList.goToNextToken();
        }
        return lValNode;
    }

    public PrimaryExpNode parserPrimaryExpNode(){
        if(PRINT_DEBUG_MOD) System.out.println("<PrimaryExp>" + tokenList.getNowToken().content);
        PrimaryExpNode primaryExpNode = new PrimaryExpNode();
        if(tokenList.getNowToken().wordType.equals(TokenType.L_PAREN)){
            primaryExpNode.nodeType = PrimaryExpNode.expNodeType;
            tokenList.goToNextToken();
            primaryExpNode.expNode = parserExpNode();
            tokenList.goToNextToken();
        }
        else if(tokenList.getNowToken().wordType.equals(TokenType.IDENTIFIER)){
            primaryExpNode.nodeType = PrimaryExpNode.lValNodeType;
            primaryExpNode.lValNode = parserLValNode();
        }
        else{
            primaryExpNode.nodeType = PrimaryExpNode.numberNodeType;
            primaryExpNode.numberNode = parserNumberNode();
        }
        return primaryExpNode;
    }

    public NumberNode parserNumberNode(){
        if(PRINT_DEBUG_MOD) System.out.println("<Number>" + tokenList.getNowToken().content);
        NumberNode numberNode = new NumberNode();
        numberNode.numberTypeToken = tokenList.getNowToken();
        tokenList.goToNextToken();
        return numberNode;
    }

    public UnaryExpNode parserUnaryExpNode(){
        if(PRINT_DEBUG_MOD) System.out.println("<UnaryExp>" + tokenList.getNowToken().content);
        UnaryExpNode unaryExpNode = new UnaryExpNode();
        if(tokenList.getNowToken().wordType.equals(TokenType.ADD) || tokenList.getNowToken().wordType.equals(TokenType.SUB) || tokenList.getNowToken().wordType.equals(TokenType.NOT)){
            unaryExpNode.nodeType = UnaryExpNode.unaryExpNodeType;
            unaryExpNode.opToken = tokenList.getNowToken();
            tokenList.goToNextToken();
            unaryExpNode.unaryExpNode = parserUnaryExpNode();
        }
        else if(tokenList.getNowToken().wordType.equals(TokenType.IDENTIFIER) && tokenList.preRead(1).wordType.equals(TokenType.L_PAREN)){
            unaryExpNode.nodeType = UnaryExpNode.identExpNodeType;
            unaryExpNode.identToken = tokenList.getNowToken();
            tokenList.goToNextToken();
            tokenList.goToNextToken();
            if(!tokenList.getNowToken().wordType.equals(TokenType.R_PAREN)){
                while(true){
                    unaryExpNode.expParamNodeList.add(parserExpNode());
                    Token token = tokenList.getNowToken();
                    if(token.wordType.equals(TokenType.COMMA))tokenList.goToNextToken();
                    else if(token.wordType.equals(TokenType.R_PAREN)) break;
                }
            }
            tokenList.goToNextToken();
        }
        else{
            unaryExpNode.nodeType = UnaryExpNode.primaryExpNodeType;
            unaryExpNode.primaryExpNode = parserPrimaryExpNode();
        }
        return unaryExpNode;
    }

    public MulExpNode parserMulExpNode(){
        if(PRINT_DEBUG_MOD) System.out.println("<MulExp>" + tokenList.getNowToken().content);
        MulExpNode mulExpNode = new MulExpNode();
        mulExpNode.unaryExpNodeArrayList.add(parserUnaryExpNode());
        while(true){
            Token token = tokenList.getNowToken();
            if(!(token.wordType.equals(TokenType.MUL) || token.wordType.equals(TokenType.DIV) || token.wordType.equals(TokenType.MOD))){
                break;
            }
            mulExpNode.opList.add(token);
            tokenList.goToNextToken();
            mulExpNode.unaryExpNodeArrayList.add(parserUnaryExpNode());
        }
        return mulExpNode;
    }

    public AddExpNode parserAddExpNode(){
        if(PRINT_DEBUG_MOD) System.out.println("<AddExp>" + tokenList.getNowToken().content);
        AddExpNode addExpNode = new AddExpNode();
        addExpNode.mulExpNodeArrayList.add(parserMulExpNode());
        while(true){
            Token token = tokenList.getNowToken();
            if(!(token.wordType.equals(TokenType.ADD) || token.wordType.equals(TokenType.SUB))){
                break;
            }
            addExpNode.opList.add(token);
            tokenList.goToNextToken();
            addExpNode.mulExpNodeArrayList.add(parserMulExpNode());
        }
        return addExpNode;
    }

    public LOrExpNode parserLOrExpNode(){
        if(PRINT_DEBUG_MOD) System.out.println("<LorExp>" + tokenList.getNowToken().content);
        LOrExpNode lOrExpNode = new LOrExpNode();
        lOrExpNode.lAndExpNodeArrayList.add(parserLAndExpNode());
        while(true){
            Token token = tokenList.getNowToken();
            if(token.wordType != TokenType.LOR){
                break;
            }
            tokenList.goToNextToken();
            lOrExpNode.lAndExpNodeArrayList.add(parserLAndExpNode());
        }
        return lOrExpNode;
    }

    public LAndExpNode parserLAndExpNode(){
        if(PRINT_DEBUG_MOD) System.out.println("<LandExp>" + tokenList.getNowToken().content);
        LAndExpNode lAndExpNode = new LAndExpNode();
        lAndExpNode.eqExpNodeArrayList.add(parserEqExpNode());
        while(true){
            Token token = tokenList.getNowToken();
            if(token.wordType != TokenType.LAND){
                break;
            }
            tokenList.goToNextToken();
            lAndExpNode.eqExpNodeArrayList.add(parserEqExpNode());
        }
        return lAndExpNode;
    }

    public EqExpNode parserEqExpNode(){
        if(PRINT_DEBUG_MOD) System.out.println("<EqExp>" + tokenList.getNowToken().content);
        EqExpNode eqExpNode = new EqExpNode();
        eqExpNode.relExpNodeArrayList.add(parserRelExpNode());
        while(true){
            Token token = tokenList.getNowToken();
            if(!(token.wordType.equals(TokenType.EQ) || token.wordType.equals(TokenType.NE))){
                break;
            }
            eqExpNode.opList.add(token);
            tokenList.goToNextToken();
            eqExpNode.relExpNodeArrayList.add(parserRelExpNode());
        }
        return eqExpNode;
    }

    public RelExpNode parserRelExpNode(){
        if(PRINT_DEBUG_MOD) System.out.println("<RelExp>" + tokenList.getNowToken().content);
        RelExpNode relExpNode = new RelExpNode();
        relExpNode.addExpNodeArrayList.add(parserAddExpNode());
        while(true){
            Token token = tokenList.getNowToken();
            if(!(token.wordType.equals(TokenType.GE) || token.wordType.equals(TokenType.GT)
            || token.wordType.equals(TokenType.LE) || token.wordType.equals(TokenType.LT))){
                break;
            }
            relExpNode.opList.add(token);
            tokenList.goToNextToken();
            relExpNode.addExpNodeArrayList.add(parserAddExpNode());
        }
        return relExpNode;
    }
}
