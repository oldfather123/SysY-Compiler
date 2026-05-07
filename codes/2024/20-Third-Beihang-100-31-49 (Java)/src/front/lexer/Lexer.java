package front.lexer;

import java.io.IOException;
import java.io.PushbackReader;
import java.util.ArrayList;

import utils.type.TokenType;

public class Lexer {

    private final PushbackReader inputStream;
    private final ArrayList<Token> tokenLst;
    private char curChar;
    private int line;

    public Lexer(PushbackReader inputStream) {
        this.inputStream = inputStream;
        tokenLst = new ArrayList<>();
        read();
        line = 1;
    }

    public void read() {
        try {
            curChar = (char) inputStream.read();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public TokenStream getTokenStream() throws IOException {
        run();
        return new TokenStream(tokenLst);
    }

    public void run() throws IOException {
        Token token = getToken();
        while (token.getTokenType() != TokenType.EOF) {
            tokenLst.add(token);
            token = getToken();
        }
    }

    public Token getToken() throws IOException {
        //  \n \t \r ' '
        while (curChar == ' ' || curChar == '\t' || curChar == '\r' || curChar == '\n') {
            if (curChar == '\n') {
                line++;
            }
            read();
        }
        //  EOF
        if (curChar == '\uFFFF') {
            return new Token(TokenType.EOF, "EOF", line);
        }
        //  + - *  % ; , ( ) { } [ ]
        if (curChar == '+') {
            read();
            return new Token(TokenType.PLUS, "+", line);
        }
        if (curChar == '-') {
            read();
            return new Token(TokenType.MINU, "-", line);
        }
        if (curChar == '*') {
            read();
            return new Token(TokenType.MULT, "*", line);
        }
        if (curChar == '%') {
            read();
            return new Token(TokenType.MOD, "%", line);
        }
        if (curChar == ';') {
            read();
            return new Token(TokenType.SEMICN, ";", line);
        }
        if (curChar == ',') {
            read();
            return new Token(TokenType.COMMA, ",", line);
        }
        if (curChar == '(') {
            read();
            return new Token(TokenType.LPARENT, "(", line);
        }
        if (curChar == ')') {
            read();
            return new Token(TokenType.RPARENT, ")", line);
        }
        if (curChar == '[') {
            read();
            return new Token(TokenType.LBRACK, "[", line);
        }
        if (curChar == ']') {
            read();
            return new Token(TokenType.RBRACK, "]", line);
        }
        if (curChar == '{') {
            read();
            return new Token(TokenType.LBRACE, "{", line);
        }
        if (curChar == '}') {
            read();
            return new Token(TokenType.RBRACE, "}", line);
        }
        //  && ||
        if (curChar == '&') {
            read();
            read();
            return new Token(TokenType.AND, "&&", line);
        }
        if (curChar == '|') {
            read();
            read();
            return new Token(TokenType.OR, "||", line);
        }

        //  != <= < > >= ==
        if (curChar == '!') {
            read();
            if (curChar == '=') {
                read();
                return new Token(TokenType.NEQ, "!=", line);
            } else {
                return new Token(TokenType.NOT, "!", line);
            }
        }
        if (curChar == '<') {
            read();
            if (curChar == '=') {
                read();
                return new Token(TokenType.LEQ, "<=", line);
            } else {
                return new Token(TokenType.LSS, "<", line);
            }
        }
        if (curChar == '>') {
            read();
            if (curChar == '=') {
                read();
                return new Token(TokenType.GEQ, ">=", line);
            } else {
                return new Token(TokenType.GRE, ">", line);
            }
        }
        if (curChar == '=') {
            read();
            if (curChar == '=') {
                read();
                return new Token(TokenType.EQL, "==", line);
            } else {
                return new Token(TokenType.ASSIGN, "=", line);
            }
        }

        //  " formatString "
        if (curChar == '"') {
            StringBuilder formatString = new StringBuilder("\"");
            read();
            while (curChar != '"') {
                formatString.append(curChar);
                read();
            }
            formatString.append("\"");
            read();
            return new Token(TokenType.STRCON, formatString.toString(), line);
        }

        //   //   /* */   /
        if (curChar == '/') {
            read();
            if (curChar == '/') {
                read();
                while (true) {
                    if (curChar == '\n') {
                        line++;
                        break;
                    }
                    if (curChar == '\uFFFF') {
                        return new Token(TokenType.EOF, "EOF", line);
                    }
                    read();
                }
                read();
                return getToken();
            } else if (curChar == '*') {
                read();
                boolean hasStar = false;
                while (true) {
                    if (curChar == '\n') {
                        line++;
                    } else if (curChar == '/' && hasStar) {
                        break;
                    }
                    hasStar = curChar == '*';
                    read();
                }
                read();
                return getToken();
            } else {
                return new Token(TokenType.DIV, "/", line);
            }
        }

        // Number
        if (isNumber(curChar) || curChar == '.') {
            boolean isFloat = false;
            boolean isHex = false;
            boolean isOct = curChar == '0';
            StringBuilder numSB = new StringBuilder();
            while (isNumber(curChar) || curChar == 'x' || curChar == 'X' || curChar == '+' || curChar == '-'
                    || curChar == 'p' || curChar == 'P' || curChar == '.' ||
                    (curChar >= 'a' && curChar <= 'f') || (curChar >= 'A' && curChar <= 'F')) {
                if (curChar == '.') {
                    isFloat = true;
                    isOct = false;
                } else if (curChar == 'X' || curChar == 'x') {
                    isHex = true;
                    isOct = false;
                } else if (curChar == 'p' || curChar == 'P') {
                    isFloat = true;
                    isHex = true;
                } else if ((curChar == 'e' || curChar == 'E') && !isHex) {
                    isFloat = true;
                } else if (curChar == '+' || curChar == '-') {
                    String nowStr = numSB.toString();
                    char last = nowStr.charAt(nowStr.length() - 1);
                    if (last != 'p' && last != 'P' && last != 'e' && last != 'E') {
                        break;
                    }
                    if ((last == 'e' || last == 'E') && !isFloat) {
                        break;
                    }
                }
                numSB.append(curChar);
                read();
            }
            String num = numSB.toString();
            if (num.equals("0")) {
                isOct = false;
            }
            if (isFloat && isHex) {
                return new Token(TokenType.HEXFCON, num, line);
            } else if (isFloat) {
                return new Token(TokenType.DECFCON, num, line);
            } else if (isHex) {
                return new Token(TokenType.HEXCON, num, line);
            } else if (isOct) {
                return new Token(TokenType.OCTCON, num, line);
            } else {
                return new Token(TokenType.DECCON, num, line);
            }
        }

        // reserved word or Ident
        StringBuilder token = new StringBuilder();
        while (Character.isUpperCase(curChar)
                || Character.isLowerCase(curChar)
                || Character.isDigit(curChar)
                || curChar == '_') {
            token.append(curChar);
            read();
        }
        return new Token(getTokenType(token.toString()), token.toString(), line);
    }

    public TokenType getTokenType(String str) {
        return switch (str) {
            case "const" -> TokenType.CONSTTK;
            case "int" -> TokenType.INTTK;
            case "float" -> TokenType.FLOATTK;
            case "while" -> TokenType.WHILETK;
            case "break" -> TokenType.BREAKTK;
            case "continue" -> TokenType.CONTINUETK;
            case "if" -> TokenType.IFTK;
            case "else" -> TokenType.ELSETK;
            case "return" -> TokenType.RETURNTK;
            case "void" -> TokenType.VOIDTK;
            default -> TokenType.IDENFR;
        };
    }

    boolean isNumber(char x) {
        return (x <= '9' && x >= '0');
    }

}

