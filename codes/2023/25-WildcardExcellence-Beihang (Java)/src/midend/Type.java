package midend;

/*
IR的类型系统，包括：
一切都是Value，每个Value会有一个Type
VOID：无返回值
INTEGER：整数 i32, i1
<<<<<<< HEAD
<<<<<<< HEAD
FloatType：浮点数 float
=======
>>>>>>> d499c19 (基本中端数据结构)
=======
FloatType：浮点数 float
>>>>>>> 3869b3e (浮点数)
POINTER：指针
ARRAY：数组
 */

public class Type {

    public int getLength() {
        return 1;
    }

    public int getSize() {
        return 1;
    }

    public Type getElementType() {
        return null;
    }

    public boolean isIntegerType() {
        return this instanceof IntegerType;
    }

    public boolean isPointerType() {
        return this instanceof PointerType;
    }

    public boolean isVoidType() {
        return this instanceof VoidType;
    }

    public boolean isArrayType() {
        return this instanceof ArrayType;
    }

    public boolean isFloatType() {
        return this instanceof FloatType;
    }

    public boolean isBasicType() {
        return !(isArrayType() || isPointerType());
    }

    public static class PointerType extends Type {
        private final Type elementType;

        public PointerType(Type elementType) {
            this.elementType = elementType;
        }

        public Type getElementType() {
            return this.elementType;
        }

        public String toString() {
            return elementType.toString() + "*";
        }
    }

    public static class IntegerType extends Type {
        private final String name; // i32 / i1
        public static final IntegerType I32 = new IntegerType("i32");
        public static final IntegerType I1 = new IntegerType("i1");

        public IntegerType(String name) {
            this.name = name;
        }

        public boolean isI1() {
            return this.name.equals("i1");
        }

        public boolean isI32() {
            return this.name.equals("i32");
        }

        public String toString() {
            return this.name;
        }
    }

    public static class FloatType extends Type {
        public FloatType() {

        }

        public String toString() {
            return "float";
        }
    }

    public static class ArrayType extends Type {
        private final Type elementType;
        private final int size;

        public ArrayType(Type elementType, int size) {
            this.elementType = elementType;
            this.size = size;
        }

        public Type getElementType() {
            return this.elementType;
        }

        @Override
        public int getLength() {
            return this.size * elementType.getLength();
        }

        public int getSize() {
            return this.size;
        }

        public String toString() {
            return "[" + size + " x " + elementType.toString() + "]";
        }
    }

    public static class VoidType extends Type {
        public VoidType() {
        }

        public String toString() {
            return "void";
        }
    }
}
