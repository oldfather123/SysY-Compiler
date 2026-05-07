package frontend.ir.objecttype.derived;

import frontend.ir.objecttype.Type;

public class TVector extends DerivedType {
    private final Type basicType;

    public TVector(Type basicType) {
        this.basicType = basicType;
    }

    public Type getBasicType() {
        return basicType;
    }

    @Override
    public boolean equals(Object obj) {
        if (!(obj instanceof TVector)) {
            return false;
        }
        return this.basicType.equals(((TVector) obj).basicType);
    }

    @Override
    public String printIRType() {
        return "VEC-" + this.basicType.printIRType();
    }

    @Override
    public int getByteSize() {
        return 4 * basicType.getByteSize();
    }
}
