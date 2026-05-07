package entities;

import util.Strings;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static entities.Registers.*;

public class RegisterAllocator {

    private final TwoWayMap<Integer, Integer> rMap = new TwoWayMap<>(), fMap = new TwoWayMap<>();
    private final Map<Integer, Integer> rLastLine = new HashMap<>(), fLastLine = new HashMap<>();
    private final Map<Integer, Integer> rLRU = new HashMap<>(), fLRU = new HashMap<>();
    private int extra = 0, time = 0, maxUseR = 0, maxUseF = 0;

    public int getExtra() {
        return extra;
    }

    public Pair<List<Integer>, List<Integer>> getUsedRegister() {
        List<Integer> rUsed = new ArrayList<>(), fUsed = new ArrayList<>();
        for (int i = 0; i < R_SIZE; i++) {
            if (rMap.containsSecond(i)) {
                rUsed.add(i);
            }
        }
        for (int i = 0; i < F_SIZE; i++) {
            if (fMap.containsSecond(i)) {
                fUsed.add(i);
            }
        }
        return new Pair<>(rUsed, fUsed);
    }

    public void putLineR(int num, int idx) {
        rLastLine.put(num, idx);
    }

    public void putLineS(int num, int idx) {
        fLastLine.put(num, idx);
    }

    public void tryFreeR(int idx) {
        List<Integer> maybeFree = new ArrayList<>();
        for (Integer num : rMap.getFirstSet()) {
            if (rLastLine.get(num) <= idx) {
                maybeFree.add(num);
            }
        }
        for (Integer num : maybeFree) {
            rMap.removeFirst(num);
        }
    }

    public void tryFreeS(int idx) {
        List<Integer> maybeFree = new ArrayList<>();
        for (Integer num : fMap.getFirstSet()) {
            if (fLastLine.get(num) <= idx) {
                maybeFree.add(num);
            }
        }
        for (Integer num : maybeFree) {
            fMap.removeFirst(num);
        }
    }

    public Register getR(int num) {
        boolean hasR = rMap.containsFirst(num);
        int R = hasR ? rMap.getSecond(num) : getR();
        if (R >= 0) {
            rLRU.put(R, time++);
            rMap.put(num, R);
            maxUseR = Math.max(maxUseR, rMap.size());
            return Registers.r(R);
        }
        maxUseR = R_SIZE;
        int minTime = Integer.MAX_VALUE, minIndex = -1;
        for (int i = 0; i < R_SIZE; i++) {
            if (minTime > rLRU.getOrDefault(i, minTime)) {
                minTime = rLRU.get(i);
                minIndex = i;
            }
        }
        rLRU.put(minIndex, time++);
        if (hasR) {
            int R2 = getR();
            if (R2 < 0) {
                rMap.put(rMap.getFirst(minIndex), R2);
                rMap.put(num, minIndex);
                return Register.label(R + Strings.DOUBLE_ARROW + Registers.r(minIndex) + Strings.DOUBLE_ARROW + R2);
            }
            rMap.put(num, R2);
            return Register.label(R + Strings.LEFT_ARROW + Registers.r(R2));
        }
        rMap.put(rMap.getFirst(minIndex), R);
        rMap.put(num, minIndex);
        return Register.label(Registers.r(minIndex) + Strings.RIGHT_ARROW + R);
    }

    public Register getF(int num) {
        boolean hasF = fMap.containsFirst(num);
        int F = hasF ? fMap.getSecond(num) : getF();
        if (F >= 0) {
            fLRU.put(F, time++);
            fMap.put(num, F);
            maxUseF = Math.max(maxUseF, fMap.size());
            return Registers.f(F);
        }
        maxUseF = F_SIZE;
        int minTime = Integer.MAX_VALUE, minIndex = -1;
        for (int i = 0; i < F_SIZE; i++) {
            if (minTime > fLRU.getOrDefault(i, minTime)) {
                minTime = fLRU.get(i);
                minIndex = i;
            }
        }
        if (minIndex >= 0) {
            fLRU.put(minIndex, time++);
            fMap.put(fMap.getFirst(minIndex), F);
            fMap.put(num, minIndex);
            if (hasF) {
                return Register.label(F + Strings.LEFT_ARROW + Registers.f(minIndex));
            } else {
                return Register.label(Registers.f(minIndex) + Strings.RIGHT_ARROW + F);
            }
        }
        throw new RuntimeException("No f register available");
    }

    public int getR() {
        return getUnusedSecond(R_SIZE, rMap);
    }

    public int getF() {
        return getUnusedSecond(F_SIZE, fMap);
    }

    private int getUnusedSecond(int size, TwoWayMap<Integer, Integer> map) {
        for (int i = 0; i < size; i++) {
            if (!map.containsSecond(i)) {
                return i;
            }
        }
        for (int i = 1; i < extra + 1; i++) {
            if (!map.containsSecond(-i)) {
                return -i;
            }
        }
        extra++;
        return -extra;
    }

    public int getMaxNoUseSR() {
        return S_SIZE - Math.max(maxUseR - T_SIZE, 0);
    }

    public int getMaxNoUseSF() {
        return FS_SIZE - Math.max(maxUseF - FT_SIZE, 0);
    }

}
