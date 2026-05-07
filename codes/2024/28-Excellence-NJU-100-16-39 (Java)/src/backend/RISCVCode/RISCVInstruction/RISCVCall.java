package backend.RISCVCode.RISCVInstruction;

public class RISCVCall extends RISCVInstruction{
    private final String destFunc;

    public RISCVCall(String destFunc) {
        this.destFunc = destFunc;
    }

    @Override
    public String getString() {
        return "  call " + destFunc + "\n";
    }
}
