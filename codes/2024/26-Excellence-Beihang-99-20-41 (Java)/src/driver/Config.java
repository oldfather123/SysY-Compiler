package driver;

/**
 * Config class
 */
public class Config {
    /**
     * 输入文件，拓展名为 '.sy'
     */
    public static String source = "test/testgvn.sy";

    /**
     * 输出汇编文件，拓展名为 '.S'
     */
    public static String output = "testcase.S";

    /**
     * 是否打印 AST 与 符号表
     */
    public static boolean emitAst = false;

    /**
     * 输出 IR 的文件名，null 表示不输出 IR，<code>&lt;stdout&gt;</code> 表示输出到控制台
     */
    public static String emitIr = null;
//    public static String emitRISCV = "./test/riscv/output";
    public static String emitRISCV = null;

    /**
     * 是否在每一个 Pass 后，都生成一次 IR，文件名为 `{emitIr}.%02d.{passId}.ll`
     */
    public static boolean emitIrEachTime = false;

    /**
     * 是否开启调试模式
     */
    public static boolean debug = false;

    /**
     * 输出 Block 的 Dot 文件，null 表示不输出
     */
    public static String emitDotBlock = null;

    /**
     * 输出 去除phi指令后 的文件名，null 表示不输出，<code>&lt;stdout&gt;</code> 表示输出到控制台
     */
    public static String emitWithoutPhi = null;

    /**
     * 优化级别
     */
    public static int optLevel = 0;

    public static class DebugSettings {
        public static boolean displayLoopInfo = false;
        public static boolean displayDetailedLoopHoist = false;
        public static boolean displayInductionVar = false;
        public static boolean displayFunctionInline = false;
    }
}
