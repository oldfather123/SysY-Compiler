package frontend.lexer;

import utils.Panic;

import java.util.List;

/**
 * token 列表的封装，提供方便的提取、检查、处理 token 的方法
 */
public class TokenStream {
    private final List<Token> tokens;
    private int iterator;

    /**
     * 初始化队列以及遍历指针
     * @param tokens token 列表
     */
    public TokenStream(List<Token> tokens) {
        this.tokens = tokens;
        this.iterator = 0;
    }

    /**
     * 获取当前 token，遍历指针后移
     * @return 得到的当前 token
     */
    public Token next() {
        iterator += 1;
        return tokens.get(iterator - 1);
    }

    /**
     * 获取下一个 token，遍历指针后移，检查类型是否符合
     *
     * @param type 期望的 token 类型
     * @throws RuntimeException 类型不符合预期时抛出异常
     */
    public void consume(TokenType type) {
        iterator += 1;
        Token token = tokens.get(iterator - 1);
        Panic.panicIfFalse(token.getType() == type, "Invalid token: expected: " + type + " got: " + token.getType());
    }

    /**
     * 检查当前 token 是否在预期类型列表之中
     * @param types 预期的 token 类型列表
     * @return 检查结果
     */
    public boolean check(TokenType... types) {
        return peek().in(types);
    }

    /**
     * 检查当前 token 是否在预期类型列表之中，若是则遍历指针后移
     * @param types 预期的 token 类型列表
     * @return 检查结果
     */
    public boolean checkConsume(TokenType... types) {
        if (peek().in(types)) {
            iterator += 1;
            return true;
        }
        return false;
    }

    /**
     * 获取当前 token，但不移动遍历指针
     * @return 当前 token
     */
    public Token peek() {
        return peek(0);
    }

    /**
     * 获取当前 token 后第 i 个 token，但不移动遍历指针，当前 token 为 `i = 0`，
     * 若 i 超出 token 列表范围，则返回 EOF
     * @param i 向后查看的 token 个数
     * @return 当前 token 后第 i 个 token
     */
    public Token peek(int i) {
        if (iterator + i >= tokens.size()) {
            return new Token(TokenType.EOF, -1);
        }
        return tokens.get(iterator + i);
    }

    /**
     * 判断是否还有不是 EOF 的token（包含当前 token）
     * @return 是否还有 token
     */
    public boolean hasNext() {
        return iterator < tokens.size() && !check(TokenType.EOF);
    }
}
