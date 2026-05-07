package IR.Type;

public class PointerType extends Type{
    Type EleType;

    public PointerType(Type EleType){
        this.EleType = EleType;
    }

    @Override
    public boolean isPointerType() {
        return true;
    }

    public Type getEleType(){
        return EleType;
    }

    @Override
    public String toString(){
        return EleType.toString() + "*";
    }
}
