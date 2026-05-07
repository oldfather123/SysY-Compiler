package cn.edu.bit.newnewcc.frontend;

public class Preprocessor {
    private Preprocessor() {
    }

    public static String preprocess(String code) {
        String[] lines = code.split("\\r?\\n");

        for (int i = 0; i < lines.length; ++i) {
            lines[i] = lines[i].replaceAll(
                "(?<=[^A-Za-z_]|^)starttime\\s*\\(\\s*\\)",
                "_sysy_starttime(" + (i + 1) + ")"
            );
            lines[i] = lines[i].replaceAll(
                "(?<=[^A-Za-z_]|^)stoptime\\s*\\(\\s*\\)",
                "_sysy_stoptime(" + (i + 1) + ")"
            );
        }

        return String.join("\n", lines);
    }
}
