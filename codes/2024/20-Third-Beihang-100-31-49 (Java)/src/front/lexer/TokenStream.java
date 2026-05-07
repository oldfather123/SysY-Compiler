package front.lexer;

import java.util.ArrayList;

public class TokenStream {

    private final ArrayList<Token> tokenList;
    private int nextPos;
    private int sentry;

    public TokenStream(ArrayList<Token> tokenList) {
        this.tokenList = tokenList;
        nextPos = 0;
        sentry = 0;
    }

    public Token read() {
        if (nextPos >= tokenList.size()) {
            return null;
        }
        return tokenList.get(nextPos++);
    }

    public Token lookForward() {
        int tarPos = nextPos;
        if (tarPos >= tokenList.size()) {
            return null;
        }
        return tokenList.get(tarPos);
    }

    public Token forwardTwice() {
        int tarPos = nextPos + 1;
        if (tarPos >= tokenList.size()) {
            return null;
        }
        return tokenList.get(tarPos);
    }

    public void sentryStand() {
        sentry = nextPos - 1;
    }

    public Token backToSentry() {
        nextPos = sentry + 1;
        return tokenList.get(sentry);
    }

    public ArrayList<String> debug() {
        ArrayList<String> tokens = new ArrayList<>();
        for (Token token : tokenList) {
            tokens.add(token.toString());
        }
        return tokens;
    }

}
