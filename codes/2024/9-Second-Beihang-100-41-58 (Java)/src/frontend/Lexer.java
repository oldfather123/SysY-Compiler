package frontend;

import config.Config;

import java.io.*;
import java.util.ArrayList;

public class Lexer {
    private char curToken;
    private Token bufferedToken;
    private final ArrayList<Token> preList = new ArrayList<>();
    private final BufferedReader input;
    private char bufferedChar = 0;

    public static final int HEXCON = 0, DECCON = 1, OCTCON = 2, DECFLOATCON = 3, HEXFLOATCON = 4, IDENTF = 5,
            CONST = 6, INT = 7, FLOAT = 8, VOID = 9, IF = 10, ELSE = 11, WHILE = 12, BREAK = 13,
            CONTINUE = 14, RETURN = 15, FMTSTR = 16, DIV = 17, ADD = 18, SUB = 19, MULT = 20, MOD = 21,
            NON = 22, LPARENT = 23, RPARENT = 24, LBRACKET = 25, RBRACKET = 26, LBRACE = 27, RBRACE = 28,
            AND = 29, OR = 30, COMMA = 31, SEMICOLON = 32, LESS = 33, LE = 34, MORE = 35, ME = 36, EQUAL = 37, NE = 38, ASSIGN = 39;

    public Lexer() throws FileNotFoundException {
        input = new BufferedReader(new BufferedReader(new FileReader(Config.inputFile)));
    }

    public char next() throws IOException {
        int temp = input.read();
        return (char) temp;
    }

    public boolean hasNext() throws IOException {
        if (bufferedChar != 0 || !preList.isEmpty()) {
            return true; // 如果bufferedChar已经被填充，那么还有字符可以读取
        }
        int temp = input.read();
        if (temp == -1) {
            return false; // 如果read返回-1，表示到达文件末尾
        }
        bufferedChar = (char) temp;
        return true;
    }

    public Token get(int count) throws IOException {
        if (count == 0) {
            if (!preList.isEmpty()) {
                return preList.get(0);
            }
            if (bufferedToken == null) {
                bufferedToken = readTok();
            }
            return bufferedToken;
        } else {
            if (!preList.isEmpty() && preList.size() > count) {
                return preList.get(count);
            }
            ArrayList<Token> tokens = new ArrayList<>();
            if (bufferedToken != null) {
                tokens.add(bufferedToken);
                bufferedToken = null;
                for (int count0 = 1; count0 <= count - preList.size(); count0++) {
                    tokens.add(after());
                }
            } else {
                for (int count0 = 0; count0 <= count - preList.size(); count0++) {
                    tokens.add(after());
                }
            }
            preList.addAll(tokens);
            return preList.get(count);
        }
    }

    public Token after() throws IOException {
        if (!preList.isEmpty()) {
            return preList.remove(0);
        }
        if (bufferedToken == null) {
            bufferedToken = readTok();
        }
        Token temp = bufferedToken;
        bufferedToken = readTok();
        return temp;
    }

    public Token readTok() throws IOException {
        while (true) {
            int temp;
            if (bufferedChar != 0) {
                temp = bufferedChar;
                bufferedChar = 0;
            } else {
                temp = input.read();
                if (temp == -1) {
                    return null;
                }
            }
            curToken = (char) temp;
            //换行符
            if (curToken == '\n' || curToken == ' ' || curToken == '\t' || curToken == '\r') {
//                if (curToken == '\n') {
//                    lineNum++;
//                }
                continue;
            }
            //数字
            if (isDigit(curToken) || curToken == '.') {
                return number();
            }
            //标识符
            if (isNonDidit(curToken)) {
                return ident();
            }
            //字符串
            if (curToken == '\"') {
                return formatString();
            }
            //其他
            switch (curToken) {
                case '/':
                    char next = next();
                    if (next == '/' || next == '*') {
                        bufferedChar = next;
                        note();
//                        if (curToken == -1) {
//                            return null;
//                        }
                        continue;
                    }
                    bufferedChar = next;
                    return new Token(DIV, "/");
                case '+':
                    return new Token(ADD, "+");
                case '-':
                    return new Token(SUB, "-");
                case '*':
                    return new Token(MULT, "*");
                case '%':
                    return new Token(MOD, "%");
                case '!':
                    char next1 = next();
                    if (next1 == '=') {
                        return new Token(NE, "!=");
                    }
                    bufferedChar = next1;
                    return new Token(NON, "!");
                case '{':
                    return new Token(LBRACE, "{");
                case '}':
                    return new Token(RBRACE, "}");
                case '[':
                    return new Token(LBRACKET, "[");
                case ']':
                    return new Token(RBRACKET, "]");
                case '(':
                    return new Token(LPARENT, "(");
                case ')':
                    return new Token(RPARENT, ")");
                case '&':
                    char next2 = next();
                    if (next2 == '&') {
                        return new Token(AND, "&&");
                    } else {
                        throw new IOException();
                    }
                case '|':
                    char next3 = next();
                    if (next3 == '|') {
                        return new Token(OR, "||");
                    } else {
                        throw new IOException();
                    }
                case ',':
                    return new Token(COMMA, ",");
                case ';':
                    return new Token(SEMICOLON, ";");
                case '<':
                    char next4 = next();
                    if (next4 == '=') {
                        return new Token(LE, "<=");
                    }
                    bufferedChar = next4;
                    return new Token(LESS, "<");
                case '>':
                    char next5 = next();
                    if (next5 == '=') {
                        return new Token(ME, ">=");
                    }
                    bufferedChar = next5;
                    return new Token(MORE, ">");
                case '=':
                    char next6 = next();
                    if (next6 == '=') {
                        return new Token(EQUAL, "==");
                    }
                    bufferedChar = next6;
                    return new Token(ASSIGN, "=");
                default:
                    return null;
            }
        }
    }

    private boolean isDigit(char temp) {
        return temp >= '0' && temp <= '9';
    }

    private boolean isNonDidit(char temp) {
        return (temp >= 'a' && temp <= 'z') || (temp >= 'A' && temp <= 'Z') || temp == '_';
    }

    private Token number() throws IOException {
        StringBuilder stringBuilder = new StringBuilder();
        boolean isOct = false;
        boolean isHex = false;
        boolean isFloat = false;
        if (curToken == '0') {
            isOct = true;
        }
        while (isDigit(curToken) || curToken == 'x' || curToken == 'X'
                || (curToken >= 'a' && curToken <= 'f') || (curToken >= 'A' && curToken <= 'F')
                || curToken == '.' || curToken == '+' || curToken == '-'
                || curToken == 'p' || curToken == 'P') {
            if (curToken == 'x' || curToken == 'X') {
                isHex = true;
                isOct = false;
            } else if (curToken == '.') {
                isFloat = true;
            } else if (!isHex && curToken == 'e' || curToken == 'E') {
                isFloat = true;
            } else if (curToken == 'p' || curToken == 'P') {
                isFloat = true;
                isHex = true;
            } else if (curToken == '+' || curToken == '-') {
                char lastword = stringBuilder.charAt(stringBuilder.length() - 1);
                if (lastword == 'e' || lastword == 'E' || lastword == 'p' || lastword == 'P') {

                } else if (!isFloat && lastword == 'e' || lastword == 'E') {

                } else {
                    break;
                }
            }
            stringBuilder.append(curToken);
            curToken = next();
        }
        String num = stringBuilder.toString();
        bufferedChar = curToken;
        if (isHex) {
            if (isFloat) {
                return new Token(HEXFLOATCON, num);
            } else {
                return new Token(HEXCON, num);
            }
        } else if (isFloat) {
            return new Token(DECFLOATCON, num);
        } else if (isOct) {
            return new Token(OCTCON, num);
        } else {
            return new Token(DECCON, num);
        }
    }

    private Token ident() throws IOException {
        StringBuilder stringBuilder = new StringBuilder();
        if (isNonDidit(curToken)) {
            stringBuilder.append(curToken);
            curToken = next();
        }
        while (isNonDidit(curToken) || isDigit(curToken)) {
            stringBuilder.append(curToken);
            curToken = next();
        }
        bufferedChar = curToken;
        String ident = stringBuilder.toString();
        return switch (ident) {
            case "const" -> new Token(CONST, ident);
            case "int" -> new Token(INT, ident);
            case "float" -> new Token(FLOAT, ident);
            case "void" -> new Token(VOID, ident);
            case "if" -> new Token(IF, ident);
            case "else" -> new Token(ELSE, ident);
            case "while" -> new Token(WHILE, ident);
            case "break" -> new Token(BREAK, ident);
            case "continue" -> new Token(CONTINUE, ident);
            case "return" -> new Token(RETURN, ident);
            default -> new Token(IDENTF, ident);
        };
    }

    public Token formatString() throws IOException {
        StringBuilder stringBuilder = new StringBuilder();
        stringBuilder.append('\"');
        curToken = next();
        while (curToken != '"') {
            stringBuilder.append(curToken);
            curToken = next();
        }
        stringBuilder.append('\"');
        String str = stringBuilder.toString();
        return new Token(FMTSTR, str);
    }

    private void note() throws IOException {
        char type = bufferedChar;
        bufferedChar = 0;
        curToken = next();
        if (type == '/') {
            while (curToken != '\n' && curToken != 0xffff) {
                curToken = next();
            }
//            if (curToken == '\n') {
//                lineNum++;
//            }
        } else if (type == '*') {
            while (true) {
                if (curToken == '*') {
                    char next = next();
                    if (next == '/' || next == 0xffff) {
                        break;
                    }
                }
//                if (curToken == '\n') {
//                    lineNum++;
//                }
//                if (curToken == -1) {
//                    break;
//                }
                curToken = next();
            }
        }
    }
}
