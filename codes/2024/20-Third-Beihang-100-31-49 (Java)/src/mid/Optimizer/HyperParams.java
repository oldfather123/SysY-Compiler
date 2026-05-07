package mid.Optimizer;

public class HyperParams {
    // 将乘法转为移位时，最高能接受将包含MSN位1的常数进行拆分
    public static int MULT_SPLIT_NUM = 2;
    // 函数内联的单个函数最高指令数目
    public static int INSTR_MAX_NUM = 6000;
    // 循环展开的最高层数
    public static int UNROLL_MAX_NUM = 105;
    // unroll_and_jam的部分展开长度
    public static int UNROLL_JAM_N = 4;
    // 单个函数能够进行循环优化的最大基本块数目
    public static int MAX_BLK_NUM_OPT = 400;

    public static int TIMEOUT = 120;
}
