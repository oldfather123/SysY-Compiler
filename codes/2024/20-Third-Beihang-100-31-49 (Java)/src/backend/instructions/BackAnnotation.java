package backend.instructions;

import utils.Config;

public class BackAnnotation extends BackInstruction {
    private final String content;

    public BackAnnotation(String content) {
        this.content = content;
    }

    public String toString() {
        return "\t# " + content.substring(0, content.length() - 1) + "\n";
    }
}
