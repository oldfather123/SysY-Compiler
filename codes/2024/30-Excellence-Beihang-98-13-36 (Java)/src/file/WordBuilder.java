package file;

import entities.Word;

public class WordBuilder {

    public StringBuilder builder = new StringBuilder();

    public Word build() {
        String word = builder.toString();
        builder = new StringBuilder();
        if (word.isEmpty()) {
            return null;
        }
        return switch (word) {
            case "const" -> Word.getWord(Word.Type.CONST);
            case "int" -> Word.getWord(Word.Type.INT);
            case "float" -> Word.getWord(Word.Type.FLOAT);
            case "void" -> Word.getWord(Word.Type.VOID);
            case "if" -> Word.getWord(Word.Type.IF);
            case "else" -> Word.getWord(Word.Type.ELSE);
            case "while" -> Word.getWord(Word.Type.WHILE);
            case "break" -> Word.getWord(Word.Type.BREAK);
            case "continue" -> Word.getWord(Word.Type.CONTINUE);
            case "return" -> Word.getWord(Word.Type.RETURN);
            default -> detectWord(word);
        };
    }

    private static Word detectWord(String word) {
        if ((word.charAt(0) >= 'A' && word.charAt(0) <= 'Z') || (word.charAt(0) >= 'a' && word.charAt(0) <= 'z') || word.charAt(0) == '_') {
            return Word.getWord(Word.Type.IDENT, word);
        }
        if (word.contains(".") || word.contains("e") || word.contains("E") || word.contains("p") || word.contains("P")) {
            return Word.getWord(Word.Type.FLOAT_CONST, word);
        }
        return Word.getWord(Word.Type.INT_CONST, word);
    }

}
