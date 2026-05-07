package mid.IntermediatePresentation;

import mid.IntermediatePresentation.Function.Function;
import mid.IntermediatePresentation.Function.LibFunction;
import mid.IntermediatePresentation.Instruction.Instruction;

import java.util.LinkedList;

public class IRManager {
    private boolean autoInsert = true;
    private static final IRManager INSTANCE = new IRManager();
    private static final Module MODULE = new Module();

    private int curLocalVarCounter = 0;
    private int curGlobalVarCounter = 0;
    private int tempVarCounter = 0;
    private int blockCounter = 0;
    private int paramCounter = 0;

    private int stringCounter = 0;

    private BasicBlock curBlock = null;

    private Function curFunction = null;

    private final LinkedList<BasicBlock> breakToStack = new LinkedList<>();
    private final LinkedList<BasicBlock> continueToStack = new LinkedList<>();

    public static IRManager getInstance() {
        return INSTANCE;
    }

    private IRManager() {
    }

    public static Module getModule() {
        return MODULE;
    }

    public void setAutoInsert(boolean autoInsert) {
        this.autoInsert = autoInsert;
    }

    public void setCurBlock(BasicBlock curBlock) {
        this.curBlock = curBlock;
    }

    public BasicBlock getCurBlock() {
        return curBlock;
    }

    public void setCurFunction(Function curFunction) {
        this.curFunction = curFunction;
    }

    public Function getCurFunction() {
        return curFunction;
    }

    public void addBBToCurFunction(BasicBlock bb) {
        if (autoInsert) {
            curFunction.addBlock(bb);
        }
    }

    public void instrCreated(Instruction instruction) {
        if (curBlock != null && autoInsert) {
            curBlock.addInstruction(instruction);
        }
    }

    public void addBreakTo(BasicBlock breakTo) {
        breakToStack.push(breakTo);
    }

    public void addContinueTo(BasicBlock continueTo) {
        continueToStack.push(continueTo);
    }

    public BasicBlock getBreakTo() {
        return breakToStack.get(0);
    }

    public BasicBlock getContinueTo() {
        return continueToStack.get(0);
    }

    public void exitCycle() {
        if (!breakToStack.isEmpty()) {
            breakToStack.pop();
        }

        if (!continueToStack.isEmpty()) {
            continueToStack.pop();
        }
    }

    public boolean inGlobalDecl() {
        return curBlock == null;
    }

    public void resetBlockCount() {
        if (blockCounter > 0) {
            blockCounter = 0;
        }
        curBlock = null;
    }


    public String declareVar() {
        String reg;
        if (curBlock != null || !autoInsert) {
            String localVarPrefix = "%local_";
            reg = localVarPrefix + curLocalVarCounter;
            curLocalVarCounter++;
        } else {
            //说明还没有开始函数声明，现在要声明全局变量
            String globalVarPrefix = "@global_";
            reg = globalVarPrefix + curGlobalVarCounter;
            curGlobalVarCounter++;
        }
        return reg;
    }

    public String declareTempVar() {
        String tempVarPrefix = "%temp_";
        String reg = tempVarPrefix + tempVarCounter;
        tempVarCounter++;
        return reg;
    }

    public String declareGlobalVar() {
        String tempVarPrefix = "@global_";
        String reg = tempVarPrefix + curGlobalVarCounter;
        curGlobalVarCounter++;
        return reg;
    }

    public String declareBlock() {
        String blockPrefix = "b";
        if (!autoInsert) {
            blockPrefix = "midB";
        }
        String bb = blockPrefix + blockCounter;
        blockCounter++;
        return bb;
    }

    public String declareParam() {
        String paramPrefix = "%param_";
        String param = paramPrefix + paramCounter;
        paramCounter++;
        return param;
    }

    public String declareString() {
        String str = "@str_" + stringCounter;
        stringCounter++;
        return str;
    }

    public String declareLocalVar() {
        String str = "%local_" + curLocalVarCounter;
        curLocalVarCounter++;
        return str;
    }

    public boolean autoInsert() {
        return autoInsert;
    }
}
