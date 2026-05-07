package IR;

public class IRConst {
    public IRConst(){
    }
    public static final int IRConstantVoidValueKind = 0;
    public static final int IRConstantInt1ValueKind = 1;
    public static final int IRConstantInt32ValueKind = 2;
    public static final int IRConstantFloatValueKind = 3;
    public static final int IRConstantArrayValueKind = 4;
    public static final int IRConstantPointerValueKind = 5;
    public static final int IRConstantFunctionValueKind = 6;
    public static final int IRBasicBlockValueKind = 7;
    public static final int IRInstructionValueKind = 8;
    public static final int IRGlobalVariableValueKind = 9;


    public static final int IntToFloat = 0;
    public static final int FloatToInt = 1;

    public static final String[] compareTypes = {
            "eq", "ne", "sgt", "sge", "slt", "sle"
    };
    public static final String[] floatCompareTypes = {
            "ueq","une", "ugt", "uge", "ult", "ule"
    };
    public static final int IREQ = 0;
    public static final int IRNE = 1;
    public static final int IRSGT = 2;
    public static final int IRSGE = 3;
    public static final int IRSLT = 4;
    public static final int IRSLE = 5;
}
