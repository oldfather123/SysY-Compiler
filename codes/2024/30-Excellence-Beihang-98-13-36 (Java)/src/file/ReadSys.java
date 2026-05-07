package file;

import entities.Word;

import java.io.FileReader;
import java.io.IOException;
import java.io.Reader;
import java.util.ArrayList;
import java.util.List;

public class ReadSys {

    public static int line = 1;
    private static final WordBuilder WORD_BUILDER = new WordBuilder();
    private static final SymbolBuilder SYMBOL_BUILDER = new SymbolBuilder();
    private static boolean readingSpacialNumber = false;

    public static List<Word> read(String path) {
        line = 1;
        try (FileReader reader = new FileReader(path)) {
            List<Word> words = new ArrayList<>();
            WORD_BUILDER.build();
            SYMBOL_BUILDER.build();
            int c = 0;
            while (c != -1) {
                c = read(reader);
                process(c, words, reader);
            }
            return words;
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    private static boolean isNotAlphaOrUnderline() {
        return (WORD_BUILDER.builder.charAt(0) < 'a' || WORD_BUILDER.builder.charAt(0) > 'z') &&
                (WORD_BUILDER.builder.charAt(0) < 'A' || WORD_BUILDER.builder.charAt(0) > 'Z') &&
                WORD_BUILDER.builder.charAt(0) != '_';
    }

    private static void process(int c, List<Word> words, FileReader reader) throws IOException {
        int buffer;
        if (c == '\"') {
            StringBuilder stringBuilder = new StringBuilder("\"");
            while ((buffer = read(reader)) != -1) {
                char c2 = (char) buffer;
                stringBuilder.append(c2);
                if (c2 == '\"') {
                    words.add(Word.getWord(Word.Type.STR_CONST, stringBuilder.toString()));
                    break;
                }
            }
            return;
        }
        if ((c == 'e' || c == 'E' || c == 'p' || c == 'P')
                && !WORD_BUILDER.builder.isEmpty() && isNotAlphaOrUnderline()) {
            readingSpacialNumber = true;
        }
        if ((c == '+' || c == '-') && !WORD_BUILDER.builder.isEmpty() && isNotAlphaOrUnderline() && readingSpacialNumber) {
            WORD_BUILDER.builder.append((char) c);
        } else if ((c <= '9' && c >= '0') || (c <= 'z' && c >= 'a') || (c <= 'Z' && c >= 'A') || c == '_' || c == '.') {
            WORD_BUILDER.builder.append((char) c);
        } else {
            readingSpacialNumber = false;
            Word word = WORD_BUILDER.build();
            if (word != null) {
                words.add(word);
            }
        }
        if ((c == '+' || c == '-' || c == '%' || c == ';' || c == ',' || c == '*'
                || c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}')
                && !readingSpacialNumber) {
            Word word = SYMBOL_BUILDER.build();
            if (word != null) {
                words.add(word);
            }
            Word word2 = SYMBOL_BUILDER.specialBuild(c);
            if (word2 != null) {
                words.add(word2);
            }
        } else if (c == '/') {
            buffer = read(reader);
            char c2 = (char) buffer;
            if (c2 == '*') {
                int state = 0;
                while ((buffer = read(reader)) != -1) {
                    char c3 = (char) buffer;
                    if (c3 == '*') {
                        state = 1;
                    } else if (c3 == '/' && state == 1) {
                        return;
                    } else {
                        state = 0;
                    }
                }
            } else if (c2 == '/') {
                while ((buffer = read(reader)) != -1) {
                    char c3 = (char) buffer;
                    if (c3 == '\n') {
                        return;
                    }
                }
            } else {
                words.add(Word.getWord(Word.Type.DIV));
                process(c2, words, reader);
            }
        } else if (c == '!' || c == '&' || c == '|' || c == '>' || c == '<' || c == '=') {
            Word word = SYMBOL_BUILDER.addChar(c);
            if (word != null) {
                words.add(word);
            }
        } else {
            Word word = SYMBOL_BUILDER.build();
            if (word != null) {
                words.add(word);
            }
        }
    }

    public static int read(Reader reader) throws IOException {
        int result = reader.read();
        if (result == '\n') {
            line++;
        }
        return result;
    }
}
