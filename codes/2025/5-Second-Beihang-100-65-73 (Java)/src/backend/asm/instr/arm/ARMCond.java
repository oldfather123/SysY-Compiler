package backend.asm.instr.arm;

import java.util.LinkedHashMap;
import java.util.Map;

public enum ARMCond {
    EQ("EQ"), // 整数或浮点数“相等”
    NE("NE"), // 整数或浮点数“不等”
    LT("LT"), // 有符号整数“小于”
    FLT("MI"), // 浮点数“小于”
    LE("LE"), // 有符号整数“小于等于”
    FLE("LS"), // 浮点数“小于等于”或无符号整数“小于等于”
    GT("GT"), // 有符号整数“大于”
    GE("GE"), // 有符号整数“大于等于”
    FGT("GT"), // 浮点数“大于”
    FGE("GE"), // 浮点数“大于等于”
    ;

    private static final Map<ARMCond, ARMCond> oppositeMap = new LinkedHashMap<>();
    static {
        oppositeMap.put(EQ, ARMCond.NE);
        oppositeMap.put(NE, ARMCond.EQ);
        oppositeMap.put(LT, ARMCond.GE);
        oppositeMap.put(FLT, ARMCond.FGE);
        oppositeMap.put(LE, ARMCond.GT);
        oppositeMap.put(FLE, ARMCond.FGT);
        oppositeMap.put(GT, ARMCond.LE);
        oppositeMap.put(GE, ARMCond.LT);
        oppositeMap.put(FGT, ARMCond.FLE);
        oppositeMap.put(FGE, ARMCond.FLT);
    }

    private final String name;

    ARMCond(String name) {
        this.name = name;
    }

    public ARMCond getOpposite() {
        return oppositeMap.get(this);
    }

    @Override
    public String toString() {
        return name;
    }
}
