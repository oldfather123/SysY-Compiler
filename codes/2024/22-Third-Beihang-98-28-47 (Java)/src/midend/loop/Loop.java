package midend.loop;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.binop.BinaryOperation;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.instr.otherop.cmp.Cmp;
import frontend.ir.structure.BasicBlock;

import java.util.ArrayList;
import java.util.Collections;

public class Loop {
    private BasicBlock preHeader; //进入循环的节点，有边指向header
    private BasicBlock preCond;
    private BasicBlock header; //循环入口，支配所有循环的节点，回边指向的节点
    private ArrayList<BasicBlock> exits = new ArrayList<>(); //循环跳出后的节点，endBlk & retBlk
    private ArrayList<BasicBlock> exitings = new ArrayList<>(); //具有指向exit块的边的blk
    private ArrayList<BasicBlock> latchs = new ArrayList<>(); //跳回header的blk
    private ArrayList<BasicBlock> blks = new ArrayList<>();
    private ArrayList<Loop> innerLoops = new ArrayList<>();
    private ArrayList<BasicBlock> sameLoopDepth = new ArrayList<>();
    private Loop prtLoop;

    private Value var;
    private Value begin;
    private Value end;
    private Value step;
    private Value cond;
    private BinaryOperation alu;

    public BinaryOperation getAlu() {
        return alu;
    }

    public Value getVar() {
        return var;
    }

    public Value getEnd() {
        return end;
    }

    public Value getBegin() {
        return begin;
    }

    public Value getStep() {
        return step;
    }

    public Cmp getCond() {
        return (Cmp) cond;
    }

    private boolean hasIndVar;

    public Loop(BasicBlock header) {
        this.header = header;
        this.addBlk(header);
        prtLoop = null;
        hasIndVar = false;
    }

    public void setPreCond(BasicBlock preCond) {
        this.preCond = preCond;
    }

    public BasicBlock getPreCond() {
        return preCond;
    }

    public void setPreHeader(BasicBlock preHeader) {
        this.preHeader = preHeader;
    }

    public BasicBlock getPreHeader() {
        return preHeader;
    }

    public void setSameLoopDepth(ArrayList<BasicBlock> sameLoopDepth) {
        this.sameLoopDepth = sameLoopDepth;
    }

    public ArrayList<BasicBlock> getSameLoopDepth() {
        return sameLoopDepth;
    }

    public BasicBlock getHeader() {
        return header;
    }

    public Loop getPrtLoop() {
        return prtLoop;
    }

    public void setPrtLoop(Loop prtLoop) {
        this.prtLoop = prtLoop;
    }

    public ArrayList<BasicBlock> getLatchs() {
        return latchs;
    }

    public boolean hasIndVar() {
        return hasIndVar;
    }

    public void addLoop(Loop inner) {
        if (innerLoops.contains(inner)) return;
        innerLoops.add(inner);
    }

    public void addBlk(BasicBlock blk) {
        //todo:为什么一个块会被加好几次？
        if (blks.contains(blk)) return;
        blks.add(blk);
    }

    public void addExitingBlk(BasicBlock blk) {
        if (exitings.contains(blk)) return;
        exitings.add(blk);
    }

    public void addLatchBlk(BasicBlock blk) {
        if (latchs.contains(blk)) return;
        latchs.add(blk);
    }

    public void addExitBlk(BasicBlock blk) {
        if (exits.contains(blk)) return;
        exits.add(blk);
    }

    public ArrayList<BasicBlock> getExits() {
        return exits;
    }

    public ArrayList<BasicBlock> getExitings() {
        return exitings;
    }

    public ArrayList<Loop> getInnerLoops() {
        return innerLoops;
    }

    public ArrayList<BasicBlock> getBlks() {
        return blks;
    }

    public void reverse() {
        blks.remove(header);
        Collections.reverse(blks);
        blks.add(0, header);
        Collections.reverse(innerLoops);
    }

    public boolean isSimpleLoop(){
        //header前一定是entering，还有一个最后跳转的latch
        if(header.getPres().size() != 2){
            return false;
        }
        if(latchs.size() != 1){
            return false;
        }
        if(exitings.size() != 1){
            return false;
        }
        BasicBlock latch = latchs.get(0);
        //短路求值
        for(BasicBlock exitingBlock : exitings){
            if(exitingBlock != latch){
                return false;
            }
        }
        return exits.size() == 1;
    }

    @Override
    public String toString() {
        return header.toString();
    }

    public void LoopPrint() {
        StringBuilder sb = new StringBuilder();
        sb.append("header: " + this.header + "\n");
        sb.append("entering: " + this.preHeader + "\n");
        sb.append("blks: " + this.blks + "\n");
        sb.append("latchs: " + this.latchs + "\n");
        sb.append("exitings: " + this.exitings + "\n");
        sb.append("exits: " + this.exits + "\n");
        sb.append("inners: " + this.innerLoops + "\n");
        sb.append("sameDepth: " + this.sameLoopDepth + "\n");
        if (prtLoop != null) {
            sb.append("prtLoop " + this.prtLoop + "\n");
        }
        if (hasIndVar) {
            sb.append("itVar: " + ((Instruction)var).print() + "\n");
            sb.append("itInit: " + begin + "\n");
            sb.append("itEnd: " + end + "\n");
            sb.append("itAlu: " + ((Instruction)alu).print() + "\n");
            sb.append("itStep: " + step + "\n");
            sb.append("cond: " + ((Instruction) cond).print() + "\n");
        }
        System.out.println(sb);
    }

    public void clearIndVar() {
        this.hasIndVar = false;
    }

    public void setIndVar(PhiInstr itVar, Value itInit, Value itEnd, Value itAlu, Value itStep, Value cond) {
        this.var = itVar;
        this.begin = itInit;
        this.end = itEnd;
        this.alu = (BinaryOperation) itAlu;
        this.step = itStep;
        this.cond = cond;
        hasIndVar = true;
    }

    public void colorBlk() {
        for (BasicBlock block : latchs) {
            block.setBlockType(BlockType.LATCH);
        }
        for (BasicBlock block : exits) {
            block.setBlockType(BlockType.LOOPEXIT);
        }
        if (exits.size() == 1) {
            for (BasicBlock blk : exits) {
                blk.setBlockType(BlockType.EXIT);
            }
        }
        for (BasicBlock block : exitings) {
            block.setBlockType(BlockType.EXITING);
        }
        for (BasicBlock block : blks) {
            block.setBlockType(BlockType.INLOOP);
        }
        header.setBlockType(BlockType.HEADER);
        if (preHeader != null) {
            preHeader.setBlockType(BlockType.PREHEADER);
        }
    }

}
