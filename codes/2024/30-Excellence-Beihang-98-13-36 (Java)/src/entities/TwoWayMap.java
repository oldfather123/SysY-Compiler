package entities;

import java.util.HashMap;
import java.util.Map;
import java.util.Set;

public class TwoWayMap<F, S> {

    private final Map<F, S> FSMap;
    private final Map<S, F> SFMap;

    public TwoWayMap() {
        FSMap = new HashMap<>();
        SFMap = new HashMap<>();
    }

    public void put(F f, S s) {
        FSMap.put(f, s);
        SFMap.put(s, f);
    }

    public S getSecond(F first) {
        return FSMap.get(first);
    }

    public F getFirst(S second) {
        return SFMap.get(second);
    }

    public boolean containsFirst(F first) {
        return FSMap.containsKey(first);
    }

    public boolean containsSecond(S second) {
        return SFMap.containsKey(second);
    }

    public Set<F> getFirstSet() {
        return FSMap.keySet();
    }

    public int size() {
        return FSMap.size();
    }

    public void removeFirst(F first) {
        S second = FSMap.get(first);
        FSMap.remove(first);
        SFMap.remove(second);
    }

}
