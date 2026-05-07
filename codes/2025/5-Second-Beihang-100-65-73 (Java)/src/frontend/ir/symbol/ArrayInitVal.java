package frontend.ir.symbol;

import frontend.ir.Value;
import frontend.ir.objecttype.Type;
import frontend.ir.objecttype.arithmetic.ArithmeticType;
import frontend.ir.objecttype.derived.TArray;

import java.util.ArrayList;
import java.util.List;

public class ArrayInitVal extends Value {
    private final List<Value> initValues;
    private final int myLim;
    private Value defaultVal;

    public ArrayInitVal(TArray type, List<Value> initValues) {
        super(type);
        this.initValues = initValues;
        this.myLim = type.getLim();

        Type elementType = type.getElementType();
        if (elementType instanceof ArithmeticType arithmeticType) {
            this.defaultVal = arithmeticType.getDefaultValue();
        } else if (elementType instanceof TArray innerArrayType) {
            this.defaultVal = new ArrayZeroInitVal(innerArrayType);
        } else {
            throw new RuntimeException("这里只考虑了数组元素类型为算术类型或者数组类型的情况");
        }
    }

    private int elementCnt = 1;

    /**
     * just for 被clone的initVal
     */
    public int calculateElementCnt() {
        elementCnt = 1;
        if (getType() instanceof TArray tArray) {
            for (int lim : tArray.getLimList()) {
                elementCnt *= lim;
            }
        } else {
            throw new RuntimeException("calculateElementCnt: " + this + " is not arrayInitVal");
        }
        return elementCnt;
    }

    /**
     * just for arrayInitVal
     */
    public ArrayInitVal cloneAndCalculateElementCnt() {
        Type thisType = getType();
        if (thisType instanceof TArray tArray) {
            List<Value> cloneInitValue = new ArrayList<>();
            for (Value ori : initValues) {
                if (ori instanceof ArrayInitVal arrayInitVal) {
                    cloneInitValue.add(arrayInitVal.cloneAndCalculateElementCnt());
                } else {
                    cloneInitValue.add(ori);
                }
            }
            ArrayInitVal clone = new ArrayInitVal(tArray, cloneInitValue);
            clone.calculateElementCnt();
            return clone;
        } else return null;
    }

    private int nullCnt = 0;

    /**
     * 置空路径 indexList 对应元素；返回整个数组是否“全被写过”（即叶子全部置空）
     * 约定：
     * - 本方法用于“clone 后”的 ArrayInitVal（elementCnt 已是快照，不再变）
     * - indexList 的长度等于数组维度（最后一维为叶子）
     */
    public boolean replaceValue2NullWithIndexList(List<Integer> indexList) {
        // acc[0] = nullCount，acc[1] = totalLeaves（快照）
        int[] acc = new int[]{nullCnt, this.elementCnt};
        boolean allCleared = replaceValue2NullWithIndexListImpl(indexList, acc);
        nullCnt = acc[0];
        return allCleared;
    }

    /**
     * 递归实现：共享 acc 进行累计
     */
    private boolean replaceValue2NullWithIndexListImpl(List<Integer> indexList, int[] acc) {
        if (!(getType() instanceof TArray)) {
            throw new RuntimeException("replaceValue2NullWithIndexListImpl: not an array");
        }
        if (indexList == null || indexList.isEmpty()) {
            throw new RuntimeException("indexList is empty");
        }

        final int curIdx = indexList.get(0);

        // ---- 保守处理 ----
        if (initValues.size() <= curIdx) {
            return true;
        }

        Value value = initValues.get(curIdx);

        // ---- 叶子维（最后一维）----
        if (indexList.size() == 1) {
            // 只有在“之前不是 null”，本次写空才算新增一次
            if (value != null) {
                initValues.set(curIdx, null);
                acc[0]++; // 新写空了一个叶子
            } else {
                // value==null：已经是“写空”状态（或者默认等价处理），不重复计数
                initValues.set(curIdx, null);
            }
            return acc[0] >= acc[1];
        }

        // ---- 还有下层索引：子数组路径 ----
        if (value instanceof ArrayInitVal child) {
            boolean childAllCleared = child.replaceValue2NullWithIndexListImpl(indexList.subList(1, indexList.size()), acc);
            // 子数组如果整体清空，可以把父槽也折叠成 null（可选：利于节省内存）
            if (childAllCleared) {
                initValues.set(curIdx, null);
            }
            return acc[0] >= acc[1];
        }

        // 如果这里不是子数组（而 indexList 还没用完），说明维度不匹配
        throw new RuntimeException("replace数组元素初值时出现了意外：维度不匹配或非 ArrayInitVal");
    }

    /**
     * 按需扩容到 size，新增位置填 null；不会整表膨胀，只扩到需要的下标
     */
    private static void ensureCapacityWithNull(List<Value> list, int size) {
        while (list.size() < size) list.add(null);
    }


    public Value getValueWithIndexList(List<Integer> indexList) {
        int curIdx = indexList.getFirst();
        if (initValues.size() <= curIdx) {
            return this.defaultVal;
        }

        Value value = initValues.get(curIdx);
        if (indexList.size() == 1) {
            return value;
        } else if (value instanceof ArrayInitVal arrayInitVal) {
            return arrayInitVal.getValueWithIndexList(indexList.subList(1, indexList.size()));
        } else {
            throw new RuntimeException("获取数组元素初值时出现了意外");
        }
    }

    public List<Value> getInitValues() {
        return initValues;
    }

    public int getLim() {
        return myLim;
    }

    public Value getDefaultVal() {
        return defaultVal;
    }

    @Override
    public String toString() {
        return this.value2string();
    }

    @Override
    public String value2string() {
        StringBuilder sb = new StringBuilder();
        sb.append(this.getType().printIRType());
        sb.append(" [");
        int i;
        for (i = 0; i < initValues.size(); i++) {
            Value value = initValues.get(i);
            sb.append(value.toString());
            if (i < initValues.size() - 1) {
                sb.append(", ");
            }
        }
        if (i != 0 && i < this.myLim) {
            sb.append(", ");
        }
        while (i < this.myLim) {
            sb.append(this.defaultVal.toString());
            if (i < this.myLim - 1) {
                sb.append(", ");
            }
            i++;
        }
        sb.append("]");
        return sb.toString();
    }

    public static class ArrayZeroInitVal extends ArrayInitVal {
        public ArrayZeroInitVal(TArray type) {
            super(type, new ArrayList<>());
        }

        @Override
        public String value2string() {
            return this.getType().printIRType() + "zeroinitializer";
        }
    }
}
