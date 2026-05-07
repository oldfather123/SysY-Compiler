package frontend;

import driver.Config;
import frontend.ast.AbstractSyntaxTree;
import frontend.lexer.Lexer;
import frontend.lexer.Token;
import frontend.lexer.TokenStream;
import frontend.parser.Parser;
import ir.IrFactory;
import ir.traversal.IrSpawner;
import ir.traversal.ScopeSpecifier;
import ir.traversal.SymbolTable;

import java.io.IOException;
import java.util.List;


public class Controller {
    public static IrFactory start() throws IOException {
        Lexer lexer = new Lexer(Config.source);
        lexer.scan();

        // Tokenize the input
        List<Token> tokens = lexer.getTokens();
        if (Config.emitAst) {
            System.out.println(tokens);
        }

        // Parse the tokens, generate AST
        Parser parser = new Parser(new TokenStream(tokens));
        parser.parse();
        List<AbstractSyntaxTree.CompileUnit> compileUnits = parser.getCompileUnits();

        ScopeSpecifier scopeSpecifier = new ScopeSpecifier(compileUnits);
        SymbolTable symbolTable = scopeSpecifier.traverse();

        if (Config.emitAst) {
            System.out.println(symbolTable.debugPrint(0));
            System.out.println(compileUnits);
        }

        // Generate IR
        IrSpawner spawner = new IrSpawner(symbolTable, Config.source);

        return spawner.spawn();
    }
}
