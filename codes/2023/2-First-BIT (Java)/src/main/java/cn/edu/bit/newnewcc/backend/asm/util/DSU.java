package cn.edu.bit.newnewcc.backend.asm.util;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

class DSU<T> {
    private final ArrayList<Integer> fa = new ArrayList<>();
    private final ArrayList<T> item = new ArrayList<>();
    private final Map<String, Integer> id = new HashMap<>();

    public DSU() {}

    private int getfa(int v) {
        return fa.get(v) == v ? v : getfa(fa.get(v));
    }

    private int getItemIndex(T v) {
        if (!id.containsKey(v.toString())) {
            int index = fa.size();
            fa.add(index);
            item.add(v);
            id.put(v.toString(), index);
        }
        return id.get(v.toString());
    }

    private T getIndexItem(int index) {
        return item.get(index);
    }

    public T getfa(T v) {
        return getIndexItem(getfa(getItemIndex(v)));
    }

    public void merge(T u, T v) {
        int idu = getfa(getItemIndex(u));
        int idv = getfa(getItemIndex(v));
        if (idu != idv) {
            fa.set(idu, idv);
        }
    }
}
