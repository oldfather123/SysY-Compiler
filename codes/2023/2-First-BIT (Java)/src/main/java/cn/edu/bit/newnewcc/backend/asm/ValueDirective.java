package cn.edu.bit.newnewcc.backend.asm;

/**
 * 用于标注数据格式，WORD时value存储的为数据值，ZERO则存储数据长度（字节数）
 */
public class ValueDirective {
    private final Type type;
    private final int value;
    private final long length;
    private final long lvalue;

    private ValueDirective(Type type, int value, long length, long lvalue) {
        this.type = type;
        this.value = value;
        this.length = length;
        this.lvalue = lvalue;
    }

    public static ValueDirective getZeroValue(long length) {
        return new ValueDirective(Type.ZERO, 0, length, 0);
    }

    public ValueDirective(int value) {
        this.type = Type.WORD;
        this.length = 0;
        this.value = value;
        this.lvalue = 0;
    }

    public ValueDirective(float value) {
        this.type = Type.WORD;
        this.length = 0;
        this.value = Float.floatToIntBits(value);
        this.lvalue = 0;
    }

    public ValueDirective(long lvalue) {
        this.type = Type.DWORD;
        this.length = 0;
        this.value = 0;
        this.lvalue = lvalue;
    }

    public String emit() {
        if (type == Type.WORD) {
            return String.format(".word %d\n", value);
        } else if (type == Type.ZERO) {
            return String.format(".zero %d\n", length);
        } else {
            return String.format(".dword %d\n", lvalue);
        }
    }

    public enum Type {
        WORD, ZERO, DWORD
    }
}
