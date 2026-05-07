package IR.IRValueRef;

import IR.IRType.IRFunctionType;
import IR.IRType.IRType;

import java.util.ArrayList;
import java.util.List;

public class IRFunctionBlockRef implements IRValueRef{
    /*
     * 该类是函数块的引用，用于表示函数块
     *
     */
    private final List<IRBaseBlockRef> baseBlocks;/*该函数块中的基本块*/
    private final IRFunctionType type;/*该函数块的类型*/
    private final String functionName;/*该函数块的名字*/
    private final List<IRValueRef> paramsValueRef;/*该函数块的参数*/
    private List<IRValueRef> caller; /*调用该函数的函数*/
    private List<IRValueRef> callee; /*被该函数调用的函数*/
    private List<IRBaseBlockRef> retBlocks;/*该函数块的返回基本块列表*/
    public IRFunctionBlockRef(String functionName, IRFunctionType type) {
        this.functionName = functionName;
        this.type = type;
        this.baseBlocks = new ArrayList<IRBaseBlockRef>();
        this.paramsValueRef = new ArrayList<IRValueRef>();
        this.caller = new ArrayList<IRValueRef>();
        this.callee = new ArrayList<IRValueRef>();
        this.retBlocks = new ArrayList<IRBaseBlockRef>();
        int count = 0;
        for (IRType paramType : type.getParamsType()) {
            IRValueRef valRegister = new IRVirtualRegRef(functionName + (count++), paramType);
            paramsValueRef.add(valRegister);
        }
    }
    public String genIRCodes() {
        // 生成function的IR代码
        //例如：define i32 @main(i32 %main018){}
        StringBuilder sb = new StringBuilder();
        sb.append("define ").append(getRetType().getText()).append(" @").append(functionName)
                .append(getParamsString()).append(" {\n");
        for (IRBaseBlockRef baseBlock : baseBlocks) {
            sb.append(baseBlock.getLabel()).append(":\n");
            sb.append(baseBlock.getCodeBuilder());
        }
        sb.append("}\n");
        return sb.toString();
    }
    @Override
    public String getText() {
        return "@" + functionName;
    }
    @Override
    public IRType getType() {
        return type;
    }
    @Override
    public int getTypeKind() {
        return type.getTypeKind();
    }

    public IRValueRef getParam(int i) {
        return paramsValueRef.get(i);
    }
    public List<IRValueRef> getParams() {
        return paramsValueRef;
    }
    public void setParams(int k, IRValueRef newValue){paramsValueRef.set(k, newValue);}
    public String getParamsString() {
        StringBuilder sb = new StringBuilder();
        sb.append('(');
        int paramsCount = getParamsCount(this);
        if (paramsCount > 0) {
            sb.append(getParam(0).getType().getText());
            sb.append(" ").append(paramsValueRef.get(0).getText());
        }
        for (int i = 1; i < paramsCount; i++) {
            sb.append(", ")
                    .append(getParam(i).getType().getText())
                    .append(" ").append(paramsValueRef.get(i).getText());
        }
        sb.append(")");
        return sb.toString();
    }
    public static int getParamsCount(IRFunctionBlockRef function) {
        return function.paramsValueRef.size();
    }
    public void addBaseBlockRef(IRBaseBlockRef baseBlock) {
        baseBlocks.add(baseBlock);
    }
    public String getFunctionName() {
        return functionName;
    }
    public List<IRBaseBlockRef> getBaseBlocks() {
        return baseBlocks;
    }
    public void addRetBlock(IRBaseBlockRef retBlock) {
        retBlocks.add(retBlock);
    }
    public IRType getRetType() {
        return type.getReturnType();
    }

    public static IRBaseBlockRef IRGetFirstBaseBlock(IRValueRef function) {
        if (function instanceof IRFunctionBlockRef) {
            return ((IRFunctionBlockRef) function).getBaseBlocks().get(0);
        }
        else {
            System.err.println("wrong block");
            return null;
        }
    }

    public static IRBaseBlockRef IRGetNextBaseBlock(IRFunctionBlockRef function, IRValueRef block) {
        List<IRBaseBlockRef> baseBlockRefs = function.getBaseBlocks();
        for (IRBaseBlockRef baseBlockRef : baseBlockRefs) {
            if (block.equals(baseBlockRef)) {
                int index = baseBlockRefs.indexOf(baseBlockRef);
                if (index == baseBlockRefs.size() - 1)
                    return null;
                else
                    return baseBlockRefs.get(index + 1);
            }
        }
        return null;

    }
}
