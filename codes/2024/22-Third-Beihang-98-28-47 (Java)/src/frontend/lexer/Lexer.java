package frontend.lexer;

import java.io.*;
import java.util.regex.Matcher;

import static frontend.lexer.TokenType.*;

public class Lexer {
    public static final Lexer lexer = new Lexer();
    private static final boolean[] constChar = new boolean[128];
    private static final boolean[] floatChar = new boolean[128];
    private static int lineno = 1;
    private TokenList tokenList = new TokenList();

    static {
        for (int c = '0'; c <= '9'; c++) {
            constChar[c] = true;
        }
        constChar['x'] = true;
        constChar['X'] = true;
        constChar['p'] = true;
        constChar['P'] = true;
        constChar['e'] = true;
        constChar['E'] = true;
        constChar['.'] = true;
        for (int c = 'a'; c <= 'f'; c++) {
            constChar[c] = true;
        }
        for (int c = 'A'; c <= 'F'; c++) {
            constChar[c] = true;
        }

        floatChar['x'] = true;
        floatChar['X'] = true;
        floatChar['p'] = true;
        floatChar['P'] = true;
        floatChar['e'] = true;
        floatChar['E'] = true;
    }

    private Lexer() {
    }

    public static Lexer getInstance() {
        return lexer;
    }

    private int readChar(BufferedInputStream source) throws IOException {
        return source.read();
    }

    private boolean isWhiteSpace(char c) {
        return Character.isWhitespace(c);
    }

    private boolean isEnter(char c) {
        return c == '\n';
    }

    private boolean isDigitOrDot(char c) {
        return (c <= '9' && c >= '0') || c == '.';
    }

    private boolean isLetter(char c) {
        return Character.isLowerCase(c) || Character.isUpperCase(c) || c == '_';
    }

    private void keywordDeal(String str) {
        switch (str) {
            //TODO: main与getint, printf等是否为关键字
            case "const": {
                Token tk = new Token(CONST, str);
                tokenList.append(tk);
                break;
            }
            case "int": {
                Token tk = new Token(INT, str);
                tokenList.append(tk);
                break;
            }
            case "float": {
                Token tk = new Token(FLOAT, str);
                tokenList.append(tk);
                break;
            }
            case "break": {
                Token tk = new Token(BREAK, str);
                tokenList.append(tk);
                break;
            }
            case "continue": {
                Token tk = new Token(CONTINUE, str);
                tokenList.append(tk);
                break;
            }
            case "if": {
                Token tk = new Token(IF, str);
                tokenList.append(tk);
                break;
            }
            case "else": {
                Token tk = new Token(ELSE, str);
                tokenList.append(tk);
                break;
            }
            case "while": {
                Token tk = new Token(WHILE, str);
                tokenList.append(tk);
                break;
            }
            case "return": {
                Token tk = new Token(RETURN, str);
                tokenList.append(tk);
                break;
            }
            case "void": {
                Token tk = new Token(VOID, str);
                tokenList.append(tk);
                break;
            }
            default: {
                Token tk = new Token(IDENT, str);
                tokenList.append(tk);
                break;
            }
        }
    }

    public TokenList lex(BufferedInputStream bis) throws IOException {
        int c = readChar(bis);
        while (c != -1) {
            c = ignoreWhiteSpace(bis, c);
            if (c == -1) {
                break;
            } else if (c == '/') {
                c = dealWithAnnotation(bis);
            } else if (isLetter((char) c)) {
                c = dealWithIdentifier(bis, c);
            } else if (isDigitOrDot((char) c)) {
                c = dealWithConst(bis, c);
            } else if (c == '\"') {
                c = dealWithString(bis);
            } else {
                c = dealWithOthers(bis, c);
            }
        }
        return tokenList;
    }

    public int ignoreWhiteSpace(BufferedInputStream bis, int c) throws IOException {
        while (isWhiteSpace((char) c) || isEnter((char) c)) {
            if (isEnter((char) c)) {
                lineno++;
            }
            c = readChar(bis);
        }
        return c;
    }

    private int dealWithAnnotation(BufferedInputStream bis) throws IOException {
        int c = readChar(bis);
        if (c == '/') {
            c = readChar(bis);
            while (c != -1) {
                if (isEnter((char) c)) {
                    c = readChar(bis);
                    break;
                }
                c = readChar(bis);
            }
        } else if (c == '*') {
            c = readChar(bis);
            while (c != -1) {
                if (c == '*') {
                    c = readChar(bis);
                    if (c == '/') {
                        c = readChar(bis);
                        break;
                    } else {
                        continue;
                    }
                }
                c = readChar(bis);
            }
        } else {
            Token tk = new Token(DIV, "/");
            tokenList.append(tk);
        }
        return c;
    }

    private int dealWithIdentifier(BufferedInputStream bis, int c) throws IOException {
        StringBuilder sb = new StringBuilder();
        sb.append((char) c);
        c = readChar(bis);
        if (c == -1) {
            keywordDeal(sb.toString());
            return c;
        }
        while (isLetter((char) c) || (c <= '9' && c >= '0')) {
            sb.append((char) c);
            c = readChar(bis);
            if (c == -1) {
                break;
            }
        }
        keywordDeal(sb.toString());
        return c;
    }

    private int dealWithConst(BufferedInputStream bis, int c) throws IOException {
        StringBuilder sb = new StringBuilder();
        boolean floatFlag = false;
        while (constChar[c]) {
            sb.append((char) c);
            c = readChar(bis);
            if (c == -1) {
                break;
            }
            if (floatFlag) {
                if (c == '+' || c == '-') {
                    sb.append((char) c);
                    c = readChar(bis);
                    if (c == -1) {
                        break;
                    }
                }
            }
            floatFlag = floatChar[c];
        }
        for (TokenType type : NUM_CON_LIST) {
            Matcher matcher = type.getPattern().matcher(sb);
            if (matcher.matches()) {
                Token tk = new Token(type, sb.toString());
                tokenList.append(tk);
                return c;
            }
        }
        return c;
        //TODO: 浮点数输入格式错误处理
    }

    private int dealWithString(BufferedInputStream bis) throws IOException {
        StringBuilder sb = new StringBuilder();
        sb.append('\"');
        int c = readChar(bis);
        if (c == -1) {
            return c;
        }
        if (c == '\"') {
            Token tk = new Token(STR_CON, "");
            tokenList.append(tk);
            c = readChar(bis);
            return c;
        }
        while (true) {
            if (c == '\\') {
                sb.append("\\");
            } else {
                sb.append((char) c);
            }
            c = readChar(bis);
            if (c == -1 || c == '\"') {
                break;
            }
        }
        if (c == -1) {
            return c;
        }
        sb.append('\"');
        Token tk = new Token(STR_CON, sb.toString());
        tokenList.append(tk);
        c = readChar(bis);
        return c;
    }

    private int dealWithOthers(BufferedInputStream bis, int c) throws IOException {
        switch (c) {
            case '=': {
                c = readChar(bis);
                if (c == '=') {
                    Token tk = new Token(EQ, "==");
                    tokenList.append(tk);
                    c = readChar(bis);
                } else {
                    Token tk = new Token(ASSIGN, "=");
                    tokenList.append(tk);
                }
                return c;
            }
            case '!': {
                c = readChar(bis);
                if (c == '=') {
                    Token tk = new Token(NE, "!=");
                    tokenList.append(tk);
                    c = readChar(bis);
                } else {
                    Token tk = new Token(NOT, "!");
                    tokenList.append(tk);
                }
                return c;
            }
            case '<': {
                c = readChar(bis);
                if (c == '=') {
                    Token tk = new Token(LE, "<=");
                    tokenList.append(tk);
                    c = readChar(bis);
                } else {
                    Token tk = new Token(LT, "<");
                    tokenList.append(tk);
                }
                return c;
            }
            case '>': {
                c = readChar(bis);
                if (c == '=') {
                    Token tk = new Token(GE, ">=");
                    tokenList.append(tk);
                    c = readChar(bis);
                } else {
                    Token tk = new Token(GT, ">");
                    tokenList.append(tk);
                }
                return c;
            }
            case '&': {
                c = readChar(bis);
                if (c == '&') {
                    Token tk = new Token(LAND, "&&");
                    tokenList.append(tk);
                    c = readChar(bis);
                }
                return c;
            }
            case '|': {
                c = readChar(bis);
                if (c == '|') {
                    Token tk = new Token(LOR, "||");
                    tokenList.append(tk);
                    c = readChar(bis);
                }
                return c;
            }
            case '+': {
                Token tk = new Token(ADD, "+");
                tokenList.append(tk);
                c = readChar(bis);
                return c;
            }
            case '-': {
                Token tk = new Token(SUB, "-");
                tokenList.append(tk);
                c = readChar(bis);
                return c;
            }
            case '*': {
                Token tk = new Token(MUL, "*");
                tokenList.append(tk);
                c = readChar(bis);
                return c;
            }
            case '/': {
                Token tk = new Token(DIV, "/");
                tokenList.append(tk);
                c = readChar(bis);
                return c;
            }
            case '%': {
                Token tk = new Token(MOD, "%");
                tokenList.append(tk);
                c = readChar(bis);
                return c;
            }
            case ';': {
                Token tk = new Token(SEMI, ";");
                tokenList.append(tk);
                c = readChar(bis);
                return c;
            }
            case ',': {
                Token tk = new Token(COMMA, ",");
                tokenList.append(tk);
                c = readChar(bis);
                return c;
            }
            case '(': {
                Token tk = new Token(LPARENT, "(");
                tokenList.append(tk);
                c = readChar(bis);
                return c;
            }
            case ')': {
                Token tk = new Token(RPARENT, ")", lineno);
                tokenList.append(tk);
                c = readChar(bis);
                return c;
            }
            case '[': {
                Token tk = new Token(LBRACK, "[");
                tokenList.append(tk);
                c = readChar(bis);
                return c;
            }
            case ']': {
                Token tk = new Token(RBRACK, "]");
                tokenList.append(tk);
                c = readChar(bis);
                return c;
            }
            case '{': {
                Token tk = new Token(LBRACE, "{");
                tokenList.append(tk);
                c = readChar(bis);
                return c;
            }
            case '}': {
                Token tk = new Token(RBRACE, "}");
                tokenList.append(tk);
                c = readChar(bis);
                return c;
            }
            default: {
                //TODO: 非法字符是否略过
                c = readChar(bis);
                return c;
            }
        }
    }
}
//
