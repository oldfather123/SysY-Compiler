package ir.pass.opt;

import ir.value.Module;

public class ArithmeticOpt implements Pass{
    /*TODO 目前暂定几种优化情况
        1.加减乘0
        2.乘除1
        3.被除数和除数关系,判断是否恒为0
        4.连加换乘(需考虑乘法的平均周期代价)
        5.如果可能有特殊的点我可以尝试自动公式推导,至少做个求等差和等比的
    */
    /*TODO 做个鸟蛋 膜拜鹿VN
    */
    @Override
    public void run(Module module) {

    }
}
