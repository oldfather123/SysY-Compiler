package midend;

import midend.LLVMType.ArrayType;
import midend.LLVMType.PointerType;
import midend.LLVMType.Type;
import midend.instr.AllocaInstr;

import java.util.ArrayList;

public class GlobalVar extends User {
    private boolean isArray = false;
    private int size = 0;
    private boolean isConst = false;
    private int allSize = 0;
    private ArrayList<Value> init = null;

    public GlobalVar(Type type, String name, Value value) {
        super(type, "@" + name);
        init = new ArrayList<>();
        init.add(value);
    }

    public GlobalVar(Type type, String name, ArrayList<Value> values) {
        super(type, "@" + name);
//        for (Value value : values) {
//            addValue(value);
//        }
        this.init = values;
        this.size = 0;
        if (values != null) {
            this.allSize = values.size();
        }
        this.isArray = true;
    }
    public boolean isArray(){return isArray;}
    public Value getValue() {
//        if (this.isArray) {
//            return init.get(0);
//        }
//        return this.getValue(0);
        return init.get(0);
    }

    public ArrayList<Value> getValueList() {
        return this.init;
    }

    public Value getValue(int count) {
        //return getValue(count);
//        if (count == 0) {
//            return this.getValue();
//        }
        return init.get(count);
    }

    public String toString() {
        PointerType pointerType = (PointerType) this.getType();
        if (!this.isArray) {
            return this.getName() + " = dso_local global "
                    + pointerType.getElementType().toString() + " " +this.getValue(0).getName();
        } else {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.append(this.getName()).append(" = dso_local global ");
            //stringBuilder.append(pointerType.getElementType().toString());
            size = 0;
            if (isAllZero(allSize)) {
                stringBuilder.append(pointerType.getElementType().toString() + " zeroinitializer");
            } else {
//                stringBuilder.append(" [");
//                for (int count = 0; count < ; count++) {
//                    Value value = getValues(count);
//                    stringBuilder.append(value.getType().toString()).append(" ").append(value.getName());
//                    if (count != size - 1) {
//                        stringBuilder.append(", ");
//                    }
//                }
//                stringBuilder.append("]");
                size = 0;
                stringBuilder.append(initValueString(pointerType.getElementType()));
            }

            return stringBuilder.toString();
        }
    }

    public String initValueString(Type type) {
        StringBuilder stringBuilder = new StringBuilder();
//        stringBuilder.append(type);
//        stringBuilder.append(" [");
        int num = ((ArrayType) type).getNum();
        if (((ArrayType) type).getElementType().isArray()) {
            stringBuilder.append(type);
            stringBuilder.append(" [");
            for (int count = 0; count < num; count++) {
                stringBuilder.append(initValueString(((ArrayType) type).getElementType()));
                if (count != num - 1) {
                    stringBuilder.append(", ");
                }
            }
            stringBuilder.append("]");
        } else {
            if (isAllZero(num)) {
                stringBuilder.append(type + " zeroinitializer");
            } else {
                stringBuilder.append(type);
                stringBuilder.append(" [");
                for (int count = 0; count < num; count++) {
                    stringBuilder.append(this.getValue(size).getType().toString() + " " + this.getValue(size).getName());
                    size++;
                    if (count != num - 1) {
                        stringBuilder.append(", ");
                    }
                }
                stringBuilder.append("]");
            }
        }
        return stringBuilder.toString();
    }

    public boolean isAllZero(int num) {
        if (init.size() == 0) {
            return true;
        }
        boolean isFloat = false;
        if (this.getValue(0) instanceof ConstFloat) {
            isFloat = true;
        }
        for (int count = size; count < size + num; count++) {
            if (isFloat) {
                if (((ConstFloat) this.getValue(count)).getValue() != 0.0F) {
                    return false;
                }
            } else {
                if (((ConstInt) this.getValue(count)).getValue() != 0) {
                    return false;
                }
            }
        }
        return true;
    }

    public boolean isAllZero() {
        if (init.size() == 0) {
            return true;
        }
        boolean isFloat = false;
        if (this.getValue(0) instanceof ConstFloat) {
            isFloat = true;
        }
        for (int count = 0; count < allSize; count++) {
            if (isFloat) {
                if (((ConstFloat) this.getValue(count)).getValue() != 0.0F) {
                    return false;
                }
            } else {
                if (((ConstInt) this.getValue(count)).getValue() != 0) {
                    return false;
                }
            }
        }
        return true;
    }

    public void setConst() {
        this.isConst = true;
    }

    public boolean isConst() {
        return this.isConst;
    }
}