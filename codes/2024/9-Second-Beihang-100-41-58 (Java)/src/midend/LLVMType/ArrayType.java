package midend.LLVMType;

import java.util.ArrayList;

public class ArrayType extends Type{

    private Type elementType;
    private int num;

    public ArrayType(Type type, int i) {
        super(Name.ARRAY);
        elementType = type;
        num = i;
    }

    public int getNum() {
        return num;
    }

    public Type getElementType() {
        return elementType;
    }

    public int getSize() {
        if (!elementType.isArray()) {
            return num;
        }
        else {
            return ((ArrayType) elementType).getSize() * num;
        }
    }

    public int getPos() {
        if (!elementType.isArray()) {
            return 1;
        }
        else {
            return ((ArrayType) elementType).getPos() + 1;
        }
    }

    public boolean isIntegerArray() {
        if (!elementType.isArray()) {
            return elementType.isInteger();
        }
        else {
            return ((ArrayType) elementType).isIntegerArray();
        }
    }

    public ArrayList<Integer> getNumList() {
        ArrayList<Integer> integerArrayList = new ArrayList<>();
        integerArrayList.add(num);
        if (elementType.isArray()) {
            integerArrayList.addAll(((ArrayType) elementType).getNumList());
        }
        return integerArrayList;
    }

    public String toString() {
        return "[" + num + " x " + elementType.toString() + "]";
    }
}
