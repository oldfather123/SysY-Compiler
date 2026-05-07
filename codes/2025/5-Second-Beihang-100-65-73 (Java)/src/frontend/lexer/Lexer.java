package frontend.lexer;

import java.io.BufferedInputStream;
import java.io.IOException;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.regex.Matcher;

import static frontend.lexer.TokenType.*;

/**
 * 词法分析器
 */
public class Lexer {
    private static final Lexer LEXER = new Lexer();
    private static final boolean[] constChar = new boolean[128];
    private static final boolean[] floatChar = new boolean[128];
    private final HashSet<Character> escapeCharacters = new HashSet<>();
    private final HashMap<String, TokenType> keywordTypeMap = new HashMap<>();
    private int lineno = 1;
    private final TokenList tokenList = new TokenList();
    
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
        
        escapeCharacters.addAll(Arrays.asList('a', 'b', 't', 'n', 'r', 'v', 'f', '\"', '\'', '\\', '0'));
        
        keywordTypeMap.put("const",     CONST);
        keywordTypeMap.put("int",       INT);
        keywordTypeMap.put("float",     FLOAT);
        keywordTypeMap.put("void",      VOID);
        keywordTypeMap.put("break",     BREAK);
        keywordTypeMap.put("continue",  CONTINUE);
        keywordTypeMap.put("if",        IF);
        keywordTypeMap.put("else",      ELSE);
        keywordTypeMap.put("while",     WHILE);
        keywordTypeMap.put("return",    RETURN);
    }
    
    public static Lexer getInstance() {
        return LEXER;
    }
    
    public TokenList analyze(BufferedInputStream iStream) throws IOException {
        if (iStream == null) {
            throw new NullPointerException();
        }
        int c = iStream.read();
        while (c != -1) {
            c = skipWhiteSpace(iStream, c);
            if (c == -1) {
                break;
            } else if (c == '/') {
                c = dealWithSlash(iStream);     // 斜杠可能代表除号，也可能是注释的开头
            } else if (Character.isLetter(c) || c == '_') {
                c = dealWithLetter(iStream, c);
            } else if (Character.isDigit(c) || c == '.') {
                c = dealWithConstInt(iStream, c);
            } else if (c == '\"') {
                c = dealWithString(iStream);
            } else {
                c = dealWithOthers(iStream, c);
            }
        }
        return tokenList;
    }
    
    private int dealWithOthers(BufferedInputStream iStream, int c) throws IOException {
        switch (c) {
            case '=': {
                c = iStream.read();
                if (c == '=') {
                    Token tk = new Token(EQ, "==", lineno);
                    tokenList.append(tk);
                    c = iStream.read();
                } else {
                    Token tk = new Token(ASSIGN, "=", lineno);
                    tokenList.append(tk);
                }
                return c;
            }
            case '!': {
                c = iStream.read();
                if (c == '=') {
                    Token tk = new Token(NE, "!=", lineno);
                    tokenList.append(tk);
                    c = iStream.read();
                } else {
                    Token tk = new Token(NOT, "!", lineno);
                    tokenList.append(tk);
                }
                return c;
            }
            case '<': {
                c = iStream.read();
                if (c == '=') {
                    Token tk = new Token(LE, "<=", lineno);
                    tokenList.append(tk);
                    c = iStream.read();
                } else {
                    Token tk = new Token(LT, "<", lineno);
                    tokenList.append(tk);
                }
                return c;
            }
            case '>': {
                c = iStream.read();
                if (c == '=') {
                    Token tk = new Token(GE, ">=", lineno);
                    tokenList.append(tk);
                    c = iStream.read();
                } else {
                    Token tk = new Token(GT, ">", lineno);
                    tokenList.append(tk);
                }
                return c;
            }
            case '&': {
                c = iStream.read();
                if (c == '&') {
                    Token tk = new Token(LAND, "&&", lineno);
                    tokenList.append(tk);
                    c = iStream.read();
                } else {
                    throw new RuntimeException("ILLEGAL SYMBOL at line " + lineno);
                }
                return c;
            }
            case '|': {
                c = iStream.read();
                if (c == '|') {
                    Token tk = new Token(LOR, "||", lineno);
                    tokenList.append(tk);
                    c = iStream.read();
                } else {
                    throw new RuntimeException("ILLEGAL SYMBOL at line " + lineno);
                }
                return c;
            }
            case '+': {
                Token tk = new Token(ADD, "+", lineno);
                tokenList.append(tk);
                c = iStream.read();
                return c;
            }
            case '-': {
                Token tk = new Token(SUB, "-", lineno);
                tokenList.append(tk);
                c = iStream.read();
                return c;
            }
            case '*': {
                Token tk = new Token(MUL, "*", lineno);
                tokenList.append(tk);
                c = iStream.read();
                return c;
            }
            case '%': {
                Token tk = new Token(MOD, "%", lineno);
                tokenList.append(tk);
                c = iStream.read();
                return c;
            }
            case ';': {
                Token tk = new Token(SEMI, ";", lineno);
                tokenList.append(tk);
                c = iStream.read();
                return c;
            }
            case ',': {
                Token tk = new Token(COMMA, ",", lineno);
                tokenList.append(tk);
                c = iStream.read();
                return c;
            }
            case '(': {
                Token tk = new Token(LPARENT, "(", lineno);
                tokenList.append(tk);
                c = iStream.read();
                return c;
            }
            case ')': {
                Token tk = new Token(RPARENT, ")", lineno);
                tokenList.append(tk);
                c = iStream.read();
                return c;
            }
            case '[': {
                Token tk = new Token(LBRACK, "[", lineno);
                tokenList.append(tk);
                c = iStream.read();
                return c;
            }
            case ']': {
                Token tk = new Token(RBRACK, "]", lineno);
                tokenList.append(tk);
                c = iStream.read();
                return c;
            }
            case '{': {
                Token tk = new Token(LBRACE, "{", lineno);
                tokenList.append(tk);
                c = iStream.read();
                return c;
            }
            case '}': {
                Token tk = new Token(RBRACE, "}", lineno);
                tokenList.append(tk);
                c = iStream.read();
                return c;
            }
            default: {
                throw new RuntimeException("源文件中出现了未曾设想的字符： " + (char) c);
            }
        }
    }
    
    private int dealWithString(BufferedInputStream iStream) throws IOException {
        if (iStream == null) {
            throw new NullPointerException();
        }
        StringBuilder sb = new StringBuilder();
        sb.append('\"');
        int c = iStream.read();
        if (c == -1) {
            throw new RuntimeException("这咋一个双引号就完事了呢");
        }
        if (c == '\"') {
            Token tk = new Token(STR_CON, "", lineno);
            tokenList.append(tk);
            c = iStream.read();
            return c;
        }
        do {
            if (c == '\\') {
                int next = iStream.read();
                if (next == -1 || escapeCharacters.contains((char) next)) {
                    sb.append("\\").append((char) next);
                } else {
                    throw new RuntimeException("非法的转义字符");
                }
            } else {
                sb.append((char) c);
            }
            c = iStream.read();
        } while (c != -1 && c != '\"');
        if (c == -1) {
            throw new RuntimeException("未完成的字符串");
        }
        sb.append('\"');
        Token tk = new Token(STR_CON, sb.toString(), lineno);
        tokenList.append(tk);
        return iStream.read();
    }
    
    private int dealWithConstInt(BufferedInputStream iStream, int c) throws IOException {
        StringBuilder sb = new StringBuilder();
        boolean floatFlag = false;
        int ch = c;
        while (constChar[ch]) {
            sb.append((char) ch);
            ch = iStream.read();
            if (ch == -1) {
                break;
            }
            if (floatFlag) {
                if (ch == '+' || ch == '-') {
                    sb.append((char) ch);
                    ch = iStream.read();
                    if (ch == -1) {
                        break;
                    }
                }
            }
            floatFlag = floatChar[ch];
        }
        for (TokenType type : NUM_CONST_TYPES) {
            Matcher matcher = type.getPattern().matcher(sb);
            if (matcher.matches()) {
                Token tk = new Token(type, sb.toString(), lineno);
                tokenList.append(tk);
                return ch;
            }
        }
        throw new RuntimeException("你根本不是数字，你是什么？" + sb);
    }
    
    private int dealWithLetter(BufferedInputStream iStream, int c) throws IOException {
        StringBuilder sb = new StringBuilder();
        sb.append((char) c);
        int next = iStream.read();
        if (next == -1) {
            String str = sb.toString();
            tokenList.append(new Token(keywordTypeMap.getOrDefault(str, IDENT), str, lineno));
            return next;
        }
        while (Character.isLetter(next) || next == '_' || Character.isDigit(next)) {
            sb.append((char) next);
            next = iStream.read();
            if (next == -1) {
                break;
            }
        }
        String str = sb.toString();
        tokenList.append(new Token(keywordTypeMap.getOrDefault(str, IDENT), str, lineno));
        return next;
    }
    
    private int dealWithSlash(BufferedInputStream iStream) throws IOException {
        int next = iStream.read();
        if (next == '/') {
            next = iStream.read();
            while (next != -1) {
                if (next == '\n') {
                    next = iStream.read();
                    lineno++;
                    break;
                }
                next = iStream.read();
            }
        } else if (next == '*') {
            next = iStream.read();
            while (next != -1) {
                if (next == '*') {
                    next = iStream.read();
                    if (next == '/') {
                        next = iStream.read();
                        break;
                    } else {
                        continue;
                    }
                } else if (next == '\n') {
                    lineno++;
                }
                next = iStream.read();
            }
        } else {
            Token tk = new Token(DIV, "/", lineno);
            tokenList.append(tk);
        }
        return next;
    }
    
    private int skipWhiteSpace(BufferedInputStream iStream, int c) throws IOException {
        while (Character.isWhitespace(c)) {
            if (c == '\n') {
                lineno++;
            }
            c = iStream.read();
        }
        return c;
    }
}
