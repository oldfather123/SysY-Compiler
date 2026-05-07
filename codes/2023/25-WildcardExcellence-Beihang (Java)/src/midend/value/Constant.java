package midend.value;

import midend.Type;

import java.util.ArrayList;

public class Constant extends Value {
    public Constant(Type type, String name) {
        super(type, name);
    }

    public static class ConstantArray extends Constant {
        public ConstantArray(Type type, String name, ArrayList<Value> values) {
            super(type, name);
            this.values = values;
        }

        private int size; //大小
        private ArrayList<Value> values;

        public boolean isAllZero() {
            ArrayList<Value> basicValues = getBasicValueList();
            for (Value basicValue : basicValues) {
                if (basicValue instanceof ConstantInteger) {
                    int tmp = ((ConstantInteger) basicValue).getConstantIntegerValue();
                    if (tmp != 0) {
                        return false;
                    }
                }
            }
            return true;
        }

        public ArrayList<Value> getBasicValueList() {
            if (basicValueList == null) {
                this.basicValueList = new ArrayList<>();
                getBasicValue(this.basicValueList);
            }
            return basicValueList;
        }

        private ArrayList<Value> basicValueList;

        public void getBasicValue(ArrayList<Value> basicValues) {
            for (int i = 0; i < size; i++) {
                if (values.get(i) instanceof ConstantArray) {
                    ((ConstantArray)values.get(i)).getBasicValue(basicValues);
                } else {
                    basicValues.add(values.get(i));
                }
            }
        }

        // 像一维数组一样访问多维数组
        public Value getValueByIndex(int index) {
            if (basicValueList == null) {
                this.basicValueList = new ArrayList<>();
                getBasicValue(this.basicValueList);
            }
            return basicValueList.get(index);
        }

//        public Value getValueByArrayIndex(int a, int b, boolean time) {
//            for (int i = 0; i < values.size(); i++) {
//                System.out.println(values);
//            }
//            Value value = values.get(a);
//            if (time) {
//                value = ((ConstantArray) value).getValueByArrayIndex(b, a, false);
//            }
//            return value;
//        }

        public String toString() {
            StringBuilder sb = new StringBuilder();
            sb.append("[");
            for (int i = 0; i < size; i++) {
                if (i != 0) {
                    sb.append(", ");
                }
                Value subValue = values.get(i);
                sb.append(subValue.getType()).append(" ");
                sb.append(subValue.getName());
            }
            sb.append("]");
            return sb.toString();
        }

        @Override
        public String getEleName() {
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < size; i++) {
                if (i != 0) {
                    sb.append(", ");
                }
                Value subValue = values.get(i);
                sb.append(subValue.getEleName());
            }
            return sb.toString();
        }

        public int getSize() {
            return this.size;
        }

        public void setSize(int size) {
            this.size = size;
        }
    }

    public static class ConstantInteger extends Constant {

        public static final ConstantInteger Constant0 = new ConstantInteger(Type.IntegerType.I32, "0");

        private Integer value; // 整型常值

        public ConstantInteger(Type type, String name) {
            super(type, name);
            this.value = Integer.parseInt(name);
        }

        public String toString() {
            return getName();
        }

        public Integer getConstantIntegerValue() {
            return value;
        }
    }

    public static class ConstantFloat extends Constant {

        private float value; // 浮点型常值
        public static ConstantFloat float0 = new ConstantFloat(new Type.FloatType(), 0.0F);

        public ConstantFloat(Type type, float value) {
            super(type, "0x" + Long.toUnsignedString(Double.doubleToLongBits((double) value), 16));
            this.value = value;
        }

        public String toString() {
            return getName();
        }

        public float getConstantFloatValue() {
            return value;
        }

        public long getIEEEInt64() {
            return Double.doubleToRawLongBits(value);
        }

        public int getIEEEInt32() {
            return Float.floatToRawIntBits(value);
        }
    }

    public static class ZeroInitArray extends Constant {
        public ZeroInitArray(Type type, String name) {
            super(type, name);
        }
    }
}
