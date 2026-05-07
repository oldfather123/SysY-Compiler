package IR.IRInstruction;

import IR.IRValueRef.IRBaseBlockRef;
import IR.IRValueRef.IRFunctionBlockRef;
import IR.IRValueRef.IRValueRef;
import com.sun.jdi.VoidType;

import java.util.ArrayList;
import java.util.List;

public class CallInstruction extends IRInstruction{
    private IRFunctionBlockRef functionBlock;
    private ArrayList<IRValueRef> params;
    public boolean isVoid(){
        return isVoid;
    }
    private boolean isVoid;
    public IRFunctionBlockRef getFunction(){
        return functionBlock;
    }

    public CallInstruction(List<IRValueRef> operands, IRBaseBlockRef basicBlock){
        super(operands,basicBlock);
        this.functionBlock = (IRFunctionBlockRef) operands.get(1);
        if(functionBlock.getRetType() instanceof VoidType){
            isVoid = true;
        }else{
            isVoid = false;
        }
        params = new ArrayList<IRValueRef>();
        for(int i = 2; i < operands.size();i++) {
            params.add(operands.get(i));
        }
    }

    public ArrayList<IRValueRef> getParams() {
        return params;
    }

    public void setParams(int i, IRValueRef newValue) {
        params.set(i, newValue);
    }

    @Override
    public String toString() {
        IRValueRef resRegister = this.getOperands().get(0);
        StringBuilder stringBuilder = new StringBuilder();
        if (!isVoid) {
            stringBuilder.append(resRegister.getText()).append(" = ");
        }
        stringBuilder.append("call").append(" ").append(functionBlock.getRetType().getText())
                .append(" ").append(functionBlock.getText()).append("(");
        if (params.size() > 0) {
            stringBuilder.append(params.get(0).getType().getText()).append(" ").append(params.get(0).getText());
        }
        for (int i = 0; i < params.size(); i++) {
            stringBuilder.append(", ")
                    .append(params.get(i).getType().getText())
                    .append(" ").append(params.get(i).getText());
        }
        stringBuilder.append(")");
        return stringBuilder.toString();
    }
}
