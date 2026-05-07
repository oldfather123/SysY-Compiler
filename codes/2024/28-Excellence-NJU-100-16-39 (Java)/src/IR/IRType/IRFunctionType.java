package IR.IRType;

import IR.IRConst;

import java.util.List;

public class IRFunctionType implements IRType{
    private final IRType returnType;
    private final List<IRType> paramsType;
    private final int paramsCount;
    public IRFunctionType(List<IRType> paramsType, IRType returnType) {
        this.paramsType = paramsType;
        this.returnType = returnType;
        this.paramsCount = paramsType.size();
    }
    @Override
    public String getText() {
        StringBuilder sb = new StringBuilder();
        sb.append('(');
        // e.g. int main(int i) 需要翻译成(i32, i32)
        if (paramsCount > 0) {
            sb.append(paramsType.get(0).getText());
        }
        for (int i = 1; i < paramsCount; i++) {
            sb.append(", ").append(paramsType.get(i).getText());
        }
        sb.append(")");
        return sb.toString();
    }
    @Override
    public int getTypeKind() {
        return IRConst.IRConstantFunctionValueKind;
    }

    public int getParamsCount() {
        return paramsCount;
    }

    public List<IRType> getParamsType() {
        return paramsType;
    }

    public IRType getParamType(int index) {
        return paramsType.get(index);
    }

    public IRType getReturnType() {
        return returnType;
    }
}
