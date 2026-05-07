package frontend.ir.symbols;

import frontend.ir.DataType;
import frontend.ir.Value;
import frontend.ir.constvalue.ConstFloat;
import frontend.ir.constvalue.ConstInt;
import frontend.ir.constvalue.ConstValue;

import java.util.ArrayList;
import java.util.List;

public class ArrayInitVal extends Value {
    private final DataType dataType;
    private final ArrayList<Value> initList = new ArrayList<>();
    private final List<Integer> limList;
    private final boolean hasInit;
    
    public ArrayInitVal(DataType dataType, List<Integer> limList, boolean hasInit) {
        this.dataType = dataType;
        this.limList = limList;
        this.hasInit = hasInit;
    }
    
    public void addInitValue(Value newVal) {
        initList.add(newVal);
    }
    
    public Value getValueWithIndex(List<Integer> indexList) {
        if (indexList == null) {
            throw new NullPointerException();
        }
        int index = indexList.get(0);
        if (index >= limList.get(0)) {
            throw new RuntimeException("越界访问");
        }
        
        if (indexList.get(0) >= initList.size()) {
            switch (dataType) {
                case INT:   return ConstInt.Zero;
                case FLOAT: return ConstFloat.Zero;
                default: throw new RuntimeException("?");
            }
        }
        Value init = initList.get(indexList.get(0));
        if (indexList.size() > 1) {
            if (init instanceof ArrayInitVal) {
                return ((ArrayInitVal) init).getValueWithIndex(indexList.subList(1, indexList.size()));
            } else {
                throw new RuntimeException("非基层数组不是数组？");
            }
        } else {
            return init;
        }
    }
    
    /**
     * 这个函数用来获取那些地方存了非零的初始化值
     * 前一个用来记录结果，后一个是递归过程中记录前驱序号用的，初始都是空列表
     */
    public void getNonZeroIndex(List<List<Integer>> res, List<Integer> indexList) {
        if (getDim() == 1) {
            for (int i = 0; i < initList.size(); i++) {
                Value init = initList.get(i);
                if (init instanceof ConstValue && init.getNumber().intValue() == 0) {
                    continue;
                }
                ArrayList<Integer> newInnerRes = new ArrayList<>(indexList);
                newInnerRes.add(i);
                res.add(newInnerRes);
            }
        } else {
            for (int i = 0; i < initList.size(); i++) {
                Value init = initList.get(i);
                if (init instanceof ArrayInitVal) {
                    ArrayList<Integer> myIndexList = new ArrayList<>(indexList);
                    myIndexList.add(i);
                    ((ArrayInitVal) init).getNonZeroIndex(res, myIndexList);
                } else {
                    throw new RuntimeException("非基层数组不是数组？");
                }
            }
        }
    }
    
    public boolean isFull() {
        return this.initList.size() == limList.get(0);
    }
    
    public int getDim() {
        return this.limList.size();
    }
    
    @Override
    public Number getNumber() {
        throw new RuntimeException("占位类，该方法不应该被调用");
    }
    
    @Override
    public DataType getDataType() {
        return dataType;
    }
    
    @Override
    public String value2string() {
        if (initList.isEmpty()) {
            return "zeroinitializer";
        }
        StringBuilder stringBuilder = new StringBuilder();
        if (limList.size() == 1) {
            boolean hasNonZero = false;
            for (Value init : initList) {
                if (init.getNumber().intValue() != 0) {
                    hasNonZero = true;
                    break;
                }
            }
            if (hasNonZero) {
                stringBuilder.append("[");
                int lim = limList.get(0);
                for (int i = 0; i < lim; i++) {
                    Value value;
                    if (i < initList.size()) {
                        value = initList.get(i);
                    } else {
                        switch (dataType) {
                            case INT:   value = ConstInt.Zero;    break;
                            case FLOAT: value = ConstFloat.Zero;  break;
                            default: throw new RuntimeException();
                        }
                    }
                    stringBuilder.append(dataType).append(" ").append(value.value2string());
                    if (i < lim - 1) {
                        stringBuilder.append(", ");
                    }
                }
                stringBuilder.append("]");
            } else {
                return "zeroinitializer";
            }
        } else {
            StringBuilder sb = new StringBuilder();
            boolean hasNonZero = false;
            for (int i = 0; i < limList.get(0); i++) {
                if (i < initList.size()) {
                    Value init = initList.get(i);
                    if (init instanceof ArrayInitVal) {
                        sb.append(((ArrayInitVal) init).printArrayTypeName()).append(" ");
                        String initStr = init.value2string();
                        if (!initStr.equals("zeroinitializer")) {
                            hasNonZero = true;
                        }
                        sb.append(initStr);
                    } else {
                        throw new RuntimeException("非基层数组里出现单蹦元素");
                    }
                } else {
                    sb.append(((ArrayInitVal) initList.get(0)).printArrayTypeName());
                    sb.append(" zeroinitializer");
                }
                if (i < limList.get(0) - 1) {
                    sb.append(", ");
                }
            }
            if (hasNonZero) {
                stringBuilder.append("[").append(sb).append("]");
            } else {
                stringBuilder.append("zeroinitializer");
            }
            
        }
        
        
        return stringBuilder.toString();
    }
    
    public String printArrayTypeName() {
        int start = 0;
        if (limList.get(0) == -1) { // 第一维长度记为 -1 说明第一维长度缺省，即是传参用的指针
            start = 1;              // 从第一个开始遍历，即去掉外侧的一层括号，最后还要加一个星号
        }
        StringBuilder stringBuilder = new StringBuilder();
        for (int i = start; i < limList.size(); i++) {
            Integer index = limList.get(i);
            stringBuilder.append("[").append(index).append(" x ");
        }
        stringBuilder.append(dataType);
        for (int i = start; i < limList.size(); i++) {
            stringBuilder.append("]");
        }
        if (start == 1) {
            stringBuilder.append("*");
        }
        return stringBuilder.toString();
    }
    
    public int getSize() {
        // todo 会不会爆 int 啊
        int size = 4;   // i32 和 float 都是 4 字节
        for (Integer integer : limList) {
            size *= integer;
        }
        return size;
    }

    public ArrayList<Value> getInitList() {
        return initList;
    }

    public int getLimSize() {
        int limSize = 1;
        for (int i = 0; i < limList.size(); i++) {
            limSize *= limList.get(i);
        }
        return limSize;
    }
    public int getDimSize(int dim) {
        return limList.get(dim);
    }

    public List<Integer> getlimLst() {
        return limList;
    }
    
    public boolean checkHasInit() {
        return hasInit;
    }
}
