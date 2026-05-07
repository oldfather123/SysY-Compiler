package ir.pass.opt;

import ir.Value;
import ir.instr.Bitcast;
import ir.instr.Call;
import ir.instr.GetElementPtrInst;
import ir.instr.Global;
import ir.instr.Instr;
import ir.instr.Load;
import ir.instr.Store;
import ir.pass.analyze.DomAnalyzer;
import ir.pass.analyze.LoopAnalyzer;
import ir.value.Arg;
import ir.value.BasicBlock;
import ir.value.Module;
import ir.value.user.Function;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.LinkedList;

public class LSGCM {
    LinkedHashMap<BasicBlock, ArrayList<Instr>> block2mems;
    boolean changed;

    public boolean run(Module module) {
        DomAnalyzer.calDomForModule(module);
        LoopAnalyzer.analyzeLoop(module);
        module.functions.values().stream().filter(e -> !e.blocks.isEmpty()).forEach(
            this::runForFunction);
        return changed;
    }

    private boolean targetIsConflict(Instr i1, Instr i2, boolean isori, boolean checkPre) {
        Value i = i1.getOP(0);
        if(i instanceof Bitcast && ((Bitcast) i).crossCast){
            return true;
        }
        if (i instanceof Arg || i instanceof Global) {
            if (i2 instanceof Call) {
                if (((Call) i2).getFunction().blocks.isEmpty()) {
                    for (var ope : i2.getOperands()) {
                        if (ope instanceof Arg || ope instanceof GetElementPtrInst) {
                            if (ope instanceof GetElementPtrInst &&
                                ((GetElementPtrInst) ope).getOP(0).equals(i)) {
                                return true;
                            }
                            if (ope instanceof Arg && ope.equals(i)) {
                                return true;
                            }
                        }
                        if(ope instanceof Bitcast && ((Bitcast) ope).crossCast){
                            return true;
                        }
                    }
                    return false;
                } else {
                    return true;
                }
            } else if (i2 instanceof Store) {
                var ptr = ((Store) i2).getDest();
                if(ptr instanceof Bitcast){
                    return true;
                }
                if (ptr instanceof Arg || ptr instanceof Global) {
                    return i.equals(ptr);
                } else if(ptr instanceof GetElementPtrInst) {
                    if(((GetElementPtrInst) ptr).getOP(0) instanceof Bitcast){
                        return ((Bitcast) ((GetElementPtrInst) ptr).getOP(0)).crossCast;
                    }else {
                        return false;
                    }
                }else{
                        return false;
                }
            } else if (i2 instanceof Load) {
                if(!checkPre){
                    if (isori) {
                        return false;
                    }
                    if(i1 instanceof Load){
                        return false;
                    }
                }
                var ptr = ((Load) i2).getTarget();
                if (ptr instanceof Arg || ptr instanceof Global) {
                    return i.equals(ptr);
                } else {
                    return false;
                }
            }
        } else if (i instanceof GetElementPtrInst) {
            if (i2 instanceof Call) {
                if (((Call) i2).getFunction().blocks.isEmpty()) {
                    for (var ope : i2.getOperands()) {
                        if (ope instanceof Arg || ope instanceof GetElementPtrInst) {
                            if (ArrayEliminate.ptrConflit((GetElementPtrInst) i, ope)) {
                                return true;
                            }
                        }
                    }
                    return false;
                } else {
                    return true;
                }
            } else {
                if(!checkPre && !(i2 instanceof Store)){
                    if (isori) {
                        return false;
                    }
                    if(i1 instanceof Load){
                        return false;
                    }
                }
                assert (i2 instanceof Load || i2 instanceof Store);
                if (i2.getOP(0) instanceof Global || i2.getOP(0) instanceof Arg) {
                    return false;
                }
                return ArrayEliminate.mayConflict((GetElementPtrInst) i,
                    (GetElementPtrInst) i2.getOP(0));
            }
        }
        assert false;
        return false;
    }

    private void initMemsForFunc(Function function) {
        block2mems = new LinkedHashMap<>();
        for (BasicBlock block : function.blocks) {
            block2mems.put(block, new ArrayList<Instr>());
            for (Value inst : block.getInsts()) {
                if (inst instanceof Call || inst instanceof Store || inst instanceof Load) {
                    block2mems.get(block).add((Instr) inst);
                }
            }
        }
    }

    private boolean canLiftCheck(Instr instr, BasicBlock block) {
        BasicBlock idom = block.getIdominatedBy();
        int targetdepth = idom.getDomDepth();
        if (idom == null) {
            return false;
        }
        for (Value operand : instr.getOperands()) {
            if (operand instanceof Instr) {
                if (((Instr) operand).getParent().getDomDepth() > targetdepth) {
                    return false;
                }
            }
        }
        return true;
    }

    private LinkedHashSet<Instr> getDomConflict(BasicBlock block, Instr instr) {
        BasicBlock blockEarlier = block.getIdominatedBy();
        while(blockEarlier.getLoopDepth()>0 && blockEarlier.getIdominatedBy()!=null){
            blockEarlier = blockEarlier.getIdominatedBy();
        }

        LinkedHashSet<Instr> result = new LinkedHashSet<>();
        for (BasicBlock basicBlock : blockEarlier.getDominate()) {
            if (basicBlock.equals(blockEarlier)) {
                continue;
            }
            var mems = block2mems.get(basicBlock);
            for (Instr mem : mems) {
                if(mem.equals(instr)){
                    continue;
                }
                if (targetIsConflict(instr, mem,block.getDominate().contains(basicBlock), false)) {
                    result.add(mem);
                }
            }
        }
        return result;
    }

    public void runForFunction(Function function) {
        boolean needRepeat = true;
        while (needRepeat) {
            needRepeat = false;
            initMemsForFunc(function);
            for (var block : function.blocks) {
                if (block.getLoopDepth() > 0) {
                    ArrayList<Instr> thismems = block2mems.get(block);
                    for (int i = 0; i < thismems.size() && (!needRepeat); i++) {
                        Instr instr = thismems.get(i);
                        if (instr instanceof Call) {
                            continue;
                        }
                        boolean canTryLift = true;
                        for (int j = i - 1; j >= 0; j--) {
                            if (targetIsConflict(instr, thismems.get(j), false, false)) {
                                canTryLift = false;
                                break;
                            }
                        }
                        if (canTryLift) {
                            int loopthis = block.getLoopDepth();
                            Value ptr = instr.getOP(0);
                            BasicBlock blockEarly = block;
                            while (true) {
                                if (canLiftCheck(instr, blockEarly)) {
                                    LinkedHashSet<Instr> earlierConflict =
                                        getDomConflict(blockEarly, instr);
                                    if (earlierConflict.isEmpty()) {
                                        blockEarly = blockEarly.getIdominatedBy();
                                        if (blockEarly.getLoopDepth() < loopthis) {
                                            instr.getParent().getInsts().remove(instr);
                                            instr.setParent(blockEarly);
                                            blockEarly.getInsts()
                                                .add(blockEarly.getInsts().size() - 1, instr);
                                            needRepeat = true;
                                            changed = true;
                                            break;
                                        }
                                    } else {
                                        break;
                                    }
                                } else {
                                    break;
                                }
                            }
                        }
                    }
                }
                if (needRepeat) {
                    break;
                }
            }
        }
    }
}
