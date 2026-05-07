package backend.asm;

/**
 * 应该是专供 ARM 使用的，用来获取标签的低 12 位地址，用于 ADRP 与 ADD 共同实现全局变量地址的加载
 */
public class ASMLo12 extends ASMValue {
    private final String labelName;

    public ASMLo12(String labelName) {
        this.labelName = labelName;
    }

    @Override
    public String printAsOperand() {
        return ":lo12:" + this.labelName;
    }
}
