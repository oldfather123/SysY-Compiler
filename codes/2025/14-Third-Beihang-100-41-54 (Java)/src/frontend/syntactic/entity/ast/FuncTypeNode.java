package frontend.syntactic.entity.ast;

import frontend.lexer.entity.Token;
import frontend.lexer.entity.TokenType;
import frontend.semantic.symbol.SymbolTable;

import java.io.BufferedWriter;
import java.io.IOException;

public class FuncTypeNode {

    public Token typeToken;

    public String getType(){return typeToken.wordType.content;}

    public boolean isVoid(){return typeToken.wordType.equals(TokenType.VOID);}

    public boolean isInt(){return typeToken.wordType.equals(TokenType.INT);}

    public boolean isFloat(){return typeToken.wordType.equals(TokenType.FLOAT);}

    public void syntacticDebug(BufferedWriter writer) throws IOException {
        writer.write(typeToken.content);
    }
}
