package frontend.ir.objecttype;

import java.util.List;

public class TVoid extends Type {
    @Override
    public String printIRType() {
        return "void";
    }

    @Override
    public boolean equals(Object obj) {
        return obj instanceof TVoid;
    }

    @Override
    public List<Integer> getSizeList() {
        throw new RuntimeException("Void类型不会出现在数组中");
    }

    public int getByteSize() {
        return 0; // Void类型没有大小
    }
}
