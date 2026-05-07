package backend.ir.entity;

import backend.ir.entity.insts.CallInst;
import backend.ir.entity.insts.Inst;
import backend.ir.optimize.dataStructure.CfgGraph;
import backend.ir.optimize.dataStructure.DominantTree;
import backend.ir.optimize.tool.ActiveVarAnalysis;
import backend.ir.optimize.tool.MemToReg;
import frontend.semantic.symbol.IRBasicType;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * &#064;Classname Function
 * &#064;Description  TODO
 * &#064;Date 2025/7/12 15:23
 * &#064;Created MuJue
 */
public class Function extends Value{
    private List<Argument> arguments =  new ArrayList<>();
    private BasicBlock superBlock;
    private final Program program;
    private final CfgGraph cfgGraph;
    private final DominantTree dominantTree;
    private final ActiveVarAnalysis activeVarAnalysis;

    public Program getProgram() {
        return program;
    }

    public Function(String name, IRBasicType IRBasicType, Program program){
        super(name, IRBasicType);
        this.program = program;
        cfgGraph = new CfgGraph(this);
        dominantTree = new DominantTree(this);
        activeVarAnalysis = new ActiveVarAnalysis(this);
    }
    public void addArgmument(Argument argument){
        arguments.add(argument);
    }

    public List<Argument> getArguments() {
        return arguments;
    }

    public void setArguments(List<Argument> arguments) {
        this.arguments = arguments;
    }

    public BasicBlock getSuperBlock() {
        return superBlock;
    }

    public void setSuperBlock(BasicBlock superBlock) {
        this.superBlock = superBlock;
    }

    @Override
    public void printAssembly(BufferedWriter fout) throws IOException {
        String btStr = getTypeStr();
        fout.write("define "+btStr + " @"+name+"(");
        int argumentSize = arguments.size();
        for(int i=0;i<argumentSize;i++){
            arguments.get(i).printAssembly(fout);
            if(i!=argumentSize-1){
                fout.write(", ");
            }
        }
        fout.write("){\n");
        superBlock.printAssembly(fout);
        fout.write("\n}\n");
    }

    @Override
    public void printName(BufferedWriter fout) throws IOException {
        fout.write("@" + name);
    }

    @Override
    public void printUse(BufferedWriter fout) throws IOException {

    }

    /*---------------以下是代码优化部分------------------*/
    public void buildCfgGraph(){
        if(cfgGraph!=null){
            cfgGraph.buildCFG();
        }
    }

    /**
     * 获取控制流图
     */
    public  CfgGraph getCfgGraph() {
        return cfgGraph;
    }

    /**
     * 获取支配树
     */
    public DominantTree getDominateTree(){
        return dominantTree;
    }

    /**
     * 建立支配树
     */
    public void buildDominantTree(){
        if(dominantTree!=null){
            //生成支配集
            dominantTree.generateDominateBlocks();
            //生成支配树
            dominantTree.generateDominantTree();
            //生成支配边界
            dominantTree.generateDominantEdge();
        }
    }

    /**
     * 获取活跃变量分析
     */
    public ActiveVarAnalysis getActiveVarAnalysis() {
        return activeVarAnalysis;
    }

    /**
     * 针对基本块进行活跃变量分析
     */
    public void activeVarAnalysis(){
        if(activeVarAnalysis!=null){
            //首先，让所有基本块构建use-def集
            activeVarAnalysis.generateUseDef();
            //然后，构建in-out集
            activeVarAnalysis.generateInOut();
            //随后，针对每个基本块，也进行活跃变量分析
            List<BasicBlock> basicBlocks = getAllBasicBlocks();
            for(BasicBlock basicBlock:basicBlocks){
                basicBlock.activeVarAnalysis();
            }
        }
    }

    /**
     * 输出活跃变量分析文件
     */
    public void printActiveVarAnalysisInst(BufferedWriter fout) throws IOException{
        List<BasicBlock> basicBlocks = getAllBasicBlocks();
        for(BasicBlock basicBlock:basicBlocks){
            fout.write("基本块: "+basicBlock.getName()+"\n\t\t\t");
            basicBlock.getActiveVarAnalysisInst().printActiveVarAnalysisInst(fout);
        }
    }

    /**
     * 获取函数下面的所有基本块
     * @return
     */
    public List<BasicBlock> getAllBasicBlocks(){
        return superBlock.getAllBasicBlocks();
    }

    /**
     * DCE删除所有基本块中的未使用指令
     */
    public void DCEBlockInst(){
        List<BasicBlock> basicBlocks = getAllBasicBlocks();
        for(BasicBlock basicBlock:basicBlocks){
            basicBlock.DCEBlockInst();
        }
    }

    public boolean isGVNInstruction(){
        for(Argument argument: arguments){
            if(argument.isArray()) return false;
        }
        for(BasicBlock basicBlock : this.getAllBasicBlocks()){
            for(Inst inst : basicBlock.getInstructions()){
                for(Value value: inst.usees){
                    if(value instanceof GlobalVar) return false;
                }
                if(inst instanceof CallInst) return false;
            }
        }
        return true;
    }


    /**
     * 插入phi指令
     */
    public void insertPhiInst(){
        //首先，设置起始基本块为函数的第一个基本块
        List<BasicBlock> basicBlocks = getAllBasicBlocks();
        MemToReg.setFirstBlock(basicBlocks.get(0));
        //遍历所有基本块，插入phi函数
        for(BasicBlock basicBlock:basicBlocks) {
            basicBlock.insertPhiInst();
        }
    }

    /**
     * 对于基本块只有一个跳转指令，且自己只有一个后继，
     * 那么直接让前面的基本块跳到后继即可
     */
    public void deleteUCBlock(){
        List<BasicBlock> basicBlocks = getAllBasicBlocks();
        for(BasicBlock basicBlock:basicBlocks){
            basicBlock.deleteUCBlock();
        }
    }

    /**
     * 删除phi指令
     * 由于这里涉及到函数的基本块的改变，所以需要先复制一个block副本再遍历
     * 目标：删除phi指令，先转换成PC指令，再转换成move指令
     */
    public void deletePhiInst(){
        List<BasicBlock> basicBlocks = new ArrayList<>(getAllBasicBlocks());
        for(BasicBlock basicBlock:basicBlocks){
            basicBlock.phiToPC();
        }
        for(BasicBlock basicBlock:basicBlocks){
            basicBlock.pcToMove();
        }
    }
}
