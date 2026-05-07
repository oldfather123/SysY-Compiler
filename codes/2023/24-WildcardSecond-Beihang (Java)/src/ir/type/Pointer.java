package ir.type;

public class Pointer extends Type{
    public Type pointTo;

    public Pointer(Type type) {
        super(type.getType());
        pointTo = type;
    }

    @Override
    public int getSpace() {
        return 0;
        // TODO: 2023/7/5
    }

    @Override
    public int getOffset() {
        return 0;
        // TODO: 2023/7/5
    }

    @Override
    public String toString() {
        return pointTo + "*";
    }

    @Override
    public boolean equals(Object obj) {
        if(obj instanceof Pointer) {
            if (this.pointTo instanceof VoidType || ((Pointer) obj).pointTo instanceof VoidType) {
                return true;
            } else {
                return pointTo.equals(((Pointer) obj).pointTo);
            }
        } else if(obj instanceof ArrayType) {
            if(pointTo instanceof VoidType) {
                return true;
            } else {
                return obj.equals(this);
            }
        } else {
            return false;
        }
    }
}
