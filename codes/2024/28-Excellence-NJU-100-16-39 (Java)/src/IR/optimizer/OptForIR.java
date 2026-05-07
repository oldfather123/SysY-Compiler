package IR.optimizer;

import IR.IRValueRef.IRFunctionBlockRef;

public interface OptForIR {
    void Optimize(IRFunctionBlockRef irFunctionBlockRef);
}
