package backend.risc_v.entity.instrcution;

public class ECallInstruction extends RiscVInstruction {
    public ECallInstruction() {

    }

    @Override
    public String toString() {
        return "ecall";
    }
}
