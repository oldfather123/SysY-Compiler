package mid.IntermediatePresentation.Array;

import backend.instructions.BackLoad;
import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.User;
import mid.IntermediatePresentation.Value;
import mid.IntermediatePresentation.ValueType;

import java.util.ArrayList;
import java.util.HashMap;

public class ArrayInitializer extends User {
    private final HashMap<Integer, Value> initVals = new HashMap<>();
    private boolean isZeroInit = false;
    private BasicBlock block;
    private final int len;

    public ArrayInitializer(ArrayList<Value> initVals, int len) {
        super("ARRAY_INIT", ValueType.NULL);
        for (int i = 0; i < initVals.size(); i++) {
            if (initVals.get(i) instanceof ConstNumber n && n.getVal().floatValue() == 0) {
                continue;
            }
            this.initVals.put(i, initVals.get(i));
        }
        for (Value v : initVals) {
            use(v);
        }
        block = IRManager.getInstance().getCurBlock();
        this.len = len;
    }

    public ArrayInitializer(int len) {
        super("ARRAY_INIT", ValueType.NULL);
        //全部填0
        this.isZeroInit = true;
        this.len = len;
    }

    public boolean setVal(int index, Value val) {
        if(index > len) return false;
//        // 目前只在LAL时用到，初始化时initVal没有被实际使用，故暂时不需要这句
//        replaceOperand(initVals.get(index), val);
        this.isZeroInit = false;
        this.initVals.put(index, val);
        return true;
    }

    public HashMap<Integer, Value> getVals() {
        return initVals;
    }

    public void merge(ArrayInitializer other, int offset) {
        for (int key : other.getVals().keySet()) {
            Value v = other.getVals().get(key);
            if (v instanceof ConstNumber n && n.getVal().floatValue() == 0) {
                return;
            }
            initVals.put(key + offset, v);
        }
    }

    public void add(int index, Value v) {
        if (v instanceof ConstNumber n && n.getVal().floatValue() == 0) {
            return;
        }
        use(v);
        initVals.put(index, v);
    }

    public String toString() {
        if (isZeroInit || initVals.isEmpty()) {
            return "zeroinitializer";
        } else {
            ArrayList<Value> values = new ArrayList<>();
            StringBuilder sb = new StringBuilder();
            String type = vType.getRefType().toString();
            sb.append("[ ");
            for (int i = 0; i < len; i++) {
                Value v = initVals.getOrDefault(i, new ConstNumber(0).withType(vType.getRefType() == ValueType.I32));
                values.add(v);
                sb.append(type).append(" ").append(v).append(", ");
            }
            sb.delete(sb.length() - 2, sb.length());
            sb.append(" ]");
            return sb.toString();
        }
    }

    public BasicBlock getBlock() {
        return block;
    }

    public void setType(ValueType vType) {
        this.vType = new ValueType(len, vType == ValueType.PI32);
    }

    public int getLen() {
        return len;
    }

    public Boolean getISZeroInit() {
        return this.isZeroInit || initVals.isEmpty();
    }
}

