package backend.risc_v.entity.instrcution;

public class CallInstruction extends RiscVInstruction{
    private final String functionName;

    public CallInstruction(String functionName) {
        this.functionName = functionName;
    }

    @Override
    public String toString() {
        return "call " + functionName;
    }
}
