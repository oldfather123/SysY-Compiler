package IR.IRInstruction;

import IR.IRConst;
import IR.IRValueRef.IRBaseBlockRef;
import IR.IRValueRef.IRValueRef;

import java.util.List;

public class TypeTransferInstruction extends IRInstruction{
    IRValueRef origin;
    IRValueRef resRegister;
    int transferType;
    public TypeTransferInstruction (List<IRValueRef> operands, IRBaseBlockRef basicBlock, int transferType){
        super(operands, basicBlock);
        this.origin = operands.get(1);
        this.resRegister = operands.get(0);
        this.transferType = transferType;
    }

    public int getTransferType() {
        return transferType;
    }

    public void setResReg(IRValueRef newValue){resRegister = newValue;}

    public void setOrigin(IRValueRef newValue){origin = newValue;}
    @Override
    public String toString(){
        // transferType == 0 即为整数转浮点数
        if(transferType == IRConst.IntToFloat){
            return resRegister.getText() + " = sitofp i32 "
                    + origin.getText() + " to float";
        }else if(transferType == IRConst.FloatToInt){
            // transferType == 1 即为浮点数转整数
            return resRegister.getText() + " = fptosi float " +
                    origin.getText() + " to i32";
        }
        return null;
    }
}
