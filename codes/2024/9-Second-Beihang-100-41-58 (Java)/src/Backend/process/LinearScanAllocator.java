package Backend.process;

import Backend.component.AsmBlock;
import Backend.component.AsmInstr;
import Backend.instruction.AsmMove;
import Backend.instruction.AsmStore;
import Backend.operand.AsmOperand;
import Backend.operand.AsmReg;
import Backend.operand.VirtualReg;

import java.util.*;

public class LinearScanAllocator {
    private final HashMap<AsmBlock, LinkedList<AsmInstr>> block2Instr;
    private final boolean isOpen;
    private final Map<AsmOperand, Integer> spillValue;
    private final Map<String, Integer> regUsageReport;
    private final Map<String, Integer> regUsageStats;
    private final List<String> allocationTrace;
    private final int totalInstructions;
    private final int totalOperands;
    private int totalSpills;

    public LinearScanAllocator(HashMap<AsmBlock, LinkedList<AsmInstr>> block2Instr, boolean isOpen) {
        this.block2Instr = block2Instr;
        this.isOpen = isOpen;
        this.spillValue = new HashMap<>();
        this.regUsageReport = new HashMap<>();
        this.regUsageStats = new HashMap<>();
        this.allocationTrace = new ArrayList<>();
        this.totalInstructions = block2Instr.values().stream().mapToInt(LinkedList::size).sum();
        this.totalOperands = calculateTotalOperands(block2Instr);
        this.totalSpills = 0;
        if (isOpen) {
            run();
            RegisterUsageAnalyzer.analyze(this);
        }
    }

    public void run() {
        if (!isOpen) {
            return;
        }
        Map<AsmOperand, Integer> regAssignments = new HashMap<>();
        PriorityQueue<AsmOperand> activeOperands = new PriorityQueue<>(Comparator.comparingInt(AsmOperand::getSpillPlace));
        for (AsmBlock asmBlock : block2Instr.keySet()) {
            for (AsmInstr instr : block2Instr.get(asmBlock)) {
                InstructionDetailsPrinter.print(instr);
                for (AsmReg use : instr.getRegUse()) {
                    if (!use.isPreColored()) {
                        if (activeOperands.contains(use)) {
                            assignRegister(use, regAssignments, activeOperands);
                            updateRegisterUsageStats(use);
                            spillValue.put(use, use.getSpillPlace());
                        }
                    }
                    OperandDetailsPrinter.print(use);
                }
                AsmReg def = instr.getRegDef();
                if (!def.isPreColored()) {
                    activeOperands.add(def);
                }
                spillValue.put(def, def.getSpillPlace());
                OperandDetailsPrinter.print(def);
                updateRegisterUsageStats(def);
                updateActiveList(activeOperands, block2Instr.get(asmBlock));
            }
        }
        handleRemainingActiveOperands(activeOperands, regAssignments);
        printRegUsageReport();
        printSummaryReport();
    }

    public Map<AsmOperand, Integer> getSpillValue() {
        return spillValue;
    }

    private void printRegUsageReport() {
        System.out.println("Register Usage Report:");
        for (Map.Entry<String, Integer> entry : regUsageReport.entrySet()) {
            System.out.println("Register " + entry.getKey() + " used " + entry.getValue() + " times");
        }
        System.out.println("Total spills: " + totalSpills);
    }

    private void printSummaryReport() {
        System.out.println("Total Instructions: " + totalInstructions);
        System.out.println("Total Operands: " + totalOperands);
        System.out.println("Total Spills: " + totalSpills);
        System.out.println("Register Usage Statistics:");
        regUsageStats.forEach((reg, count) -> System.out.println("  " + reg + ": " + count));
        System.out.println("Allocation Trace:");
        allocationTrace.forEach(System.out::println);
    }

    private int calculateTotalOperands(HashMap<AsmBlock, LinkedList<AsmInstr>> block2Instr) {
        int total = 0;
        for (LinkedList<AsmInstr> instrs : block2Instr.values()) {
            for (AsmInstr instr : instrs) {
                total += instr.getRegUse().size() + (instr.getRegDef() != null ? 1 : 0);
            }
        }
        return total;
    }

    private void assignRegister(AsmOperand operand, Map<AsmOperand, Integer> regAssignments, PriorityQueue<AsmOperand> activeOperands) {
        AsmReg reg = findFreeReg(operand);
        if (reg != null) {
            regAssignments.put(operand, reg.getSpillPlace());
            operand.setColor(operand.getSpillPlace());
            activeOperands.remove(operand);
            logAllocationDecision(operand, reg);
        } else {
            AsmOperand candidate = selectSpillCandidate(activeOperands);
            spillOperand(candidate, regAssignments);
            regAssignments.put(operand, Objects.requireNonNull(findFreeReg(operand)).getSpillPlace());
            operand.setColor(operand.getSpillPlace());
        }
        String regName = "Reg" + operand.getSpillPlace();
        regUsageReport.put(regName, regUsageReport.getOrDefault(regName, 0) + 1);
    }

    private void debugActiveOperands(PriorityQueue<AsmOperand> activeOperands) {
        System.out.println("Active operands:");
        for (AsmOperand operand : activeOperands) {
            System.out.println("  " + operand);
        }
    }

    private void handleSpill(AsmOperand operand) {
        totalSpills++;
        System.out.println("Spilling operand " + operand + " to memory.");
    }

    private void logInstruction(AsmInstr instr) {
        System.out.println("Processing instruction: " + instr.toString());
        for (AsmOperand use : instr.getRegUse()) {
            System.out.println("  uses operand " + use);
        }
        AsmOperand def = instr.getRegDef();
        if (def != null) {
            System.out.println("   defines operand " + def);
        }
    }

    private void logAllocationDecision(AsmOperand operand, AsmReg reg) {
        String decision = "Allocated " + operand + " to " + reg;
        allocationTrace.add(decision);
        System.out.println(decision);
    }

    private void logInstructionDetails(AsmInstr instr) {
        String details = getInstructionDetails(instr);
        System.out.println(details);
    }

    private String getInstructionDetails(AsmInstr instr) {
        return "Instruction: " + instr;
    }

    private void logOperandDetails(AsmOperand operand) {
        String details = getOperandDetails(operand);
        System.out.println(details);
    }

    private String getOperandDetails(AsmOperand operand) {
        return "Operand: " + operand;
    }

    private void updateRegisterUsageStats(AsmReg reg) {
        String regName = reg.getClass().getSimpleName();
        regUsageStats.put(regName, regUsageStats.getOrDefault(regName, 0) + 1);
    }

    private AsmReg findFreeReg(AsmOperand asmOperand) {
        if (asmOperand.isPreColored()) {
            return null;
        }
        return new VirtualReg();
    }

    private AsmOperand selectSpillCandidate(PriorityQueue<AsmOperand> activeOperands) {
        TreeMap<AsmOperand, Integer> map = new TreeMap<>();
        for (AsmOperand asmOperand : activeOperands) {
            int spillValue = asmOperand.getSpillPlace();
            int color = asmOperand.getColor();
            map.put(asmOperand, 2 * spillValue + 3 * color);
            logOperandDetails(asmOperand);
        }
        return map.lastKey();
    }

    private void spillOperand(AsmOperand operand, Map<AsmOperand, Integer> regAssignments) {
        int spillValue = regAssignments.get(operand);
        for (AsmBlock asmBlock : block2Instr.keySet()) {
            LinkedList<AsmInstr> instrs = block2Instr.get(asmBlock);
            for (AsmInstr instr : instrs) {
                if (instr.getRegUse().contains((AsmReg) operand)) {
                    AsmStore store = new AsmStore(Arrays.asList(operand, new VirtualReg(), operand), "sd");
                    AsmMove move = new AsmMove(Arrays.asList(operand, new VirtualReg()));
                    asmBlock.addInstr(move);
                    asmBlock.addInstr(store);
                    regAssignments.put(operand, spillValue);
                }
                logInstruction(instr);
                logInstructionDetails(instr);
            }
        }
    }

    private void updateActiveList(PriorityQueue<AsmOperand> activeOperands, LinkedList<AsmInstr> instrs) {
        Iterator<AsmInstr> iterator = instrs.iterator();
        while (iterator.hasNext()) {
            AsmInstr nowInstr = iterator.next();
            if (activeOperands.contains(nowInstr.getRegDef())) {
                iterator.remove();
            } else {
                for (AsmOperand asmOperand : nowInstr.getRegUse()) {
                    if (activeOperands.contains(asmOperand)) {
                        iterator.remove();
                    }
                }
            }
        }
        debugActiveOperands(activeOperands);
    }

    private void handleRemainingActiveOperands(PriorityQueue<AsmOperand> activeOperands, Map<AsmOperand, Integer> regAssignments) {
        while (!activeOperands.isEmpty()) {
            AsmOperand operand = activeOperands.poll();
            if (!regAssignments.containsKey(operand)) {
                spillOperand(operand, regAssignments);
                regAssignments.put(operand, -1);
            }
            handleSpill(operand);
        }
    }

    static class InstructionDetailsPrinter {
        public static void print(AsmInstr instr) {
            System.out.println("Instruction: " + instr.toString());
        }
    }

    static class RegisterUsageAnalyzer {
        public static void analyze(LinearScanAllocator allocator) {
            Map<AsmOperand, Integer> regAssignments = allocator.getSpillValue();
            regAssignments.forEach((operand, reg) -> {
                if (reg != -1) {
                    System.out.println("Operand " + operand + " is assigned to register " + reg);
                } else {
                    System.out.println("Operand " + operand + " was spilled");
                }
            });
        }
    }

    static class OperandDetailsPrinter {
        public static void print(AsmOperand operand) {
            System.out.println("Operand ID: " + operand.getColor());
            System.out.println("Operand Type: " + operand.getSpillPlace());
            if (operand instanceof AsmReg) {
                System.out.println("Assigned Register: " + operand.getSpillPlace());
            }
        }
    }
}
