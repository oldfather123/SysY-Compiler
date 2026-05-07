package cn.edu.bit.newnewcc.backend.asm.operand;

import java.util.Objects;

/**
 * 全局标记实际上存储的是地址，通常代表全局变量或浮点变量
 * 读取地址的时候使用%hi(label), %lo(label)两个伪指令，分别读取高16位和低16位
 * 使用地址寄存器+地址偏移量的形式读取，例如%lo(label)(a5)
 */
public class Label extends AsmOperand {
    private final String labelName;
    private final SEGMENT segment;
    private final IntRegister baseAddress;

    /**
     * 创建一个内存位置的标识符，用于读取数据
     *
     * @param labelName     标识符名字
     * @param segment     取地址的段（分为高16位与低16位）
     * @param baseAddress 取的基地址，若为null则仅返回偏移量
     */
    public Label(String labelName, SEGMENT segment, IntRegister baseAddress) {
        this.labelName = labelName;
        this.segment = segment;
        this.baseAddress = baseAddress;
    }

    /**
     * 创建一个代码位置的标识符，用于支持代码跳转
     * <p>
     * 标识符具有两类，一类是全局变量、函数名等带有globl标签的可链接的名称，
     * <p>
     * 另一类是局部的块标签、数据标签等，无需globl标记，通常在前方加'.'作为区分
     *
     * @param labelName     标识符名称
     * @param isLinkedLabel 是否是带有标记的全局类标签
     */
    public Label(String labelName, boolean isLinkedLabel) {
        if (isLinkedLabel) {
            this.labelName = labelName;
        } else {
            this.labelName = "." + labelName;
        }
        this.segment = null;
        this.baseAddress = null;
    }

    public Label(String labelName, SEGMENT segment) {
        this.labelName = labelName;
        this.segment = segment;
        this.baseAddress = null;
    }

    public String getLabelName() {
        return labelName;
    }

    public SEGMENT getSegment() {
        return segment;
    }

    public IntRegister getBaseAddress() {
        return baseAddress;
    }

    private String getOffset() {
        if (segment == SEGMENT.HIGH) {
            return String.format("%%hi(%s)", labelName);
        } else if (segment == SEGMENT.LOW) {
            return String.format("%%lo(%s)", labelName);
        } else {
            return labelName;
        }
    }

    public boolean isHighSegment() {
        return segment == SEGMENT.HIGH;
    }
    public boolean isLowSegment() {
        return segment == SEGMENT.LOW;
    }

    @Override
    public String toString() {
        return String.format("Label(%s, %s, %s)", getLabelName(), getSegment(), getBaseAddress());
    }

    @Override
    public String emit() {
        if (baseAddress == null) {
            return getOffset();
        } else {
            return getOffset() + "(" + baseAddress.emit() + ")";
        }
    }

    public enum SEGMENT {
        HIGH, LOW
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        Label label = (Label) o;

        if (!labelName.equals(label.labelName)) return false;
        if (segment != label.segment) return false;
        return Objects.equals(baseAddress, label.baseAddress);
    }

    @Override
    public int hashCode() {
        int result = labelName.hashCode();
        result = 31 * result + (segment != null ? segment.hashCode() : 0);
        result = 31 * result + (baseAddress != null ? baseAddress.hashCode() : 0);
        return result;
    }
}
