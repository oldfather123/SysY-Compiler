package frontend;

public class Token {
    private final int type;
    private final String val;
    private int lineNum = 0;

    public Token(int type, String val) {
        this.type = type;
        this.val = val;
    }

    public int getType() {
        return this.type;
    }

    public String getVal() {
        return this.val;
    }

    public int getLineNum() {
        return this.lineNum;
    }
}
