package file;

import entities.Word;

public class SymbolBuilder {

    private int state = 0;

    private static final int NOT = 11, AND = 21, AND2 = 22, OR = 31, OR2 = 32,
            LSS = 91, LEQ = 101, GRE = 111, GEQ = 121, EQL = 131, NEQ = 141, ASSIGN = 151;

    public Word addChar(int c) {
        Word word = null;
        switch (c) {
            case '!':
                word = build();
                state = NOT;
                break;
            case '&':
                if (state == AND) {
                    state = AND2;
                } else {
                    word = build();
                    state = AND;
                }
                break;
            case '|':
                if (state == OR) {
                    state = OR2;
                } else {
                    word = build();
                    state = OR;
                }
                break;
            case '<':
                word = build();
                state = LSS;
                break;
            case '>':
                word = build();
                state = GRE;
                break;
            case '=':
                switch (state) {
                    case LSS -> state = LEQ;
                    case GRE -> state = GEQ;
                    case ASSIGN -> state = EQL;
                    case NOT -> state = NEQ;
                    default -> {
                        word = build();
                        state = ASSIGN;
                    }
                }
                break;
        }
        return word;
    }

    public Word build() {
        Word word = switch (state) {
            case NOT -> Word.getWord(Word.Type.NOT);
            case AND2 -> Word.getWord(Word.Type.AND);
            case OR2 -> Word.getWord(Word.Type.OR);
            case LSS -> Word.getWord(Word.Type.LSS);
            case LEQ -> Word.getWord(Word.Type.LEQ);
            case GRE -> Word.getWord(Word.Type.GRE);
            case GEQ -> Word.getWord(Word.Type.GEQ);
            case EQL -> Word.getWord(Word.Type.EQL);
            case NEQ -> Word.getWord(Word.Type.NEQ);
            case ASSIGN -> Word.getWord(Word.Type.ASSIGN);
            default -> null;
        };
        state = 0;
        return word;
    }

    public Word specialBuild(int c) {
        return switch (c) {
            case '+' -> Word.getWord(Word.Type.PLUS);
            case '-' -> Word.getWord(Word.Type.MINU);
            case '%' -> Word.getWord(Word.Type.MOD);
            case ';' -> Word.getWord(Word.Type.SEMICN);
            case ',' -> Word.getWord(Word.Type.COMMA);
            case '*' -> Word.getWord(Word.Type.MULT);
            case '(' -> Word.getWord(Word.Type.LPARENT);
            case ')' -> Word.getWord(Word.Type.RPARENT);
            case '[' -> Word.getWord(Word.Type.LBRACK);
            case ']' -> Word.getWord(Word.Type.RBRACK);
            case '{' -> Word.getWord(Word.Type.LBRACE);
            case '}' -> Word.getWord(Word.Type.RBRACE);
            default -> null;
        };
    }

}
