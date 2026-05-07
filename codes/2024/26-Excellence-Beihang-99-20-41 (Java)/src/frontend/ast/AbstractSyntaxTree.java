package frontend.ast;

import frontend.lexer.TokenType;
import ir.traversal.SymbolTable;
import utils.RefCell;

import java.util.List;

public class AbstractSyntaxTree {
    public interface CompileUnit {
    }

    public interface BlockItem extends CompileUnit {
    }

    public interface Declaration extends BlockItem {
    }

    public interface Statement extends BlockItem {
    }

    public static class SymbolRedirection {
        private SymbolTable.SymbolId targetSymbol = null;

        public void setTargetSymbol(SymbolTable.SymbolId targetSymbol) {
            if (this.targetSymbol != null) {
                throw new RuntimeException("Target symbol already set");
            }
            if (targetSymbol == null) {
                throw new RuntimeException("Target symbol cannot be null");
            }
            this.targetSymbol = targetSymbol;
        }

        public SymbolTable.SymbolId getTargetSymbol() {
            return targetSymbol;
        }

        @Override
        public String toString() {
            return targetSymbol == null ? "null" : targetSymbol.toString();
        }
    }

    public record AddExpr(MulExpr leading, List<ExprWithLeading<MulExpr>> follows)
        implements CompileUnit {
    }

    public record MulExpr(UnaryExpr leading, List<ExprWithLeading<UnaryExpr>> follows)
        implements CompileUnit {
    }

    // body 为 FuncCall 或 PrimaryExpr
    public record UnaryExpr(List<TokenType> leadingSymbols, Object body) implements CompileUnit {
    }

    // body 为 AddExpr 或 LVal 或 Number
    public record PrimaryExpr(Object body) implements CompileUnit {
    }

    public record FuncCall(String funcName, List<AddExpr> args) implements CompileUnit {
    }

    public record OrExpr(List<AndExpr> items) implements CompileUnit {
    }

    public record AndExpr(List<EqExpr> items) implements CompileUnit {
    }

    public record EqExpr(RelExpr leading,
                         List<ExprWithLeading<RelExpr>> follows) implements CompileUnit {
    }

    public record RelExpr(AddExpr leading, List<ExprWithLeading<AddExpr>> follows)
        implements CompileUnit {
    }

    public record LVal(String ident, List<AddExpr> indexes,
                       SymbolRedirection redirection) implements CompileUnit {
    }

    public record Number(Integer valueInt, Float valueFloat) implements CompileUnit {
    }

    public record Block(List<BlockItem> items, RefCell<SymbolTable> symbols) implements Statement {
    }

    public record AssignStmt(LVal lvalue, AddExpr rvalue) implements Statement {
    }

    public record BranchStmt(OrExpr condition, Statement thenBlock, Statement elseBlock)
        implements Statement {
    }

    public record WhileStmt(OrExpr condition, Statement body) implements Statement {
    }

    public record ReturnStmt(AddExpr returnExpr) implements Statement {
    }

    public record LoopControlStmt(TokenType type) implements Statement {
    }

    public record ExprStmt(AddExpr expr) implements Statement {
    }

    public record VarDecl(boolean isConst, TokenType type,
                          List<VarDef> decls) implements Declaration {
    }

    public record VarDef(String ident, List<AddExpr> indexes, InitVal init,
                         SymbolRedirection redirection) implements CompileUnit {
    }

    public record FuncDecl(TokenType returnType, String funcName, List<FuncParam> params,
                           Block body)
        implements Declaration {
    }

    public record FuncParam(TokenType type, String ident, List<AddExpr> indexes,
                            SymbolRedirection redirection) implements CompileUnit {
    }

    // 两个参数只有一个有效，另一个为 null
    public record InitVal(AddExpr expr, List<InitVal> values) implements CompileUnit {
    }
}
