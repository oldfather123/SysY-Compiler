package backend.RISCVCode.RISCVInstruction;

public class RISCVRet extends RISCVInstruction {
    public RISCVRet(){

    };

    @Override
    public String getString() {
        return "  ret\n";
    }

}
