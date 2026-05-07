package frontend.syntax.ast.nodes;

import frontend.syntax.ast.AST;
import frontend.syntax.ast.nodes.declanddef.IInitVal;

import java.util.ArrayList;
import java.util.List;

public class ASTStringConst extends AST.Node implements IInitVal {
    private final List<Integer> asciiList;
    
    public ASTStringConst(String content, int lineno) {
        super(lineno);
        this.asciiList = new ArrayList<>();
        for (int i = 1; i < content.length() - 1; i++) {    // 去掉头尾的引号
            char c = content.charAt(i);
            if (c == '\\') {
                asciiList.add(
                        switch (content.charAt(++i)) {
                            case 'a' -> 7;
                            case 'b' -> 8;
                            case 't' -> 9;
                            case 'n' -> 10;
                            case 'r' -> 13;
                            case 'v' -> 11;
                            case 'f' -> 12;
                            case '\"' -> 34;
                            case '\'' -> 39;
                            case '\\' -> 92;
                            case '0' -> 0;
                            default -> throw new RuntimeException("转义了个什么玩意这是：" + content.charAt(i - 1));
                        }
                );
            } else {
                asciiList.add((int) c);
            }
        }
    }
    
    public List<Integer> getAsciiList() {
        return this.asciiList;
    }
}
