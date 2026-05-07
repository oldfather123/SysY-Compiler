package IR.Type;

public class IntegerType extends Type{

    private final int bit;
    private IntegerType(int bit){
        this.bit = bit;
    }

    public static IntegerType I32 = new IntegerType(32);
    public static IntegerType I1 = new IntegerType(1);

    @Override
    public boolean isIntegerTy(){
        return true;
    }


    @Override
    public String toString(){
        return "i" + bit;
    }
}
