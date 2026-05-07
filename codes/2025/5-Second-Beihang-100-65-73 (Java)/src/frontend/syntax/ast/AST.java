package frontend.syntax.ast;

import frontend.lexer.Token;
import frontend.lexer.TokenType;
import frontend.syntax.ast.nodes.CompUnit;

import java.util.Arrays;
import java.util.HashMap;
import java.util.List;

public record AST(CompUnit compUnit) {
    
    public enum BinaryExpType {
        LOR("<LOrExp>", TokenType.LOR),
        LAND("<LAndExp>", TokenType.LAND),
        EQ("<EqExp>", TokenType.EQ, TokenType.NE),
        REL("<RelExp>", TokenType.GT, TokenType.LT, TokenType.GE, TokenType.LE),
        ADD("<AddExp>", TokenType.ADD, TokenType.SUB),
        MUL("<MulExp>", TokenType.MUL, TokenType.DIV, TokenType.MOD),
        ;
        
        private final String expName;
        private final List<TokenType> types;
        
        BinaryExpType(String expName, TokenType... types) {
            this.expName = expName;
            this.types = Arrays.asList(types);
        }
        
        public boolean containsOp(TokenType type) {
            return types.contains(type);
        }
        
        private static final HashMap<BinaryExpType, BinaryExpType> DOWN_MAP = new HashMap<>(); // 层层下降
        
        static {
            DOWN_MAP.put(BinaryExpType.LOR, BinaryExpType.LAND);
            DOWN_MAP.put(BinaryExpType.LAND, BinaryExpType.EQ);
            DOWN_MAP.put(BinaryExpType.EQ, BinaryExpType.REL);
            DOWN_MAP.put(BinaryExpType.REL, BinaryExpType.ADD);
            DOWN_MAP.put(BinaryExpType.ADD, BinaryExpType.MUL);
        }
        
        public BinaryExpType getDownType() {
            return DOWN_MAP.get(this);
        }
        
        @Override
        public String toString() {
            return this.expName;
        }
    }
    
    public enum BType {
        INT,
        FLOAT,
        ;
        
        public static BType token2Btype(Token typeToken) {
            if (typeToken == null) {
                throw new NullPointerException();
            }
            return switch (typeToken.getType()) {
                case INT   -> BType.INT;
                case FLOAT -> BType.FLOAT;
                default -> throw new RuntimeException("基本类型只能是 int 或者 char");
            };
        }
    }
    
    public enum FuncType {
        INT,
        FLOAT,
        VOID,
        ;
        
        public static FuncType token2FuncType(Token typeToken) {
            if (typeToken == null) {
                throw new NullPointerException();
            }
            return switch (typeToken.getType()) {
                case INT   -> FuncType.INT;
                case FLOAT -> FuncType.FLOAT;
                case VOID  -> FuncType.VOID;
                default -> throw new RuntimeException("函数类型只能是 int，char 或者 void");
            };
        }
    }
    
    public enum OperatorName {
        LOR,
        LAND,
        EQ,
        NE,
        GT,
        LT,
        GE,
        LE,
        ADD,
        SUB,
        MUL,
        DIV,
        MOD,
        NOT,
        ;
        
        public static OperatorName token2OperatorName(Token typeToken) {
            if (typeToken == null) {
                throw new NullPointerException();
            }
            return switch (typeToken.getType()) {
                case LOR -> OperatorName.LOR;
                case LAND -> OperatorName.LAND;
                case EQ -> OperatorName.EQ;
                case NE -> OperatorName.NE;
                case GT -> OperatorName.GT;
                case LT -> OperatorName.LT;
                case GE -> OperatorName.GE;
                case LE -> OperatorName.LE;
                case ADD -> OperatorName.ADD;
                case SUB -> OperatorName.SUB;
                case MUL -> OperatorName.MUL;
                case DIV -> OperatorName.DIV;
                case MOD -> OperatorName.MOD;
                case NOT -> OperatorName.NOT;
                default -> throw new RuntimeException("未曾设想的运算符： " + typeToken);
            };
        }
    }
    
    public static abstract class Node {
        private final int lineno;
        
        protected Node(int lineno) {
            this.lineno = lineno;
        }
        
        public int getLineno() {
            return lineno;
        }
    }
}
