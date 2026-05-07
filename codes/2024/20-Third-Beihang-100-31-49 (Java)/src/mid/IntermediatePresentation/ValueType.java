package mid.IntermediatePresentation;

import mid.IntermediatePresentation.Instruction.Call;

public class ValueType {
    public static final ValueType I32 = new ValueType("i32");
    public static final ValueType PI32 = new ValueType("i32*");

    public static final ValueType FLT = new ValueType("float");
    public static final ValueType PFLT = new ValueType("float*");
    public static final ValueType I1 = new ValueType("i1");
    public static final ValueType NULL = new ValueType("void");
    public static final ValueType ARRAY = new ValueType("array");
    public static final ValueType NPI32 = new ValueType("npi32");
    public static final ValueType NPFLT = new ValueType("npfloat");


    private final String type;
    private int len = 1;

    private boolean isInt = true;

    private int pptrLevel = 0;

    private ValueType(String type) {
        this.type = type;
        if (type.equals("i32*") || type.equals("float*")) {
            pptrLevel = 1;
        }
    }

    public ValueType(int len, boolean isInt) {
        this.type = "array";
        this.len = len;
        this.isInt = isInt;
        pptrLevel = 1;
    }

    public ValueType(boolean isInt, int pptrLevel) {
        this.type = isInt ? "npi32" : "npfloat";
        this.pptrLevel = pptrLevel;
        this.isInt = isInt;
    }

    public boolean equals(Object o) {
        if (o instanceof ValueType oType) {
            if (type.equals("array")) {
                return oType.toString().startsWith("[");
            } else {
                return type.equals(oType.toString());
            }
        } else {
            return false;
        }
    }

    public String toString() {
        if (this == ValueType.NPI32 || this == ValueType.NPFLT) {
            if (pptrLevel == 1) {
                return isInt ? "i32*" : "float*";
            }
            return pptrLevel + type;
        }
        if (!type.equals("array")) {
            return type;
        } else {
            String elemType = (isInt) ? "i32" : "float";
            return "[ " + len + " x " + elemType + " ]";
        }
    }

    public int getLength() {
        return len;
    }

    public int getPptrLevel() {
        return pptrLevel;
    }

    public String getRefTypeString() {
        return getRefType().toString();
    }

    public ValueType getRefType() {
        return switch (type) {
            case "i32*" -> ValueType.I32;
            case "float*" -> ValueType.FLT;
            case "array" -> isInt ? ValueType.I32 : ValueType.FLT;
            case "npi32" -> pptrLevel == 1 ? ValueType.I32 : new ValueType(true, pptrLevel - 1);
            case "npfloat" -> pptrLevel == 1 ? ValueType.FLT : new ValueType(false, pptrLevel - 1);
            default -> ValueType.NULL;
        };
    }

    public ValueType getPointerType() {
        return switch (type) {
            case "i32" -> ValueType.PI32;
            case "float" -> ValueType.PFLT;
            case "i32*", "npi32" -> new ValueType(true, pptrLevel + 1);
            case "float*", "npfloat" -> new ValueType(false, pptrLevel + 1);
            case "array" -> new ValueType(isInt, pptrLevel + 1);
            default -> ValueType.NULL;
        };
    }

    public boolean isPointer() {
        return pptrLevel >= 1;
    }

    public int getByteLen() {
        if (isPointer()) {
            if (equals(ValueType.ARRAY)) {
                return getRefType().getByteLen() * len;
            }
            return 8;
        } else {
            return 4;
        }
    }
}
