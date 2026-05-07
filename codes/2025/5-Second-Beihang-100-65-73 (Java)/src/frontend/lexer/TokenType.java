package frontend.lexer;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.regex.Pattern;

public enum TokenType {
    // keyword (must have lookahead assertion)
    CONST("const", "CONSTTK"),
    INT("int", "INTTK"),
    FLOAT("float", "FLOATTK"),
    BREAK("break", "BREAKTK"),
    CONTINUE("continue", "CONTINUETK"),
    IF("if", "IFTK"),
    ELSE("else", "ELSETK"),
    VOID("void", "VOIDTK"),
    WHILE("while", "WHILETK"),
    RETURN("return", "RETURNTK"),
    
    // ident
    IDENT("[A-Za-z_][A-Za-z0-9_]*", "IDENFR"),
    
    // int const
    HEX_INT_CON("0(x|X)[0-9A-Fa-f]+", "INTCON"),
    OCT_INT_CON("0[0-7]+", "INTCON"),
    DEC_INT_CON("0|([1-9][0-9]*)", "INTCON"), // decimal
    
    // float const
    HEX_FLOAT_CON("(0(x|X)[0-9A-Fa-f]*\\.[0-9A-Fa-f]*((p|P|e|E)(\\+|\\-)?[0-9A-Fa-f]*)?)|" +
            "(0(x|X)[0-9A-Fa-f]*[\\.]?[0-9A-Fa-f]*(p|P|e|E)((\\+|\\-)?[0-9A-Fa-f]*)?)", "FLOATCON"),
    DEC_FLOAT_CON("([0-9]*\\.[0-9]*((p|P|e|E)(\\+|\\-)?[0-9]+)?)|" +
            "([0-9]*[\\.]?[0-9]*(p|P|e|E)((\\+|\\-)?[0-9]+)?)", "FLOATCON"), // decimal
    
    // string const
    STR_CON("\"[\\x00-\\x7F]*\"", "STRCON"),
    
    // operator (double char)
    LAND("&&", "AND"),
    LOR("\\|\\|", "OR"),
    LE("<=", "LEQ"),
    GE(">=", "GEQ"),
    EQ("==", "EQL"),
    NE("!=", "NEQ"),
    
    // operator (single char)
    LT("<", "LSS"),
    GT(">", "GRE"),
    ADD("\\+", "PLUS"), // regex
    SUB("-", "MINU"),
    MUL("\\*", "MULT"),
    DIV("/", "DIV"),
    MOD("%", "MOD"),
    NOT("!", "NOT"),
    ASSIGN("=", "ASSIGN"),
    SEMI(";", "SEMICN"),
    COMMA(",", "COMMA"),
    LPARENT("\\(", "LPARENT"),
    RPARENT("\\)", "RPARENT"),
    LBRACK("\\[", "LBRACK"),
    RBRACK("]", "RBRACK"),
    LBRACE("\\{", "LBRACE"),
    RBRACE("}", "RBRACE"),
    ;
    
    public static final ArrayList<TokenType> NUM_CONST_TYPES =
            new ArrayList<>(Arrays.asList(HEX_INT_CON, DEC_INT_CON, OCT_INT_CON, HEX_FLOAT_CON, DEC_FLOAT_CON));
    private final String pattern;   // regex pattern
    private final String classCode;
    
    TokenType(String pattern, String classCode) {
        this.pattern = pattern;
        this.classCode = classCode;
    }
    
    public Pattern getPattern() {
        return Pattern.compile("^(" + pattern + ")");
    }
    
    public String getClassCode() {
        return classCode;
    }
    
    public boolean isNumConst() {
        return NUM_CONST_TYPES.contains(this);
    }
}
