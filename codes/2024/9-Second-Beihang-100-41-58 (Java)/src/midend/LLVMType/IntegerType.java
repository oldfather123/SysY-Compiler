package midend.LLVMType;

public class IntegerType extends Type{
    private int bits;

    private IntegerType(int bitNum) {
        super(Name.INT);
        bits = bitNum;
    }

    public static IntegerType i1 = new IntegerType(1);

    public static IntegerType i32 = new IntegerType(32);

    public String toString() {
        return "i" + bits;
    }

  }
