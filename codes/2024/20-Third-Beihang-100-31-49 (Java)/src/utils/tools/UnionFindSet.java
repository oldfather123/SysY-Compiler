package utils.tools;

import mid.IntermediatePresentation.Value;

import java.util.HashMap;

public class UnionFindSet {
    private final HashMap<Value, Value> fatherMap;

    public UnionFindSet() {
        fatherMap = new HashMap<>();
    }

    public Value findHead(Value v) {
        if (v == null) {
            return null;
        }
        if (!fatherMap.containsKey(v)) {
            return v;
        }
        Value father = fatherMap.get(v);
        if (father != v) {
            father = findHead(father);
        }
        //将当前结点直接挂接到头结点上，降低层级
        fatherMap.put(v, father);
        return father;
    }

    public boolean isSameSet(Value a, Value b) {
        return findHead(a) == findHead(b);
    }

    public void union(Value child, Value father) {
        Value aHead = findHead(child);
        Value bHead = findHead(father);
        //两者头结点相同，不需要合并
        if (aHead == bHead) {
            return;
        }
        fatherMap.put(aHead, bHead);
    }

    public void add(Value v) {
        if (fatherMap.containsKey(v)) {
            return;
        }
        fatherMap.put(v, v);
    }
}
