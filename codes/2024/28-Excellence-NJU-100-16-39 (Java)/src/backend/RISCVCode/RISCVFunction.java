package backend.RISCVCode;

import IR.IRValueRef.IRBaseBlockRef;
import IR.IRValueRef.IRValueRef;

import java.util.*;

/**
 * RISCVFunction类，包含多个RISCV块（RISCVBlock）
 */
public class RISCVFunction {
    private ArrayList<RISCVBlock> blocks;

    String functionName;

    RISCVLabel functionLabel;

    int stackSize;

    private int stackTopOffset; // 栈顶偏移量，例如栈大小为24，则栈顶偏移量依次为20，16，12，8，4，0

    private int paramStackSize;

    private int stackSizeLocation = 0;

    private LinkedHashMap<LinkedHashMap<IRBaseBlockRef, IRValueRef>, String> phiDestAndSrc;


    private LinkedHashMap<String, String> paramsAndRegister; // 用于存储传递进来的参数的寄存器地址（其实是固定的）第一个参数 a0，第二个 a1……

    public void setStackSize(int size) {
        stackSize = size;
        if (size != 0)
            stackTopOffset = 0;
    }

    public RISCVFunction(String functionName, LinkedHashMap<String, String> paramsAndRegister, int paramStackSize) {
        this.blocks = new ArrayList<>();
        this.functionName = functionName;
        functionLabel = new RISCVLabel(functionName);
        this.paramsAndRegister = paramsAndRegister;
        this.paramStackSize = paramStackSize;
        phiDestAndSrc = new LinkedHashMap<>();
    }

    public void addPhiDestAndSrc(LinkedHashMap<IRBaseBlockRef, IRValueRef> src, String dest) {
        phiDestAndSrc.put(src, dest);
    }

    public LinkedHashMap<LinkedHashMap<IRBaseBlockRef, IRValueRef>, String> getPhiDestAndSrc() {
        return phiDestAndSrc;
    }

    public void addBlock(RISCVBlock block) {
        blocks.add(block);
    }

    public List<RISCVBlock> getBlocks() {
        return this.blocks;
    }

    public String getFunctionName() {
        return functionName;
    }

    public RISCVLabel getFunctionLabel() {
        return functionLabel;
    }

    public int getStackSize() {
        return stackSize;
    }

    public String getParamsRegister(String varName) {
        return paramsAndRegister.get(varName);
    }

    public void setStackTopOffset(String stackTop) {
            int tmp = Integer.parseInt(stackTop);
            if (tmp + 8 > stackTopOffset)
                stackTopOffset = tmp + 8;
    }

    public int getStackTopOffset() {
        return stackTopOffset;
    }
}
