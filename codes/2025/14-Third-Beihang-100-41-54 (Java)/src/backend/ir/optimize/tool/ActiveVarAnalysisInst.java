package backend.ir.optimize.tool;

import backend.ir.entity.BasicBlock;
import backend.ir.entity.Value;
import backend.ir.entity.insts.Inst;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.*;

/**
 * 针对语句的活跃变量分析，隶属于一个基本块中
 */
public class ActiveVarAnalysisInst {
    private final BasicBlock basicBlock;
    private final Map<Inst, Set<Value>> use;
    private final Map<Inst, Set<Value>> def;
    private final Map<Inst, Set<Value>> in;
    private final Map<Inst, Set<Value>> out;

    public Set<Value> getUseSet(Inst inst){
        return use.computeIfAbsent(inst, k -> new HashSet<>());
    }
    public Set<Value> getDefSet(Inst inst){
        return def.computeIfAbsent(inst, k -> new HashSet<>());
    }
    public Set<Value> getInSet(Inst inst){
        return in.computeIfAbsent(inst, k -> new HashSet<>());
    }
    public Set<Value> getOutSet(Inst inst){
        return out.computeIfAbsent(inst, k -> new HashSet<>());
    }

    public  ActiveVarAnalysisInst(BasicBlock basicBlock) {
        this.basicBlock = basicBlock;
        use = new HashMap<>();
        def = new HashMap<>();
        in = new HashMap<>();
        out = new HashMap<>();
    }

    /**
     * 初始化每条指令的 use 和 def 集合
     */
    private void generateUseDefForInsts() {
        List<Inst> insts = basicBlock.getInstructions();
        for (Inst inst : insts) {
            // Use: 指令中读取的变量（操作数）
            Set<Value> useSet = new HashSet<>();
            for(Value value: inst.getUsees()){
                if(value instanceof Inst inst1 && inst1.isUseful()){
                    useSet.add(value);
                }
            }
            use.put(inst, useSet);

            // Def: 指令定义的变量（结果）
            Set<Value> defSet = new HashSet<>();
            if(inst.isUseful()) defSet.add(inst);
            def.put(inst, defSet);
        }
    }

    /**
     * 计算每条指令的 in 和 out 集合
     * 从基本块的 LiveOut 开始反向遍历
     */
    public void analyze() {
        generateUseDefForInsts(); // 初始化 use 和 def
        List<Inst> insts = basicBlock.getInstructions();

        // 初始时，最后一条指令的 out 是基本块的 LiveOut
        Set<Value> currentLive = new HashSet<>(basicBlock.getOutSet());

        // 反向遍历指令
        for (int i = insts.size() - 1; i >= 0; i--) {
            Inst inst = insts.get(i);

            // 当前指令的 out 集合是 currentLive
            out.put(inst, new HashSet<>(currentLive));

            // 计算 in = (out - def) ∪ use
            Set<Value> inSet = new HashSet<>(currentLive);
            inSet.removeAll(getDefSet(inst)); // out - def
            inSet.addAll(getUseSet(inst));   // ∪ use
            in.put(inst, inSet);

            // 更新 currentLive 为当前指令的 in（用于下一条指令）
            currentLive = new HashSet<>(inSet);
        }
    }

    /**
     * debug使用
     */
    public void printActiveVarAnalysisInst(BufferedWriter fout) throws IOException {
        List<Inst> insts = basicBlock.getInstructions();
        for(Inst inst : insts){
            fout.write("指令: ");
            inst.printAssembly(fout);
            fout.write("\n\t\t\t\t");
            fout.write("use集合: \n\t\t\t\t");
            for(Value usee: inst.getUseSet()){
                fout.write("\t");
                usee.printAssembly(fout);
                fout.write("\n\t\t\t\t");
            }
            fout.write("def集合: \n\t\t\t\t");
            for(Value usee: inst.getDefSet()){
                fout.write("\t");
                usee.printAssembly(fout);
                fout.write("\n\t\t\t\t");
            }
            fout.write("in集合: \n\t\t\t\t");
            for(Value usee: inst.getInSet()){
                fout.write("\t");
                usee.printAssembly(fout);
                fout.write("\n\t\t\t\t");
            }
            fout.write("out集合: \n\t\t\t\t");
            for(Value usee: inst.getOutSet()){
                fout.write("\t");
                usee.printAssembly(fout);
                fout.write("\n\t\t\t\t");
            }
            fout.write("\n\t\t\t");
        }
        fout.write("\n\t\t");
    }
}
