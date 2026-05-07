package utils;

public class Panic {
    public static void panic(String message) {
        // 抛出异常可以获取调用栈，方便调试
        // 对于在线评测，怎么处理异常都没有区别
        throw new RuntimeException("Panic: " + message);
    }

    public static void panicIfFalse(boolean condition, String message) {
        if (!condition) {
            panic(message);
        }
    }

    public static void panicIfFalse(boolean condition) {
        panicIfFalse(condition, "no additional message");
    }

    public static void panicIfTrue(boolean condition, String message) {
        panicIfFalse(!condition, message);
    }

    public static void panicIfTrue(boolean condition) {
        panicIfTrue(condition, "no additional message");
    }
}
