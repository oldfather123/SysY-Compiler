package ir.type;

//void 类型
public class VoidType extends Type{
    public VoidType(){
        super(TypeName.V);
    }

    @Override
    public String toString() {return "void";}

    @Override
    public int getSpace() {return 0;}

    @Override
    public int getOffset() {return 0;}
}
