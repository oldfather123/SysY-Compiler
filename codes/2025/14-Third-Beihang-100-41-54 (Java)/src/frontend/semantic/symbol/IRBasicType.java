package frontend.semantic.symbol;

import frontend.lexer.entity.TokenType;
import frontend.syntactic.entity.ast.BTypeNode;
import frontend.syntactic.entity.ast.FuncTypeNode;

public enum IRBasicType {
    I1("i1"),
    I32("i32"),
    I64("i64"),
    I8("i8"),
    FLOAT("float"),
    VOID("void"),
    NONE("");

    private final String IRTypeName;
    public static boolean isInt(IRBasicType IRBasicType){
        return IRBasicType == I32 || IRBasicType == I64;
    }
    public static boolean isFloat(IRBasicType IRBasicType){
        return IRBasicType == FLOAT;
    }
    public static boolean isChar(IRBasicType IRBasicType){
        return IRBasicType == I8;
    }
    public static boolean isBoolean(IRBasicType IRBasicType){
        return IRBasicType == I1;
    }
    IRBasicType(String IRTypeName) {
        this.IRTypeName = IRTypeName;
    }
    public static IRBasicType BTNode2BT(BTypeNode bTypeNode){
        if(bTypeNode.tokenType.equals(TokenType.INT)){
            return I32;
        }
        else if(bTypeNode.tokenType.equals(TokenType.FLOAT)){
            return FLOAT;
        }
        else if(bTypeNode.tokenType.equals(TokenType.VOID)){
            return VOID;
        }
        System.out.println("the basic type is illegal");
        return NONE;
    }
    public static IRBasicType FTNode2BT(FuncTypeNode funcTypeNode) {
        TokenType tokenType = funcTypeNode.typeToken.wordType;
        if (tokenType.equals(TokenType.INT)) {
            return I32;
        } else if (tokenType.equals(TokenType.FLOAT)) {
            return FLOAT;
        } else if (tokenType.equals(TokenType.VOID)) {
            return VOID;
        }
        System.out.println("the basic type is illegal");
        return NONE;
    }
    @Override
    public String toString() {
        return this.IRTypeName;
    }
}
