package frontend.syntactic.entity.ast;

import frontend.lexer.entity.Token;
import frontend.lexer.entity.TokenType;
import frontend.semantic.symbol.SymbolTable;

import java.io.BufferedWriter;
import java.io.IOException;

public class NumberNode {

    public Token numberTypeToken;

    public boolean isInt(){
        return numberTypeToken.wordType.equals(TokenType.DEC_INT)
                || numberTypeToken.wordType.equals(TokenType.HEX_INT)
                || numberTypeToken.wordType.equals(TokenType.OCT_INT);
    }
    public boolean isFloat(){
        return numberTypeToken.wordType.equals(TokenType.FLOAT)
                || numberTypeToken.wordType.equals(TokenType.HEX_FLOAT) || numberTypeToken.wordType.equals(TokenType.DEC_FLOAT);
    }

    public void syntacticDebug(BufferedWriter writer) throws IOException {
        writer.write("<Number> " + numberTypeToken.content + "\n");
    }
}

