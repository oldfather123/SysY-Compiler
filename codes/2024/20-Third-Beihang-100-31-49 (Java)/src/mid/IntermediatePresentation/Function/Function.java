package mid.IntermediatePresentation.Function;

import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.Instruction.Alloca;
import mid.IntermediatePresentation.Instruction.Instruction;
import mid.IntermediatePresentation.User;
import mid.IntermediatePresentation.Value;
import mid.IntermediatePresentation.ValueType;

import java.util.ArrayList;
import java.util.HashSet;

public class Function extends User {
    protected Param param;
    protected ArrayList<BasicBlock> bbs = new ArrayList<>();

    public Function(String name, Param param, ValueType type) {
        super(name, type);
        use(param);
        this.param = param;
        if (!name.equals("@main")) {
            IRManager.getModule().addFunction(this);
        }

        IRManager.getInstance().setCurFunction(this);
        // bb构造时会自动调用func.use(bb)
        new BasicBlock();
    }

    public Function(String name, ValueType type) {
        super(name, type);
        if (!name.equals("@main")) {
            IRManager.getModule().addFunction(this);
        }

        IRManager.getInstance().setCurFunction(this);
        new BasicBlock();
    }

    public void addBlock(BasicBlock bb) {
        bbs.add(bb);
        use(bb);
    }

    public void addBlockBefore(BasicBlock nextBlock, BasicBlock newBlock) {
        bbs.add(bbs.indexOf(nextBlock), newBlock);
        use(newBlock);
    }

    public void removeBlock(BasicBlock bb) {
        operandList.remove(bb);
        bbs.remove(bb);
    }

    public ArrayList<BasicBlock> getBlocks() {
        return bbs;
    }

    public BasicBlock getEntranceBlock() {
        return bbs.get(0);
    }

    public void addEntranceBlock(BasicBlock b) {
        bbs.add(0, b);
        HashSet<Alloca> allocas = new HashSet<>();
        for (Instruction instr : getEntranceBlock().getInstructionList()) {
            if (!(instr instanceof Alloca alloca)) {
                break;
            }
            allocas.add(alloca);
        }
        getEntranceBlock().getInstructionList().removeAll(allocas);
        allocas.forEach(b::addInstrAtEntry);
        use(b);
    }

    public boolean isVoid() {
        return vType == ValueType.NULL;
    }

    public void setParam(Param param) {
        this.param = param;
        use(param);
    }

    public Param getParam() {
        return param;
    }


    public String toString() {
        StringBuilder sb = new StringBuilder();
        String type = vType.toString();
        sb.append("define dso_local ").append(type).append(" ").append(reg);
        sb.append("(").append(param).append(")").append(" {\n");
        for (BasicBlock bb : bbs) {
            sb.append(bb);
        }
        sb.append("}\n\n");
        return sb.toString();
    }

    public void destroy() {
        for (BasicBlock b : bbs) {
            b.destroy();
        }
        bbs.clear();
        param.destroy();
    }

    public int instructionCount() {
        int cnt = 0;
        for (BasicBlock b : bbs) {
            cnt += b.getInstructionList().size();
        }
        return cnt;
    }


    public int intParamCnt() {
        int cnt = 0;
        for (Value v : param.getParams()) {
            if (v.getType() != ValueType.FLT) {
                cnt++;
            }
        }
        return cnt;
    }

    public int floatParamCnt() {
        int cnt = 0;
        for (Value v : param.getParams()) {
            if (v.getType() == ValueType.FLT) {
                cnt++;
            }
        }
        return cnt;
    }
}
