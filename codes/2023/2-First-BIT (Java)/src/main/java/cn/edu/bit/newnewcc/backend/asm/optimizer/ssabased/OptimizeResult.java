package cn.edu.bit.newnewcc.backend.asm.optimizer.ssabased;

import cn.edu.bit.newnewcc.backend.asm.instruction.AsmInstruction;
import cn.edu.bit.newnewcc.backend.asm.operand.Register;

import java.util.*;

public class OptimizeResult {
    private final List<AsmInstruction> instructions = new ArrayList<>();
    private final Map<Register, Register> registerMap = new HashMap<>();

    private OptimizeResult() {
    }

    public void addInstruction(AsmInstruction asmInstruction) {
        instructions.add(asmInstruction);
    }

    public void addInstructions(Collection<AsmInstruction> asmInstructions) {
        instructions.addAll(asmInstructions);
    }

    public void addRegisterMapping(Register source, Register destination) {
        registerMap.put(source, destination);
    }

    public void addRegisterMappings(Map<Register, Register> mappings) {
        registerMap.putAll(mappings);
    }

    public List<AsmInstruction> getInstructions() {
        return Collections.unmodifiableList(instructions);
    }

    public Map<Register, Register> getRegisterMap() {
        return Collections.unmodifiableMap(registerMap);
    }

    public static OptimizeResult getNew() {
        return new OptimizeResult();
    }
}
