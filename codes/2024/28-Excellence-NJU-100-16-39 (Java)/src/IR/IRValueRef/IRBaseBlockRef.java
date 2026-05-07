package IR.IRValueRef;

import IR.IRConst;
import IR.IRInstruction.IRInstruction;
import IR.IRType.IRType;

import java.util.ArrayList;
import java.util.List;

public class IRBaseBlockRef implements IRValueRef{
    /*
     * 该类是基本块的引用，用于表示基本块
     * 例如mainEntry:
     */
    public static int baseBlockCounter = 0;
    public final int baseBlockId;
    private final StringBuilder IRCodeBuilder;/*用于存储该基本块的IR代码*/
    private final String label;
    private List<IRInstruction> instructionList;
    private IRFunctionBlockRef functionBlockRef;/*该基本块所属的函数块*/
    private List<IRValueRef> predList = new ArrayList<>();/*该基本块的前驱基本块，用于指明控制流*/
    private List<IRValueRef> succList = new ArrayList<>();/*该基本块的后继基本块，用于指明控制流*/
    public IRBaseBlockRef(String label) {
        this.IRCodeBuilder = new StringBuilder();
        this.label = label;
        this.baseBlockId = baseBlockCounter++;
        this.instructionList = new ArrayList<>();
    }
    public IRFunctionBlockRef setFunctionBlockRef(IRFunctionBlockRef functionBlockRef) {
        this.functionBlockRef = functionBlockRef;
        return this.functionBlockRef;
    }
    public IRFunctionBlockRef getFunctionBlockRef() {
        return functionBlockRef;
    }

    /** -------- static methods --------*/
    public static IRBaseBlockRef IRAppendBasicBlock(IRFunctionBlockRef function, String label) {
        /*在函数块中的基本块列表中添加一个新的基本块*/
        IRBaseBlockRef baseBlock = new IRBaseBlockRef(label);
        function.addBaseBlockRef(baseBlock);
        baseBlock.setFunctionBlockRef(function);
        return baseBlock;
    }
    public void addPredBaseBlock(IRValueRef pred){
        predList.add(pred);
    }

    public List<IRValueRef> getPredList(){return predList;}
    public List<IRValueRef> getSuccList(){return succList;}

    public void addSuccBaseBlock(IRValueRef succ){
        succList.add(succ);
    }
    public void appendInstr(IRInstruction instr) {
        instructionList.add(instr);
    }
    public void addInstructionAtStart(IRInstruction instruction) {
        instructionList.add(0, instruction);
    }

    @Override
    public String getText(){
        return label + baseBlockId;
    }
    @Override
    public IRType getType() {
        return null;
    }
    @Override
    public int getTypeKind() {
        return IRConst.IRBasicBlockValueKind;
    }
    public void appendln(String s) {
        IRCodeBuilder.append("  ").append(s).append("\n");
    }
    public void appendln(String s, int align) {
        IRCodeBuilder.append("  ").append(s).append(", align ").append(align).append("\n");
    }
    public void appendlnAtStart(String s){
        IRCodeBuilder.insert(0, '\n');
        IRCodeBuilder.insert(0, s);
        IRCodeBuilder.insert(0, "  ");
    }
    public String getLabel() {
        return label + baseBlockId;
    }
    public StringBuilder getCodeBuilder() {
        return IRCodeBuilder;
    }
    public List<IRInstruction> getInstructionList() {
        return instructionList;
    }

    public static IRInstruction IRGetFirstInstruction(IRBaseBlockRef baseBlockRef) {
        return baseBlockRef.getInstructionList().get(0);
    }

    public static IRInstruction IRGetNextInstruction(IRBaseBlockRef baseBlockRef, IRInstruction irInstruction) {
        List<IRInstruction> instructions = baseBlockRef.getInstructionList();
        for (IRInstruction instruction : instructions) {
            if (irInstruction.equals(instruction)) {
                int index = instructions.indexOf(instruction);
                if (index == instructions.size() - 1)
                    return null;
                else
                    return instructions.get(index + 1);
            }
        }
        return null;
    }
}