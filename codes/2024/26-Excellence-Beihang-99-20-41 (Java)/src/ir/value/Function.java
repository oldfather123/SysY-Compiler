package ir.value;

import frontend.ast.AbstractSyntaxTree;
import ir.IrInstr;
import ir.type.IrType;
import pass.utils.ControlFlowGraph;
import pass.utils.DominatorTree;
import pass.utils.LoopInfo;
import utils.GlobalCounter;
import utils.RefCell;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;

public class Function extends Value {
    private final List<Argument> arguments;
    private AbstractSyntaxTree.Block body;  // Temporary field before the implementation of IRBuilder
    private final List<BasicBlock> basicBlocks;
    private boolean isLibFunc = false;
    private BasicBlock Exit;

    public Function(String name, IrType returnType) {
        super("@" + name, returnType);
        this.arguments = new LinkedList<>();
        this.body = null;
        this.basicBlocks = new LinkedList<>();
    }

    public void setBody(AbstractSyntaxTree.Block body) {
        this.body = body;
    }

    public AbstractSyntaxTree.Block getBody() {
        return body;
    }

    public void addArgument(Argument arg) {
        arguments.add(arg);
    }

    public List<Argument> getArguments() {
        return arguments;
    }

    public void addBasicBlock(BasicBlock block) {
        basicBlocks.add(block);
    }

    public void setBbExit(BasicBlock b) {
        this.Exit=b;
    }

    public BasicBlock getBbExit() {
        return this.Exit;
    }

    public String addBasicBlock() {
        String id = GlobalCounter.gen("b").toString();
        basicBlocks.add(new BasicBlock(id));
        return id;
    }

    public void insertBasicBlock(String blockId, BasicBlock block) {
        for (int i = 0; i < basicBlocks.size(); i++) {
            if (basicBlocks.get(i).getName().equals(blockId)) {
                basicBlocks.add(i + 1, block);
                return;
            }
        }
    }

    public void insertBasicBlocks(String blockId, List<BasicBlock> blocks) {
        for (int i = 0; i < basicBlocks.size(); i++) {
            if (basicBlocks.get(i).getName().equals(blockId)) {
                basicBlocks.addAll(i + 1, blocks);
                return;
            }
        }
    }

    public void insertBlockInstr(String blockId, IrInstr instr) {
        for (BasicBlock block : basicBlocks) {
            if (block.getName().equals(blockId)) {
                block.insertTailInstr(instr);
                return;
            }
        }
    }

    public boolean hasTerminator(String blockId) {
        for (BasicBlock block : basicBlocks) {
            if (block.getName().equals(blockId)) {
                return block.hasTerminator();
            }
        }
        return false;
    }

    public String getReturnType() {
        return getType().toString();
    }

    public IrType getReturnTypeType() {
        return getType();
    }

    public BasicBlock getBlock(String blockId) {
        for (BasicBlock block : basicBlocks) {
            if (block.getName().equals(blockId)) {
                return block;
            }
        }
        return null;
    }

    public LinkedList<BasicBlock> getBasicBlocks() {
        return new LinkedList<>(basicBlocks);
    }

    public ArrayList<String> getBasicBlocksName() {
        ArrayList<String> res = new ArrayList<>();
        for (BasicBlock block : basicBlocks) {
            res.add(block.getName());
        }
        return res;
    }

    public String getArgumentsString(boolean withIdent) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < arguments.size(); i++) {
            sb.append(arguments.get(i).getType());
            if (withIdent) {
                sb.append(" ").append(arguments.get(i).getName());
            }
            if (i < arguments.size() - 1) {
                sb.append(", ");
            }
        }
        return sb.toString();
    }

    public String getStatementString() {
        StringBuilder sb = new StringBuilder();
        for (BasicBlock block : basicBlocks) {
            sb.append(block.getName()).append(":\n");
            for (IrInstr instr : block.getInstrs()) {
                sb.append("\t").append(instr).append("\n");
            }
        }
        return sb.toString();
    }

    public void resetBasicBlock(ArrayList<BasicBlock> blocks) {
        basicBlocks.clear();
        basicBlocks.addAll(blocks);
    }

    public boolean isLibFunction(){
        return isLibFunc;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder("fn ");
        sb.append(super.toString());
        sb.append("(");
        for (int i = 0; i < arguments.size(); i++) {
            sb.append(arguments.get(i));
            if (i < arguments.size() - 1) {
                sb.append(", ");
            }
        }
        sb.append("): Block of <");
        if (body == null) {
            sb.append("lib");
        } else {
            sb.append(body.hashCode());
        }
        sb.append(">");
        return sb.toString();
    }

    private final RefCell<ControlFlowGraph> controlFlowGraph = new RefCell<>(null);
    private final RefCell<DominatorTree> domTree = new RefCell<>(null);
    private final RefCell<HashMap<BasicBlock, LoopInfo>> loopInfoMap = new RefCell<>(null);
    private final RefCell<List<LoopInfo>> loops = new RefCell<>(null);
    private final RefCell<List<LoopInfo>> allLoops = new RefCell<>(null);

    public void setCFG(ControlFlowGraph cfg) {
        controlFlowGraph.set(cfg);
    }

    public void setDomTree(DominatorTree domTree) {
        this.domTree.set(domTree);
    }

    public void invalidateDomTree() {
        controlFlowGraph.set(null);
        domTree.set(null);
    }

    public void invalidateLoopInfo() {
        loopInfoMap.set(null);
        loops.set(null);
        allLoops.set(null);
    }

    public ControlFlowGraph getCFG() {
        return controlFlowGraph.getNotNull();
    }

    public DominatorTree getDomTree() {
        return domTree.getNotNull();
    }

    public void setLoopInfoMap(HashMap<BasicBlock, LoopInfo> loopInfoMap) {
        this.loopInfoMap.set(loopInfoMap);
    }

    public HashMap<BasicBlock, LoopInfo> getLoopInfoMap() {
        return loopInfoMap.getNotNull();
    }

    public void setLoops(List<LoopInfo> loops) {
        this.loops.set(loops);
    }

    public List<LoopInfo> getLoops() {
        return loops.getNotNull();
    }

    public void setAllLoops(List<LoopInfo> allLoops) {
        this.allLoops.set(allLoops);
    }

    public List<LoopInfo> getAllLoops() {
        return allLoops.getNotNull();
    }
}
