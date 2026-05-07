package riscv.regalloc;

import riscv.value.AsmReg;

import java.util.Collection;
import java.util.Comparator;
import java.util.Set;
import java.util.TreeSet;

class RegPool {
    private final Set<AsmReg> pool;

    RegPool(Collection<AsmReg> asmRegs) {
        this.pool = new TreeSet<>(Comparator.comparingInt(r -> r == AsmReg.tp ? 999 : r.ordinal()));
        this.pool.addAll(asmRegs);
    }

    AsmReg fetch() {
        var it = pool.iterator();
        AsmReg r = it.hasNext() ? it.next() : null;
        if (r != null) pool.remove(r);
        return r;
    }

    void free(AsmReg r) {
        assert r != null;
        pool.add(r);
    }

    boolean isEmpty() {
        return pool.isEmpty();
    }
}
