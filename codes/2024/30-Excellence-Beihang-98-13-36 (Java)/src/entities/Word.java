package entities;

import file.ReadSys;

import java.util.HashMap;
import java.util.Map;

public class Word {

    public final SemiWord semiWord;
    public final int line;

    public Word(SemiWord semiWord, int line) {
        this.semiWord = semiWord;
        this.line = line;
    }

    public Word(SemiWord semiWord) {
        this(semiWord, ReadSys.line);
    }

    public boolean is(Type type) {
        return semiWord.type == type;
    }

    public boolean isBType() {
        return is(Type.INT) || is(Type.FLOAT);
    }

    public boolean isFuncType() {
        return is(Type.VOID) || is(Type.INT) || is(Type.FLOAT);
    }

    public String getValue() {
        return semiWord.value;
    }

    public Type getType() {
        return semiWord.type;
    }

    private static final Map<String, SemiWord> WORDS = new HashMap<>();

    public static Word getWord(Type type) {
        return getWord(type, type.name);
    }

    public static Word getWord(Type type, String value) {
        if (WORDS.containsKey(type + value)) {
            return new Word(WORDS.get(type + value));
        }
        SemiWord word = new SemiWord(type, value);
        WORDS.put(type + value, word);
        return new Word(word);
    }

    public enum Type {
        CONST, INT, FLOAT, IDENT, VOID, IF, ELSE, WHILE, BREAK, CONTINUE, RETURN, INT_CONST, FLOAT_CONST, STR_CONST,
        NOT, AND, OR, PLUS, MINU, MULT, DIV, MOD, LSS, LEQ, GRE, GEQ, EQL, NEQ, ASSIGN, SEMICN, COMMA,
        LPARENT, RPARENT, LBRACK, RBRACK, LBRACE, RBRACE;

        private final String name;

        Type() {
            this.name = this.name().toLowerCase();
        }

        @Override
        public String toString() {
            return name;
        }
    }

    public record SemiWord(Type type, String value) { }

    @Override
    public String toString() {
        return "Word{" + "semiWord=" + semiWord + ", line=" + line + '}';
    }
}
