package riscv.util;

import midend.value.Argument;
import midend.value.Value;
import midend.value.instrs.Instr;
import midend.value.instrs.Loc;
import riscv.value.AsmLoc;
import riscv.value.AsmReg;

import java.util.HashMap;
import java.util.List;

public class AllocTable {
    private final HashMap<Value, AsmReg> regMap = new HashMap<>(); // Value = Reg | GetElementPointer
    private final HashMap<Value, AsmLoc> spillMap = new HashMap<>(); // Value = Reg | GetElementPointer
    private final HashMap<Loc, AsmLoc> locMap = new HashMap<>();
    private final HashMap<Argument, Object> paramMap = new HashMap<>(); // Object = AsmReg | AsmLoc
    // 某一条指令的活跃寄存器记录，用于保存现场
    // 只记录 Call 和 CallVoid 指令的
    public final HashMap<Instr, List<AsmReg>> rActiveListMap = new HashMap<>();

    private int currentTop = 0;

    public boolean isAsmReg(Value irReg) {
        return regMap.containsKey(irReg);
    }

    public boolean isSpill(Value irReg) {
        return spillMap.containsKey(irReg);
    }

    public void addReg(Value irReg, AsmReg asmReg) {
        assert asmReg != null;
        if (regMap.containsKey(irReg)) return;
        regMap.put(irReg, asmReg);
    }

    public AsmReg getAsmRegOfReg(Value irReg) {
        return regMap.get(irReg);
    }

    public boolean hasLoc(Loc loc) {
        return locMap.containsKey(loc);
    }

    public void allocStackLoc(Loc loc, int size) {
        assert size % 4 == 0;
        addStackLoc(loc, currentTop);
        currentTop += size;
    }

    public void addStackLoc(Loc loc, int offset) {
        assert offset % 4 == 0;
        addLoc(loc, new AsmLoc(AsmReg.sp, offset));
    }

    public void addLoc(Loc loc, AsmLoc asmLoc) {
        assert asmLoc != null;
        locMap.put(loc, asmLoc);
    }

    public AsmLoc getAsmLocOfLoc(Loc loc) {
        return locMap.get(loc);
    }

    private int intTypeParamCount = 0;
    private int floatTypeParamCount = 0;

    public void addParam(Argument param) {
        int paramSize = param.getType().isPointerType() ? 8 : 4;
        if (param.getType().isFloatType()) {
            if (floatTypeParamCount < AsmReg.fastCallFRegs.length) {
                paramMap.put(param, AsmReg.fastCallFRegs[floatTypeParamCount]);
            }
            else {
                paramMap.put(param, new AsmLoc(AsmReg.sp, currentTop));
                currentTop += paramSize;
            }
            floatTypeParamCount++;
        }
        // 指针和 int
        else {
            if (intTypeParamCount < AsmReg.fastCallRegs.length) {
                paramMap.put(param, AsmReg.fastCallRegs[intTypeParamCount]);
            }
            else {
                paramMap.put(param, new AsmLoc(AsmReg.sp, currentTop));
                currentTop += paramSize;
            }
            intTypeParamCount++;
        }

    }

    // Object = AsmReg | AsmLoc
    public Object getParamPosition(Argument param) {
        return paramMap.get(param);
    }

    public void addSpill(Value irReg) {
        spillMap.put(irReg, new AsmLoc(AsmReg.sp, currentTop));
        // 溢出寄存器一律对齐 8 字节
        currentTop += 8;
    }

    public AsmLoc getSpillLoc(Value irReg) {
        return spillMap.get(irReg);
    }


    public int frameSize() {
        return currentTop;
    }
}
