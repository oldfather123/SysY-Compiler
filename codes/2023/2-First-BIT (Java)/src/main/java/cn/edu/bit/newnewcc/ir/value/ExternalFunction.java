package cn.edu.bit.newnewcc.ir.value;

import cn.edu.bit.newnewcc.ir.Type;
import cn.edu.bit.newnewcc.ir.type.FunctionType;

public class ExternalFunction extends BaseFunction {
    public ExternalFunction(FunctionType type, String functionName) {
        super(type);
        this.functionName = functionName;
    }

    private String functionName;

    public String getFunctionName() {
        return functionName;
    }

    @Override
    public String getValueName() {
        return functionName;
    }

    @Override
    public String getValueNameIR() {
        return '@' + functionName;
    }

    @Override
    public void setValueName(String valueName) {
        functionName = valueName;
    }

    public void emitIr(StringBuilder builder) {
        builder.append("declare ")
                .append(this.getReturnType().getTypeName())
                .append(' ')
                .append(this.getValueNameIR())
                .append('(');
        boolean isFirstParameter = true;
        for (Type parameterType : this.getParameterTypes()) {
            if (!isFirstParameter) {
                builder.append(", ");
            }
            builder.append(parameterType.getTypeName());
            isFirstParameter = false;
        }
        builder.append(")\n");
    }

}
