package frontend.ir.objecttype;

import java.util.List;

/**
 * 类型
 */
public abstract class Type {
    public abstract String printIRType();

    @Override
    public String toString() {
        return this.printIRType();
    }

    public List<Integer> getSizeList() {
        throw new RuntimeException("不应上升到Type类调用getSizeList方法");
    }

    public abstract int getByteSize();
}
