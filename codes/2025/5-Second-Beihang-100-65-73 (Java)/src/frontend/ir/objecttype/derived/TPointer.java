package frontend.ir.objecttype.derived;

import frontend.ir.Value;
import frontend.ir.constant.PointerConst;
import frontend.ir.objecttype.Type;

import java.util.List;

public class TPointer extends DerivedType {
    private final Type referencedType;

    public TPointer(Type referencedType) {
        this.referencedType = referencedType;
    }

    public Type getReferencedType() {
        return referencedType;
    }

    @Override
    public String printIRType() {
        return this.referencedType.printIRType() + "*";
    }

    @Override
    public boolean equals(Object obj) {
        if (!(obj instanceof TPointer)) {
            return false;
        }
        return this.referencedType.equals(((TPointer) obj).referencedType);
    }

    @Override
    public List<Integer> getSizeList() {
        throw new RuntimeException("Pointer类型不会出现在数组中");
    }

    public Value getDefaultValue() {
        return PointerConst.NULL;
    }

    public int getByteSize() {
        return 8;
    }
}
