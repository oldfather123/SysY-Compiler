package frontend.parser;

import frontend.ast.AbstractSyntaxTree;
import frontend.ast.ExprWithLeading;
import frontend.lexer.Token;
import frontend.lexer.TokenStream;
import frontend.lexer.TokenType;
import utils.RefCell;

import java.util.LinkedList;
import java.util.List;

public class Parser {
    private final TokenStream tks;
    private final List<AbstractSyntaxTree.CompileUnit> compileUnits;

    public Parser(TokenStream tks) {
        this.tks = tks;
        this.compileUnits = new LinkedList<>();
    }

    public void parse() {
        while (tks.hasNext()) {
            if (tks.peek(2).in(TokenType.LPARENT)) {
                compileUnits.add(parseFuncDef());
            } else {
                compileUnits.add(parseDeclaration());
            }
        }
    }

    public List<AbstractSyntaxTree.CompileUnit> getCompileUnits() {
        return compileUnits;
    }

    private AbstractSyntaxTree.FuncDecl parseFuncDef() {
        /// FuncDef -> FuncType Ident '(' [FuncFParams] ')' Block
        Token type = tks.next();
        Token ident = tks.next();
        tks.consume(TokenType.LPARENT);
        List<AbstractSyntaxTree.FuncParam> funcParams = parseFuncParams();
        tks.consume(TokenType.RPARENT);
        AbstractSyntaxTree.Block block = parseBlock();
        return new AbstractSyntaxTree.FuncDecl(type.getType(), ident.getIdent(), funcParams, block);
    }

    private List<AbstractSyntaxTree.FuncParam> parseFuncParams() {
        List<AbstractSyntaxTree.FuncParam> funcParams = new LinkedList<>();
        while (!tks.check(TokenType.RPARENT)) {
            Token type = tks.next();
            Token ident = tks.next();
            List<AbstractSyntaxTree.AddExpr> indexes = new LinkedList<>();
            if (tks.checkConsume(TokenType.LSQUARE)) {
                tks.consume(TokenType.RSQUARE);
                indexes.add(null);
                while (tks.checkConsume(TokenType.LSQUARE)) {
                    indexes.add(parseExpr());
                    tks.consume(TokenType.RSQUARE);
                }
            }
            tks.checkConsume(TokenType.COMMA);
            funcParams.add(new AbstractSyntaxTree.FuncParam(
                type.getType(), ident.getIdent(), indexes, new AbstractSyntaxTree.SymbolRedirection()));
        }
        return funcParams;
    }

    private AbstractSyntaxTree.Block parseBlock() {
        /// Block -> '{' { BlockItem } '}'
        List<AbstractSyntaxTree.BlockItem> items = new LinkedList<>();
        tks.consume(TokenType.LBRACE);
        /// BlockItem -> Decl | Stmt
        while (!tks.check(TokenType.RBRACE)) {
            if (tks.check(TokenType.CONST, TokenType.INT, TokenType.FLOAT)) {
                items.add(parseDeclaration());
            } else {
                items.add(parseStatement());
            }
        }
        tks.consume(TokenType.RBRACE);
        return new AbstractSyntaxTree.Block(items, new RefCell<>(null));
    }

    private AbstractSyntaxTree.Declaration parseDeclaration() {
        /// ConstDecl -> 'const' BType ConstDef { ',' ConstDef } ';'
        /// VarDecl -> BType VarDef { ',' VarDef } ';'
        boolean isConst = tks.checkConsume(TokenType.CONST);
        Token type = tks.next();
        List<AbstractSyntaxTree.VarDef> varDefs = new LinkedList<>();
        do {
            Token ident = tks.next();
            /// { '[' Exp ']' }
            List<AbstractSyntaxTree.AddExpr> indexes = new LinkedList<>();
            while (tks.checkConsume(TokenType.LSQUARE)) {
                indexes.add(parseExpr());
                tks.consume(TokenType.RSQUARE);
            }
            if (tks.checkConsume(TokenType.ASSIGN)) {
                varDefs.add(new AbstractSyntaxTree.VarDef(
                    ident.getIdent(), indexes, parseInitVal(), new AbstractSyntaxTree.SymbolRedirection()));
            } else {
                varDefs.add(new AbstractSyntaxTree.VarDef(
                    ident.getIdent(), indexes, null, new AbstractSyntaxTree.SymbolRedirection()));
            }
        } while (tks.checkConsume(TokenType.COMMA));

        tks.consume(TokenType.SEMICOLON);

        return new AbstractSyntaxTree.VarDecl(isConst, type.getType(), varDefs);
    }

    private AbstractSyntaxTree.InitVal parseInitVal() {
        if (tks.checkConsume(TokenType.LBRACE)) {
            List<AbstractSyntaxTree.InitVal> initVals = new LinkedList<>();
            while (!tks.check(TokenType.RBRACE)) {
                initVals.add(parseInitVal());
                tks.checkConsume(TokenType.COMMA);
            }
            tks.consume(TokenType.RBRACE);
            return new AbstractSyntaxTree.InitVal(null, initVals);
        } else {
            return new AbstractSyntaxTree.InitVal(parseExpr(), null);
        }
    }

    private AbstractSyntaxTree.Statement parseStatement() {
        if (tks.check(TokenType.LBRACE)) {
            return parseBlock();
        } else if (tks.checkConsume(TokenType.IF)) {
            tks.consume(TokenType.LPARENT);
            AbstractSyntaxTree.OrExpr condition = parseCondition();
            tks.consume(TokenType.RPARENT);
            AbstractSyntaxTree.Statement thenBlock = parseStatement();
            if (tks.checkConsume(TokenType.ELSE)) {
                return new AbstractSyntaxTree.BranchStmt(condition, thenBlock, parseStatement());
            } else {
                return new AbstractSyntaxTree.BranchStmt(condition, thenBlock, null);
            }
        } else if (tks.checkConsume(TokenType.WHILE)) {
            tks.consume(TokenType.LPARENT);
            AbstractSyntaxTree.OrExpr condition = parseCondition();
            tks.consume(TokenType.RPARENT);
            return new AbstractSyntaxTree.WhileStmt(condition, parseStatement());
        } else if (tks.checkConsume(TokenType.RETURN)) {
            if (tks.checkConsume(TokenType.SEMICOLON)) {
                return new AbstractSyntaxTree.ReturnStmt(null);
            } else {
                AbstractSyntaxTree.ReturnStmt ret = new AbstractSyntaxTree.ReturnStmt(parseExpr());
                tks.consume(TokenType.SEMICOLON);
                return ret;
            }
        } else if (tks.check(TokenType.BREAK, TokenType.CONTINUE)) {
            TokenType ctrl = tks.next().getType();
            tks.consume(TokenType.SEMICOLON);
            return new AbstractSyntaxTree.LoopControlStmt(ctrl);
        } else {
            /// LVal '=' Exp ';' | [ Exp ] ';'
            int i = 0;
            while (!tks.peek(i).in(TokenType.SEMICOLON) && !tks.peek(i).in(TokenType.ASSIGN)) {
                i++;
            }
            if (tks.peek(i).in(TokenType.ASSIGN)) {
                /// LVal '=' Exp ';'
                AbstractSyntaxTree.LVal lvalue = parseLVal();
                tks.consume(TokenType.ASSIGN);
                AbstractSyntaxTree.AddExpr expr = parseExpr();
                tks.consume(TokenType.SEMICOLON);
                return new AbstractSyntaxTree.AssignStmt(lvalue, expr);
            } else if (tks.checkConsume(TokenType.SEMICOLON)) {
                /// [ Exp ] ';'
                // The semicolon has been consumed
                return new AbstractSyntaxTree.ExprStmt(null);  // Pay attention to this!
            } else {
                /// Exp ';'
                AbstractSyntaxTree.AddExpr expr = parseExpr();
                tks.consume(TokenType.SEMICOLON);
                return new AbstractSyntaxTree.ExprStmt(expr);
            }
        }
    }

    private AbstractSyntaxTree.LVal parseLVal() {
        Token ident = tks.next();
        List<AbstractSyntaxTree.AddExpr> indexes = new LinkedList<>();
        while (tks.checkConsume(TokenType.LSQUARE)) {
            indexes.add(parseExpr());
            tks.consume(TokenType.RSQUARE);
        }
        return new AbstractSyntaxTree.LVal(ident.getIdent(), indexes, new AbstractSyntaxTree.SymbolRedirection());
    }

    private AbstractSyntaxTree.AddExpr parseExpr() {
        AbstractSyntaxTree.MulExpr leading = parseMulExpr();
        List<ExprWithLeading<AbstractSyntaxTree.MulExpr>> follows = new LinkedList<>();
        while (tks.check(TokenType.ADD, TokenType.SUB)) {
            follows.add(new ExprWithLeading<>(tks.next().getType(), parseMulExpr()));
        }
        return new AbstractSyntaxTree.AddExpr(leading, follows);
    }

    private AbstractSyntaxTree.MulExpr parseMulExpr() {
        AbstractSyntaxTree.UnaryExpr leading = parseUnaryExpr();
        List<ExprWithLeading<AbstractSyntaxTree.UnaryExpr>> follows = new LinkedList<>();
        while (tks.check(TokenType.MUL, TokenType.DIV, TokenType.MOD)) {
            follows.add(new ExprWithLeading<>(tks.next().getType(), parseUnaryExpr()));
        }
        return new AbstractSyntaxTree.MulExpr(leading, follows);
    }

    private AbstractSyntaxTree.UnaryExpr parseUnaryExpr() {
        List<TokenType> leadingSymbols = new LinkedList<>();
        while (tks.check(TokenType.ADD, TokenType.SUB, TokenType.NOT)) {
            leadingSymbols.add(tks.next().getType());
        }
        if (tks.check(TokenType.IDENT) && tks.peek(1).in(TokenType.LPARENT)) {
            // Function Call
            Token funcName = tks.next();
            List<AbstractSyntaxTree.AddExpr> args = new LinkedList<>();
            tks.consume(TokenType.LPARENT);
            while (!tks.check(TokenType.RPARENT)) {
                args.add(parseExpr());
                tks.checkConsume(TokenType.COMMA);
            }
            tks.consume(TokenType.RPARENT);
            if (funcName.getIdent().equals("starttime") || funcName.getIdent().equals("stoptime")) {
                return new AbstractSyntaxTree.UnaryExpr(
                    leadingSymbols, new AbstractSyntaxTree.FuncCall(
                        "_sysy_" + funcName.getIdent(),
                        List.of(new AbstractSyntaxTree.AddExpr(
                            new AbstractSyntaxTree.MulExpr(
                                new AbstractSyntaxTree.UnaryExpr(
                                    List.of(), new AbstractSyntaxTree.PrimaryExpr(
                                        new AbstractSyntaxTree.Number(funcName.getLine(), null)
                                    )
                                ), List.of()
                            ), List.of())
                        )
                    )
                );
            } else {
                return new AbstractSyntaxTree.UnaryExpr(
                    leadingSymbols, new AbstractSyntaxTree.FuncCall(funcName.getIdent(), args));
            }
        } else {
            return new AbstractSyntaxTree.UnaryExpr(leadingSymbols, parsePrimaryExpr());
        }
    }

    private AbstractSyntaxTree.PrimaryExpr parsePrimaryExpr() {
        if (tks.checkConsume(TokenType.LPARENT)) {
            // Primary Expr: '(' Exp ')'
            AbstractSyntaxTree.AddExpr expr = parseExpr();
            tks.consume(TokenType.RPARENT);
            return new AbstractSyntaxTree.PrimaryExpr(expr);
        } else if (tks.check(TokenType.NUMBER)) {
            // Primary Expr: IntConst
            AbstractSyntaxTree.Number number = new AbstractSyntaxTree.Number(tks.next().getInt(), null);
            return new AbstractSyntaxTree.PrimaryExpr(number);
        } else if (tks.check(TokenType.REAL)) {
            // Primary Expr: FloatConst
            AbstractSyntaxTree.Number number = new AbstractSyntaxTree.Number(null, tks.next().getFloat());
            return new AbstractSyntaxTree.PrimaryExpr(number);
        } else {
            // Primary Expr: LVal
            return new AbstractSyntaxTree.PrimaryExpr(parseLVal());
        }
    }

    private AbstractSyntaxTree.OrExpr parseCondition() {
        List<AbstractSyntaxTree.AndExpr> andItems = new LinkedList<>();
        do {
            // [Inline] Parse And Expr
            List<AbstractSyntaxTree.EqExpr> eqItems = new LinkedList<>();
            do {
                // [Inline] Parse Eq Expr
                AbstractSyntaxTree.RelExpr leading = parseRelationExpr();
                List<ExprWithLeading<AbstractSyntaxTree.RelExpr>> follows = new LinkedList<>();
                while (tks.check(TokenType.EQ, TokenType.NEQ)) {
                    follows.add(new ExprWithLeading<>(tks.next().getType(), parseRelationExpr()));
                }
                eqItems.add(new AbstractSyntaxTree.EqExpr(leading, follows));
            } while (tks.checkConsume(TokenType.AND));
            andItems.add(new AbstractSyntaxTree.AndExpr(eqItems));
        } while (tks.checkConsume(TokenType.OR));
        return new AbstractSyntaxTree.OrExpr(andItems);
    }

    private AbstractSyntaxTree.RelExpr parseRelationExpr() {
        AbstractSyntaxTree.AddExpr leading = parseExpr();
        List<ExprWithLeading<AbstractSyntaxTree.AddExpr>> follows = new LinkedList<>();
        while (tks.check(TokenType.GT, TokenType.LT, TokenType.GE, TokenType.LE)) {
            follows.add(new ExprWithLeading<>(tks.next().getType(), parseExpr()));
        }
        return new AbstractSyntaxTree.RelExpr(leading, follows);
    }
}
