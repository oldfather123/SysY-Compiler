package frontend.syntactic.entity.ast;

import frontend.lexer.entity.TokenType;

import java.io.BufferedWriter;
import java.io.IOException;

public class BTypeNode {

    public TokenType tokenType;

    public void syntacticDebug(BufferedWriter writer) throws IOException {
        writer.write(tokenType.content + " ");
    }
}
