package frontend.lexer;

import utils.Panic;

import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.PushbackReader;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import static java.util.Map.entry;

public class Lexer {
    private final PushbackReader file;
    private int line = 1;
    private final List<Token> tokens;

    private final static Map<String, TokenType> keywords = Map.of(
        "const", TokenType.CONST, "int", TokenType.INT, "float", TokenType.FLOAT,
        "break", TokenType.BREAK, "continue", TokenType.CONTINUE, "if", TokenType.IF,
        "else", TokenType.ELSE, "while", TokenType.WHILE, "return", TokenType.RETURN,
        "void", TokenType.VOID
    );

    private final static Map<Character, TokenType> symbols = Map.ofEntries(
        entry('+', TokenType.ADD), entry('-', TokenType.SUB), entry('*', TokenType.MUL),
        entry('/', TokenType.DIV), entry('%', TokenType.MOD), entry('{', TokenType.LBRACE),
        entry('}', TokenType.RBRACE), entry('[', TokenType.LSQUARE),
        entry(']', TokenType.RSQUARE), entry('(', TokenType.LPARENT),
        entry(')', TokenType.RPARENT), entry(';', TokenType.SEMICOLON),
        entry(',', TokenType.COMMA), entry('.', TokenType.DOT)
    );

    /**
     * 构造函数，打开文件，并初始化文件读取器和 Token 列表
     * @param filename 文件名
     * @throws FileNotFoundException 文件不存在时抛出
     */
    public Lexer(String filename) throws FileNotFoundException {
        file = new PushbackReader(new FileReader(filename));
        tokens = new ArrayList<>();
    }

    /**
     * 获取 Token 列表
     * @return 不可变 Token 列表
     */
    public List<Token> getTokens() {
        return tokens;
    }

    /**
     * 扫描代码，生成 Token 列表
     * @throws IOException 读取文件异常时抛出
     */
    public void scan() throws IOException {
        int c;
        while ((c = file.read()) != -1) {
            if (c == '\n') {
                line++;
                continue;
            } else if (c == ' ' || c == '\t' || c == '\r') {
                continue;
            }
            file.unread(c);

            if (Character.isDigit(c) || c == '.') {
                scanNumber();
            } else if (Character.isLetter(c) || c == '_') {
                scanIdent();
            } else if (c == '/') {
                scanSlash();
            } else {
                scanSymbol();
            }
        }
    }

    private int scanExpInt() throws IOException {
        StringBuilder sb = new StringBuilder();
        while (true) {
            int c = file.read();
            if (Character.isDigit(c)) {
                sb.append((char) c);
            } else if ((c == '+' || c == '-') && sb.isEmpty()) {
                sb.append((char) c);
            } else {
                file.unread(c);
                break;
            }
        }
        return Integer.parseInt(sb.toString());
    }

    private void scanNumber() throws IOException {
        int c = file.read();
        int radix = 10;  // 8, 10 or 16 for int
        boolean isFloat = false;
        boolean isEFloat = false;
        boolean isPFloat = false;
        int floatExp = 0;
        StringBuilder number = new StringBuilder();

        if (c == '.') {
            isFloat = true;
            number.append("0.");
        } else if (c == '0') {
            c = file.read();
            if (c == 'x' || c == 'X') {
                radix = 16;
            } else if (Character.isDigit(c)) {
                radix = 8;
                number.append((char) c);
            } else {
                number.append("0");
                file.unread(c);
            }
        } else {
            number.append((char) c);
        }

        while (true) {
            c = file.read();
            if ((c == 'e' || c == 'E') && radix != 16) {
                floatExp = scanExpInt();
                isEFloat = true;
                isFloat = true;
                break;
            } else if (Character.isDigit(c) || "abcdefABCDEF".indexOf(c) != -1) {
                number.append((char) c);
            } else if (c == '.') {
                number.append((char) c);
                isFloat = true;
            } else if (c == 'p' || c == 'P') {
                floatExp = scanExpInt();
                isPFloat = true;
                isFloat = true;
                break;
            } else {
                file.unread(c);
                break;
            }
        }

        if (isFloat) {
            if (isPFloat || radix == 16) {
                tokens.add(new Token(TokenType.REAL, Float.parseFloat("0X" + number + "P" + floatExp), line));
            } else if (isEFloat) {
                tokens.add(new Token(TokenType.REAL, Float.parseFloat(number + "E" + floatExp), line));
            } else {
                if (number.charAt(number.length() - 1) == '.') {
                    number.append("0");
                }
                tokens.add(new Token(TokenType.REAL, Float.parseFloat(number.toString()), line));
            }
        } else {
            tokens.add(new Token(TokenType.NUMBER, Integer.parseInt(number.toString(), radix), line));
        }
    }

    private void scanIdent() throws IOException {
        int c = file.read();
        StringBuilder ident = new StringBuilder();
        while (Character.isLetterOrDigit(c) || c == '_') {
            ident.append((char) c);
            c = file.read();
        }
        file.unread(c);
        tokens.add(new Token(keywords.getOrDefault(ident.toString(), TokenType.IDENT), ident.toString(), line));
    }

    private void scanSlash() throws IOException {
        int c = file.read();
        Panic.panicIfFalse(c == '/', "invalid character " + (char) c);
        if ((c = file.read()) == -1) {
            tokens.add(new Token(TokenType.DIV, line));
            return;
        }
        if (c == '/') {
            // Skip line comment
            while (c != '\n' && c != -1) {
                c = file.read();
            }
        } else if (c == '*') {
            // Skip block comment
            while (true) {
                if ((c = file.read()) == -1) {
                    return;
                }
                if (c == '*') {
                    if ((c = file.read()) == '/') {
                        return;
                    } else {
                        file.unread(c);
                    }
                }
            }
        } else {
            // Just a division operator
            tokens.add(new Token(TokenType.DIV, line));
            file.unread(c);
        }
    }

    private void scanSymbol() throws IOException {
        int c = file.read();
        switch (c) {
            case '|' -> {
                c = file.read();
                Panic.panicIfFalse(c == '|', "only '||' is allowed");
                tokens.add(new Token(TokenType.OR, line));
            }
            case '&' -> {
                c = file.read();
                Panic.panicIfFalse(c == '&', "only '&&' is allowed");
                tokens.add(new Token(TokenType.AND, line));
            }
            case '!' -> {
                if ((c = file.read()) == '=') {
                    tokens.add(new Token(TokenType.NEQ, line));
                } else {
                    file.unread(c);
                    tokens.add(new Token(TokenType.NOT, line));
                }
            }
            case '>' -> {
                if ((c = file.read()) == '=') {
                    tokens.add(new Token(TokenType.GE, line));
                } else {
                    file.unread(c);
                    tokens.add(new Token(TokenType.GT, line));
                }
            }
            case '<' -> {
                if ((c = file.read()) == '=') {
                    tokens.add(new Token(TokenType.LE, line));
                } else {
                    file.unread(c);
                    tokens.add(new Token(TokenType.LT, line));
                }
            }
            case '=' -> {
                if ((c = file.read()) == '=') {
                    tokens.add(new Token(TokenType.EQ, line));
                } else {
                    file.unread(c);
                    tokens.add(new Token(TokenType.ASSIGN, line));
                }
            }
            default -> tokens.add(new Token(symbols.get((char) c), line));
        }
    }
}
