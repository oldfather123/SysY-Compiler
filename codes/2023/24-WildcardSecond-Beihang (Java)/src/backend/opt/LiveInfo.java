package backend.opt;

import backend.component.RiscvBlock;
import backend.component.RiscvFunction;
import backend.component.RiscvInstr;
import backend.operand.RiscvOperand;
import backend.operand.RiscvReg;
import backend.operand.RiscvVirReg;
import tools.DoubleNode;

import java.util.LinkedHashMap;
import java.util.LinkedHashSet;

public class LiveInfo {
    private final LinkedHashSet<RiscvReg> liveUse = new LinkedHashSet<>();
    private final LinkedHashSet<RiscvReg> liveDef = new LinkedHashSet<>();
    private LinkedHashSet<RiscvReg> liveIn = new LinkedHashSet<>();
    private LinkedHashSet<RiscvReg> liveOut = new LinkedHashSet<>();

    public LinkedHashSet<RiscvReg> getLiveDef() {
        return liveDef;
    }

    public LinkedHashSet<RiscvReg> getLiveIn() {
        return liveIn;
    }

    public LinkedHashSet<RiscvReg> getLiveOut() {
        return liveOut;
    }

    public void setLiveOut(LinkedHashSet<RiscvReg> liveOut) {
        this.liveOut = liveOut;
    }

    public LinkedHashSet<RiscvReg> getLiveUse() {
        return liveUse;
    }

    public static LinkedHashMap<RiscvBlock, LiveInfo> liveInfoAnalysis(RiscvFunction function) {
        //分析liveUse liveDef
        LinkedHashMap<RiscvBlock, LiveInfo> liveInfoHashMap = new LinkedHashMap<>();
        for(RiscvBlock block: function.getBlocks()) {
            LiveInfo liveInfo = new LiveInfo();
            if(block.getRiscvInstrs().isEmpty()) {
                continue;
            }
            liveInfoHashMap.put(block, liveInfo);
            for(DoubleNode<RiscvInstr> insNode = block.getRiscvInstrs().getHead();
                    insNode != null; insNode = insNode.getSucc()) {
                RiscvInstr ins = insNode.getContent();
                for(RiscvOperand op:ins.getOperands()) {
                    if(op instanceof RiscvReg) {
                        if(!liveInfo.getLiveDef().contains((RiscvReg) op)) {
                            liveInfo.getLiveUse().add((RiscvReg) op);
                        }
                    }
                }
                if(ins.getDefReg() != null) {
                    liveInfo.getLiveDef().add(ins.getDefReg());
                }
            }
            liveInfo.liveIn.addAll(liveInfo.liveUse);
        }
        //计算LiveIn 计算LiveOut
        Boolean changed = true;
        while(changed) {
            changed = false;
            for(int i = function.getBlocks().size() - 1;i >= 0;i--) {
                RiscvBlock block = function.getBlocks().get(i);
                LinkedHashSet<RiscvReg> newLiveOut = new LinkedHashSet<>();

                LiveInfo oldInfo = liveInfoHashMap.get(block);

                for(var succs: block.getSuccs()) {
                    assert liveInfoHashMap.containsKey(succs);
                    newLiveOut.addAll(liveInfoHashMap.get(succs).getLiveIn());
                }
                LinkedHashSet<RiscvReg> livein = oldInfo.getLiveIn();
                livein.clear();
                livein.addAll(newLiveOut);
                livein.removeAll(oldInfo.getLiveDef());
                livein.addAll(oldInfo.getLiveUse());
                if(!oldInfo.getLiveOut().equals(newLiveOut)) {
                    changed = true;
                    oldInfo.setLiveOut(newLiveOut);
                }
            }
        }
        return liveInfoHashMap;
    }
}
