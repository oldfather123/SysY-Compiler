package pass.ir;

import ir.IrInstr;
import ir.IrModule;
import ir.instr.LoadInstr;
import ir.instr.StoreInstr;
import ir.instr.TermInstr;
import ir.type.PointerType;
import ir.value.BasicBlock;
import ir.value.Function;
import ir.value.Value;
import pass.Pass;
import pass.utils.DataStreamInstrMap;
import pass.utils.DominatorMap;
import utils.AliasContainer;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;

import static pass.utils.DataStreamInstrMap.initialize;

//激进的死代码分析
public class ADCE implements Pass.IrPass{
    public HashMap<String, ArrayList<String>> dominatorMap = new HashMap<>();
    public final HashMap<Integer, String> instrToBlock = new HashMap<>();
    public final HashMap<String, IrInstr> valueToInstr = new HashMap<>();
    public final HashMap<String, BasicBlock> blockMap = new HashMap<>();
    public final HashSet<Integer> aliveInstrs = new HashSet<>();
    public final HashSet<String> aliveBlocks = new HashSet<>();
    public final HashSet<String> jumpDependentBlocks = new HashSet<>();
    public final AliasContainer aliasContainer = AliasContainer.getInstance();
    public HashSet<String> usefulPointers = new HashSet<>();

    @Override
    public void run(IrModule module) {
        ArrayList<Function> funcs = module.getFunctions();
        usefulPointers.addAll(aliasContainer.globalBasicPointers);
        for (Function func : funcs) {
            tackleFunc(func);
        }
    }

    public void tackleFunc(Function func) {
        LinkedList<BasicBlock> blocks = func.getBasicBlocks();

        initialize(instrToBlock, valueToInstr,blocks,blockMap);
//        HashMap<Integer, ArrayList<Integer>> dataStreamMap =
//                DataStreamInstrMap.getDataStreamMap(func, instrToBlock, valueToInstr, blockMap);
        dominatorMap = DominatorMap.getDominatorTree(func,new HashMap<>(), new HashMap<>());
        trackPassOne(blocks);
        //pass2在pass1里循环调用
        int flag;
        do {
            flag = usefulPointers.size();
            trackPassThree(blocks);
        }while (flag != usefulPointers.size());
        trackPassFour(blocks,func);
    }

    private void trackPassOne(LinkedList<BasicBlock> blocks) {
        for (BasicBlock block : blocks) {
            LinkedList<IrInstr> instrs = block.getInstrs();
            for (IrInstr instr : instrs) {
                if ((instr instanceof TermInstr && ((TermInstr) instr).op.equals(IrInstr.OpCode.RET)) ||
                        (instr instanceof TermInstr && ((TermInstr) instr).op.equals(IrInstr.OpCode.CALL)) ||
                        (instr instanceof StoreInstr)){
                    if (instr instanceof StoreInstr) {
                        if (aliasContainer.ifReady) {
                            if (aliasContainer.globalBasicPointers.contains(
                                    aliasContainer.getPointerName(((StoreInstr) instr).pointer.getName()))) {
                                trackInstr(instr);
                                do {
                                    trackPassTwo();
                                } while (!jumpDependentBlocks.isEmpty());
                            }
                        }
                        else {
                            trackInstr(instr);
                            do {
                                trackPassTwo();
                            } while (!jumpDependentBlocks.isEmpty());
                        }
                    }
                    else {
                        trackInstr(instr);
                        do {
                            trackPassTwo();
                        } while (!jumpDependentBlocks.isEmpty());
                    }
                }
            }
        }
    }

    private void trackPassTwo() {
        // copy a temp set to avoid concurrent modification
        HashSet <String> temp = new HashSet<>(jumpDependentBlocks);
        for (String name : temp) {
            BasicBlock block = blockMap.get(name);
            IrInstr instr = block.getTerminator();
            trackInstr(instr);
        }
        // delete the temp set
        for (String name : temp) {
            jumpDependentBlocks.remove(name);
        }
    }

    private void trackPassThree(LinkedList<BasicBlock> blocks) {
        for (BasicBlock block : blocks) {
            LinkedList<IrInstr> instrs = block.getInstrs();
            for (IrInstr instr : instrs) {
                if (aliasContainer.ifReady) {
                    if (instr instanceof LoadInstr) {
                        if (usefulPointers.contains(aliasContainer.getPointerName((((LoadInstr) instr)).pointer.getName()))) {
                            trackInstr(instr);
                            do {
                                trackPassTwo();
                            } while (!jumpDependentBlocks.isEmpty());
                        }
                    }
                    else if (instr instanceof StoreInstr) {
                        if (usefulPointers.contains(aliasContainer.getPointerName(((StoreInstr) instr).pointer.getName()))) {
                            trackInstr(instr);
                            do {
                                trackPassTwo();
                            } while (!jumpDependentBlocks.isEmpty());
                        }
                    }
                }
            }
        }
    }

    private void trackPassFour(LinkedList<BasicBlock> blocks, Function func) {
        ArrayList<BasicBlock> resetBlocks = new ArrayList<>();
        HashSet<String> deadBlocks = new HashSet<>();
        for (BasicBlock block : blocks) {
            if (!aliveBlocks.contains(block.getName())) {
                deadBlocks.add(block.getName());
            }
        }
//        IrInstr returnInstr = new TermInstr()
        for (BasicBlock block : blocks) {
            if (!aliveBlocks.contains(block.getName())) {
                continue;
            }
            resetBlocks.add(block);
            LinkedList<IrInstr> instrs = block.getInstrs();
            ArrayList<IrInstr> resetInstrs = new ArrayList<>();
            for (IrInstr instr : instrs) {
                if (aliveInstrs.contains(instr.getGlobalInstrId())) {
                    resetInstrs.add(instr);
                }
            }
            IrInstr terminator = block.getTerminator();
            if (terminator != null && !aliveInstrs.contains(terminator.getGlobalInstrId())) {
                IrInstr ans = ((TermInstr)terminator).resetBrAfterResetBlock(deadBlocks);
                if (ans != null) {
                    resetInstrs.add(ans);
                }
            }
            block.resetInstrs(resetInstrs);
        }
        func.resetBasicBlock(resetBlocks);
    }

    public void trackInstr(IrInstr instr) {
        if (aliveInstrs.contains(instr.getGlobalInstrId())) {
            return;
        }
        if (instr instanceof LoadInstr) {
            usefulPointers.add(aliasContainer.getPointerName(((LoadInstr) instr).pointer.getName()));
        }
        if (instr instanceof TermInstr && ((TermInstr) instr).op.equals(IrInstr.OpCode.CALL)) {
            for (Value val : instr.getDependentValues()) {
                if (val == null)
                    continue;
                if (val.getType() instanceof PointerType) {
                    usefulPointers.add(aliasContainer.getPointerName(val.getName()));
                }
            }
        }
        toolAddJumpDependentBlocks(instrToBlock.get(instr.getGlobalInstrId()),new HashSet<>());
        aliveBlocks.add(instrToBlock.get(instr.getGlobalInstrId()));
        aliveInstrs.add(instr.getGlobalInstrId());
        if (instr.getDependentValues() != null) {
            for (Value val : instr.getDependentValues()) {
                if (val == null)
                    continue;
                if (valueToInstr.containsKey(val.getName())) {
                    trackInstr(valueToInstr.get(val.getName()));
                }
            }
        }
    }

    private void toolAddJumpDependentBlocks(String name,HashSet<String> visit) {
        visit.add(name);
        if (dominatorMap.containsKey(name)) {
            if (dominatorMap.get(name) == null)
                return;
            if (aliveBlocks.contains(name))
                return;
            for (String block : dominatorMap.get(name)) {
                if (visit.contains(block))
                    return;
                jumpDependentBlocks.add(block);
                toolAddJumpDependentBlocks(block,visit);
            }
        }
    }

    @Override
    public String getName() {
        return "ADCE";
    }
}
