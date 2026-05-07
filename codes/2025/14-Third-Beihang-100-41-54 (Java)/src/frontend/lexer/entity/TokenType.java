package frontend.lexer.entity;
import java.util.HashMap;
import java.util.Map;
import java.util.regex.Pattern;

public enum TokenType {
    INT("int"),
    FLOAT("float"),
    VOID("void"),
    RETURN("return"),
    CONST("const"),
    IF("if"),
    WHILE("while"),
    BREAK("break"),
    CONTINUE("continue"),
    PUT_F("putf"),
    ELSE("else"),
    IDENTIFIER("[A-Za-z_][A-Za-z0-9_]*"),
    L_PAREN("\\("),
    R_PAREN("\\)"),
    L_BRACE("\\{"),
    R_BRACE("\\}"),
    L_BRACK("\\["),
    R_BRACK("]"),
    LOR("\\|\\|"),
    LAND("&&"),
    EQ("=="),
    NE("!="),
    LE("<="),
    GE(">="),
    LT("<"),
    GT(">"),
    ADD("\\+"),
    SUB("-"),
    MUL("\\*"),
    DIV("/"),
    MOD("%"),
    NOT("!"),
    XOR("^"),
    ASSIGN("="),
    COMMA(","),
    DEC_FLOAT("^([0-9]*+\\.[0-9]*([eEpP][\\+\\-]?[0-9]+)?|[0-9]+([eEpP][\\+\\-]?[0-9]+))$"),

    HEX_FLOAT("^0[xX]([0-9A-Fa-f]+\\.?[0-9A-Fa-f]*|\\.[0-9A-Fa-f]+)[pP][\\+\\-]?[0-9]+$"),
    DEC_INT("0|([1-9][0-9]*)"),
    HEX_INT("0(x|X)[0-9A-Fa-f]+"),
    OCT_INT("0[0-7]+"),
    SEMI(";"),
    STR("\"[^\"]*\""),
    EOF("");

    public final String content;

    private static final Map<TokenType, Pattern> patternCache = new HashMap<>();

    static {
        // 预编译所有正则表达式
        for (TokenType type : TokenType.values()) {
            if (!type.content.isEmpty()) {
                patternCache.put(type, Pattern.compile("^" + type.content + "$"));
            }
        }
    }

    /**
     * 根据传入的字符串推断WordType类型
     * @param input 待解析的字符串
     * @return 对应的WordType，如果无法识别则返回null
     */
    public static TokenType parseWordType(String input) {
        if (input == null) {
            return null;
        }

        // 处理EOF特殊情况
        if (input.isEmpty()) {
            return TokenType.EOF;
        }

        // 按照优先级顺序检查各种类型

        // 1. 首先检查关键字（精确匹配）
        TokenType keywordType = checkKeywords(input);
        if (keywordType != null) {
            return keywordType;
        }

        // 2. 检查字符串字面量
        if (matches(input, TokenType.STR)) {
            return TokenType.STR;
        }

        // 3. 检查数字类型（按优先级顺序）
        if (matches(input, TokenType.HEX_FLOAT)) {
            return TokenType.HEX_FLOAT;
        }
        if (matches(input, TokenType.DEC_FLOAT)) {
            return TokenType.DEC_FLOAT;
        }
        if (matches(input, TokenType.HEX_INT)) {
            return TokenType.HEX_INT;
        }
        if (matches(input, TokenType.OCT_INT)) {
            return TokenType.OCT_INT;
        }
        if (matches(input, TokenType.DEC_INT)) {
            return TokenType.DEC_INT;
        }

        // 4. 检查操作符（按长度从长到短检查，避免短操作符覆盖长操作符）
        if (matches(input, TokenType.LOR)) {
            return TokenType.LOR;
        }
        if (matches(input, TokenType.LAND)) {
            return TokenType.LAND;
        }
        if (matches(input, TokenType.EQ)) {
            return TokenType.EQ;
        }
        if (matches(input, TokenType.NE)) {
            return TokenType.NE;
        }
        if (matches(input, TokenType.LE)) {
            return TokenType.LE;
        }
        if (matches(input, TokenType.GE)) {
            return TokenType.GE;
        }
        if (matches(input, TokenType.LT)) {
            return TokenType.LT;
        }
        if (matches(input, TokenType.GT)) {
            return TokenType.GT;
        }

        // 5. 检查单字符操作符和分隔符
        if (matches(input, TokenType.L_PAREN)) {
            return TokenType.L_PAREN;
        }
        if (matches(input, TokenType.R_PAREN)) {
            return TokenType.R_PAREN;
        }
        if (matches(input, TokenType.L_BRACE)) {
            return TokenType.L_BRACE;
        }
        if (matches(input, TokenType.R_BRACE)) {
            return TokenType.R_BRACE;
        }
        if (matches(input, TokenType.L_BRACK)) {
            return TokenType.L_BRACK;
        }
        if (matches(input, TokenType.R_BRACK)) {
            return TokenType.R_BRACK;
        }
        if (matches(input, TokenType.ADD)) {
            return TokenType.ADD;
        }
        if (matches(input, TokenType.SUB)) {
            return TokenType.SUB;
        }
        if (matches(input, TokenType.MUL)) {
            return TokenType.MUL;
        }
        if (matches(input, TokenType.DIV)) {
            return TokenType.DIV;
        }
        if (matches(input, TokenType.MOD)) {
            return TokenType.MOD;
        }
        if (matches(input, TokenType.NOT)) {
            return TokenType.NOT;
        }
        if (matches(input, TokenType.ASSIGN)) {
            return TokenType.ASSIGN;
        }
        if (matches(input, TokenType.COMMA)) {
            return TokenType.COMMA;
        }
        if (matches(input, TokenType.SEMI)) {
            return TokenType.SEMI;
        }

        // 6. 最后检查标识符（必须放在最后，因为它的匹配范围最广）
        if (matches(input, TokenType.IDENTIFIER)) {
            return TokenType.IDENTIFIER;
        }
        throw new RuntimeException("can't recognize token: " + input);
    }

    /**
     * 检查是否为关键字
     * @param input 输入字符串
     * @return 对应的关键字类型，如果不是关键字则返回null
     */
    private static TokenType checkKeywords(String input) {
        return switch (input) {
            case "int" -> TokenType.INT;
            case "float" -> TokenType.FLOAT;
            case "void" -> TokenType.VOID;
            case "return" -> TokenType.RETURN;
            case "const" -> TokenType.CONST;
            case "if" -> TokenType.IF;
            case "while" -> TokenType.WHILE;
            case "break" -> TokenType.BREAK;
            case "continue" -> TokenType.CONTINUE;
            case "else" -> TokenType.ELSE;
            case "putf" -> TokenType.PUT_F;
            default -> null;
        };
    }

    /**
     * 检查输入字符串是否匹配指定的WordType模式
     * @param input 输入字符串
     * @param type WordType类型
     * @return 是否匹配
     */
    private static boolean matches(String input, TokenType type) {
        Pattern pattern = patternCache.get(type);
        return pattern != null && pattern.matcher(input).matches();
    }

    TokenType(String content) {
        this.content = content;
    }
}
