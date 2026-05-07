package IR.IRInstruction;

import IR.IRValueRef.IRBaseBlockRef;
import IR.IRValueRef.IRValueRef;

import java.util.List;

public class CalculateInstruction extends IRInstruction {
    String type;

    public CalculateInstruction(List<IRValueRef> operands, IRBaseBlockRef basicBlock, String type) {
        super(operands, basicBlock);
        this.type = type;
    }

    public String getType() {
        return type;
    }

    @Override
    public String toString() {
        IRValueRef resRegister = getOperands().get(0);
        IRValueRef lhsValRef = getOperands().get(1);
        IRValueRef rhsValRef = getOperands().get(2);
        switch (type) {
            case "add":
                return resRegister.getText() + " =  add " + resRegister.getType().getText() + " " + lhsValRef.getText() + ", " + rhsValRef.getText();
            case "fadd":
                return resRegister.getText() + " = fadd " + resRegister.getType().getText() + " " + lhsValRef.getText() + ", " + rhsValRef.getText();
            case "sub":
                return resRegister.getText() + " = sub " + resRegister.getType().getText() + " " + lhsValRef.getText() + ", " + rhsValRef.getText();
            case "fsub":
                return resRegister.getText() + " = fsub " + resRegister.getType().getText() + " " + lhsValRef.getText() + ", " + rhsValRef.getText();
            case "mul":
                return resRegister.getText() + " = mul " + resRegister.getType().getText() + " " + lhsValRef.getText() + ", " + rhsValRef.getText();
            case "fmul":
                return resRegister.getText() + " = fmul " + resRegister.getType().getText() + " " + lhsValRef.getText() + ", " + rhsValRef.getText();
            case "sdiv":
                //有符号除法
                return resRegister.getText() + " = sdiv " + resRegister.getType().getText() + " " + lhsValRef.getText() + ", " + rhsValRef.getText();
            case "fdiv":
                return resRegister.getText() + " = fdiv " + resRegister.getType().getText() + " " + lhsValRef.getText() + ", " + rhsValRef.getText();
            case "srem":
                return resRegister.getType() + " = srem " + resRegister.getType().getText() + " " + lhsValRef.getText() + ", " + rhsValRef.getText();
            case "xor":
                return resRegister.getText() + " = xor " + rhsValRef.getType().getText() + " " + lhsValRef.getText() + ", true";
        }
        return null;
    }
}
