package backend.ir.optimize.tool;

import backend.ir.entity.BasicBlock;
import backend.ir.entity.Function;
import backend.ir.entity.Value;
import backend.ir.entity.insts.Inst;
import backend.ir.entity.insts.PHIInst;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.*;

/**
 * 活跃变量分析，按照课本的方法
 * 活跃变量关注的是基本块，和流图类似，所以还是以函数为单位
 * 为冲突图做准备
 */
public class ActiveVarAnalysis {
    private final Function function;
    private final Map<BasicBlock, Set<Value>> use;
    private final Map<BasicBlock, Set<Value>> def;
    private final Map<BasicBlock, Set<Value>> in;
    private final Map<BasicBlock, Set<Value>> out;
    public ActiveVarAnalysis(Function function) {
        this.function = function;
        use = new HashMap<>();
        def = new HashMap<>();
        in = new HashMap<>();
        out = new HashMap<>();
    }

    public Set<Value> getUseVar(BasicBlock block) {
        return use.computeIfAbsent(block, k -> new HashSet<>());
    }
    public Set<Value> getDefVar(BasicBlock block) {
        return def.computeIfAbsent(block, k -> new HashSet<>());
    }
    public Set<Value> getInVar(BasicBlock block) {
        return in.computeIfAbsent(block, k -> new HashSet<>());
    }
    public Set<Value> getOutVar(BasicBlock block) {
        return out.computeIfAbsent(block, k -> new HashSet<>());
    }

    /**
     * 遍历所有的基本块，生成use和def集合
     */
    public void generateUseDef(){
        use.clear();
        def.clear();
        List<BasicBlock> basicBlocks = function.getAllBasicBlocks();
        //遍历每个基本块的每个指令，生成即可
        for (BasicBlock basicBlock : basicBlocks) {
            List<Value> usees = basicBlock.getUsees();
            for (Value usee : usees) {
                if(usee instanceof PHIInst phiInst){
                    phiInst.addIntoUse();
                }
            }
            for(Value usee: usees){
                if(usee instanceof Inst inst){
                    inst.generateUseDef();
                }
            }
        }
    }

    /**
     * 活跃变量分析
     * 首先，最初的out均为空
     * in = use + (out-def)，集合运算
     * out是后继的in的并集
     * 当in不再发生变化时，退出循环
     */
    public void generateInOut(){
        in.clear();
        out.clear();
        List<BasicBlock> basicBlocks = function.getAllBasicBlocks();
        boolean inChange = true;//标记in集合是否改变
        while(inChange){
            inChange = false;
            //倒着遍历基本块
            for(int i=basicBlocks.size()-1;i>=0;i--){
                BasicBlock basicBlock = basicBlocks.get(i);
                Set<Value> outSet = new HashSet<>();
                //获取后继基本块，便于确定它们的in集合
                List<BasicBlock> outBlocks =  basicBlock.getOutBlocks();
                for(BasicBlock outBlock : outBlocks){
                    Set<Value> inSet = in.computeIfAbsent(outBlock, k -> new HashSet<>());
                    //求这些in集合的并集
                    outSet.addAll(inSet);
                }
                out.put(basicBlock, outSet);
                Set<Value> inSet2 = new HashSet<>();
                //按照公式，求in
                // 计算 out - def
                for(Value outNum: out.get(basicBlock)){
                    if(!def.get(basicBlock).contains(outNum)){
                        inSet2.add(outNum);
                    }
                }
                // 计算 in = use + (out - def)
                inSet2.addAll(use.get(basicBlock));
                //判断原来的in和新生成的in是否一致，如果不一致，标记为true
                if(!inSet2.equals(in.get(basicBlock))){
                    inChange = true;
                }
                in.put(basicBlock, inSet2);
            }
        }
    }

    /**
     * debug使用
     */
    public void printActiveVarAnalysis(BufferedWriter fout) throws IOException {
        List<BasicBlock> basicBlocks = function.getAllBasicBlocks();
        for(BasicBlock basicBlock : basicBlocks){
            fout.write("基本块: "+basicBlock.getName()+"\n\t\t\t");
            fout.write("use集合: \n\t\t\t");
            for(Value usee: basicBlock.getUseSet()){
                fout.write("\t");
                usee.printAssembly(fout);
                fout.write("\n\t\t\t");
            }
            fout.write("def集合: \n\t\t\t");
            for(Value usee: basicBlock.getDefSet()){
                fout.write("\t");
                usee.printAssembly(fout);
                fout.write("\n\t\t\t");
            }
            fout.write("in集合: \n\t\t\t");
            for(Value usee: basicBlock.getInSet()){
                fout.write("\t");
                usee.printAssembly(fout);
                fout.write("\n\t\t\t");
            }
            fout.write("out集合: \n\t\t\t");
            for(Value usee: basicBlock.getOutSet()){
                fout.write("\t");
                usee.printAssembly(fout);
                fout.write("\n\t\t\t");
            }
            fout.write("\n\t\t");
        }
        fout.write("\n\t");
    }
}
