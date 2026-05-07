package ir.pass.analyze;

import ir.value.BasicBlock;

import java.util.ArrayList;

public class Loop {
    public ArrayList<BasicBlock> blocks = new ArrayList<BasicBlock>();
    public Loop father;
    public ArrayList<Loop> children = new ArrayList<Loop>();
    public int level;
    public long times;
    public void updateLevel(){
        this.level = 1;
        Loop tmp = this;
        while(tmp.father!=null){
            this.level++;
            tmp = tmp.father;
        }
        for(var block : blocks){
            block.setLoopDepth(this.level);
        }
    }

    public Loop(){
        times = 10;
    }
}
