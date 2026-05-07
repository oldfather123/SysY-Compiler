package IR.IRInstruction;

import IR.IRValueRef.IRBaseBlockRef;
import IR.IRValueRef.IRValueRef;

import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public class PhiInstruction extends IRInstruction {
    private final LinkedHashMap<IRBaseBlockRef, IRValueRef> incomingValues;

    public PhiInstruction(List<IRValueRef> operands, IRBaseBlockRef baseBlock) {
        super(operands, baseBlock);
        this.incomingValues = new LinkedHashMap<>();
    }

    public void addIncomingValue(IRValueRef value, IRBaseBlockRef block) {
        this.incomingValues.put(block, value);
    }

    public LinkedHashMap<IRBaseBlockRef, IRValueRef> getIncomingValues() {
        return this.incomingValues;
    }

    public void setIncomingValue(IRValueRef value, IRBaseBlockRef block) {
        this.incomingValues.put(block, value);
    }

    @Override
    public String toString() {
        IRValueRef resRegister = getOperands().get(0);
        StringBuilder sb = new StringBuilder();
        sb.append(resRegister.getText()).append(" = phi ").append(resRegister.getType().getText()).append(" ");
        boolean first = true;
        for (Map.Entry<IRBaseBlockRef, IRValueRef> entry : incomingValues.entrySet()) {
            if (!first) {
                sb.append(", ");
            }
            sb.append("[ ").append(entry.getValue().getText()).append(", ").append(entry.getKey().getText()).append(" ]");
            first = false;
        }
        return sb.toString();
    }
}
