package backend.ir.entity;

import backend.ir.entity.insts.*;
import backend.ir.handler.IRParser;
import backend.ir.optimize.dataStructure.CfgGraph;
import backend.ir.optimize.dataStructure.DominantTree;
import backend.ir.optimize.tool.ActiveVarAnalysisInst;
import backend.ir.optimize.tool.DCETool;
import backend.risc_v.optimize.tool.DeletePhi;
import backend.ir.optimize.tool.MemToReg;
import frontend.semantic.symbol.IRBasicType;
import frontend.semantic.symbol.SymbolTable;
import glob.types.BLOCK_TYPE;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.*;

/**
 * &#064;Classname BasicBlock
 * &#064;Description  TODO
 * &#064;Date 2025/7/12 15:18
 * &#064;Created MuJue
 */

// 存放 Inst 和子 Basicblock

public class BasicBlock extends User {
    private BasicBlock fatherBlock;
    private final SymbolTable symbolTable;
    private Function function;
    private BLOCK_TYPE blockType;
    private boolean exist = true;
    private final ActiveVarAnalysisInst activeVarAnalysisInst;

    public ActiveVarAnalysisInst getActiveVarAnalysisInst() {
        return activeVarAnalysisInst;
    }

    public boolean isExist() {
        return exist;
    }

    public void setExist(boolean exist) {
        this.exist = exist;
    }

    public BasicBlock(String name, SymbolTable symbolTable, BasicBlock fatherBlock){
        super(name, IRBasicType.NONE);
        this.fatherBlock = fatherBlock;
        this.symbolTable = symbolTable;
        blockType = BLOCK_TYPE.NORMAL_BLOCK;
        activeVarAnalysisInst = new ActiveVarAnalysisInst(this);
        if(fatherBlock != null){
            this.function = fatherBlock.function;
        }
    }
    public BasicBlock(String name, SymbolTable symbolTable, Function function, BLOCK_TYPE blockType){
        super(name, IRBasicType.NONE);
        this.fatherBlock = null;
        this.symbolTable = symbolTable;
        this.blockType = blockType;
        this.function = function;
        activeVarAnalysisInst = new ActiveVarAnalysisInst(this);
    }
    public BasicBlock getFatherBlock() {
        return fatherBlock;
    }
    public void setFatherBlock(BasicBlock fatherBlock) {
        this.fatherBlock = fatherBlock;
    }
    public void setBlockType(BLOCK_TYPE blockType) {
        this.blockType = blockType;
    }
    public Inst getLastInst(){
        int valueSize = usees.size();
        for(int i = valueSize - 1; i >= 0; i--){
            if(usees.get(i) instanceof Inst){
                return (Inst) usees.get(i);
            }
        }
        return null;
    }

    public SymbolTable getSymbolTable() {
        return symbolTable;
    }

    public Function getFunction() {
        return function;
    }

    public void setFunction(Function function) {
        this.function = function;
    }

    public BLOCK_TYPE getBlockType() {
        return blockType;
    }

    @Override
    public void printAssembly(BufferedWriter fout) throws IOException {
        if(exist) {
            fout.write(name+":\n");
            int size = usees.size();
            for(int i = 0; i < size; i++){
                if(!(usees.get(i) instanceof BasicBlock)){
                    fout.write("\t");
                }
                usees.get(i).printAssembly(fout);
                if(i != size - 1){
                    fout.write("\n");
                }
            }
        }else{
            //这个基本块不存在，但是子基本块还是可以存在的
            for (Value usee : usees) {
                if (usee instanceof BasicBlock) {
                    fout.write("\n");
                    usee.printAssembly(fout);
                }
            }
        }
    }

    @Override
    public void printName(BufferedWriter fout) throws IOException {
        fout.write("%" + name);
    }

    @Override
    public void printUse(BufferedWriter fout) throws IOException {

    }

    public int getLoopDepth(){
        return 0;
    }

    /*----------------以下是优化部分--------------*/

    /**
     * 获取控制流图中，所有流入该基本块的block
     */
    public List<BasicBlock> getInBlocks(){
        List<BasicBlock> inBlocks = function.getCfgGraph().getInBlocks().get(this);
        return inBlocks==null?new ArrayList<>():inBlocks;
    }

    /**
     * 获取控制流图中，所有流出该基本块的block
     */
    public List<BasicBlock> getOutBlocks(){
        List<BasicBlock> outBlocks = function.getCfgGraph().getOutBlocks().get(this);
        return outBlocks==null?new ArrayList<>():outBlocks;
    }

    /**
     * 获取该基本块和所有子基本块
     * @return
     */
    public List<BasicBlock> getAllBasicBlocks(){
        List<BasicBlock> basicBlocks = new ArrayList<>();
        //添加自己
        basicBlocks.add(this);
        //递归添加子基本块
        for(Value usee : usees){
            if(usee instanceof BasicBlock basicBlockSon){
                basicBlocks.addAll(basicBlockSon.getAllBasicBlocks());
            }
        }
        return basicBlocks;
    }

    /**
     * 在指定位置加入指令
     * @param index 加入指令的位置
     */
    public void addInst(Inst inst, int... index){
        if(index.length==0){
            addUsee(inst);
        }else{
            int idx =  index[0];
            addUseeAt(inst,idx);
        }
        inst.setBasicBlock(this);
    }

    /**
     * 获取基本块所有指令，按照顺序
     * 只能用于查询，千万不要用于修改
     * 修改请使用getUsees
     */
    public List<Inst> getInstructions(){
        List<Inst> insts = new ArrayList<>();
        for(Value usee : usees){
            if(usee instanceof Inst instSon){
                insts.add(instSon);
            }
        }
        return insts;
    }

    /**
     * 删除基本块中的跳转指令之后的指令
     */
    public void DCEBlockInst(){
        DCETool.deleteBranchInstruction(this);
    }


    /**
     * 获取基本块的支配集合
     */
    public List<BasicBlock> getDominateBlocks(){
        List<BasicBlock> dominateBlocks = function.getDominateTree().getDominateBlocks().get(this);
        return dominateBlocks==null?new ArrayList<>():dominateBlocks;
    }

    /**
     * 获取基本块的支配边界
     */
    public List<BasicBlock> getDominateEdge(){
        List<BasicBlock> dominateEdge = function.getDominateTree().getDominateEdge().get(this);
        return dominateEdge==null?new ArrayList<>():dominateEdge;
    }

    /**
     * 获取支配树的父支配块
     */
    public BasicBlock getParentDominateBlock(){
        return function.getDominateTree().getParentBlock().get(this);
    }

    /**
     * 获取支配树的子支配块
     */
    public List<BasicBlock> getChildDominateBlocks(){
        List<BasicBlock> childDominateBlocks = function.getDominateTree().getChildBlocks().get(this);
        return childDominateBlocks==null?new ArrayList<>():childDominateBlocks;
    }

    /**
     * 获取活跃变量的use集合
     */
    public Set<Value> getUseSet(){
        return function.getActiveVarAnalysis().getUseVar(this);
    }

    /**
     * def集合
     */
    public Set<Value> getDefSet(){
        return function.getActiveVarAnalysis().getDefVar(this);
    }

    /**
     * 活跃变量分析的in集合
     */
    public Set<Value> getInSet(){
        return function.getActiveVarAnalysis().getInVar(this);
    }

    /**
     * 活跃变量分析out集合
     */
    public Set<Value> getOutSet(){
        return function.getActiveVarAnalysis().getOutVar(this);
    }

    /**
     * 进行语句级别的活跃变量分析
     */
    public void activeVarAnalysis(){
        if(activeVarAnalysisInst!=null){
            activeVarAnalysisInst.analyze();
        }
    }

    /**
     * 插入phi指令
     */
    public void insertPhiInst(){
        List<Inst> newInsts = getInstructions();
        for(Inst inst : newInsts){
            if(inst instanceof AllocateInst allocInst){
                if(allocInst.getFullDim()==null){
                    allocInst.insertPhiInst();
                    MemToReg.renameVar();
                }
            }
        }
    }

    /**
     * 对于基本块只有一个跳转指令，且自己只有一个后继，
     * 那么直接让前面的基本块跳到后继即可
     */
    public void deleteUCBlock(){
        List<Inst> newInsts = getInstructions();
        if(newInsts.size()==1 && newInsts.get(0)
                instanceof UCJumpInst ucJumpInst){
            //先获取要跳转的块，然后看跳转的块，inBlock是不是只有这一个
            BasicBlock jumpBlock = (BasicBlock) ucJumpInst.getUsee(0);
            List<BasicBlock> inBlocks = getInBlocks();
            List<BasicBlock> outBlocks = getOutBlocks();
            if(outBlocks.size()!=1){return;}
            BasicBlock outBlock = outBlocks.get(0);
            if(outBlock.getInBlocks().size()!=1){return;}
            //接下来，当前基本块和跳转后的基本块是单向流通
            boolean flag = false;
            for(BasicBlock inBlock : inBlocks){
                List<Inst> inBlockInsts = inBlock.getInstructions();
                for(Inst inst : inBlockInsts){
                    if(inst instanceof UCJumpInst ucJumpInst1){
                        flag = true;
                        ucJumpInst1.removeUsee(this);
                        ucJumpInst1.addUsee(jumpBlock);
                    }else if(inst instanceof BranchInst branchInst){
                        flag = true;
                        branchInst.removeUsee(this);
                        branchInst.addUsee(jumpBlock);
                    }
                }
            }

            if(flag){
                exist = false;
                CfgGraph cfgGraph = function.getCfgGraph();
                DominantTree dominantTree = function.getDominateTree();
                cfgGraph.toMergeFromBlock(this,outBlock);
                dominantTree.mergeDominantTree(outBlock,this);
            }
        }
    }

    /**
     * phi转pc指令
     */
    public void phiToPC(){
        //首先看这个基本块有没有phi，没有phi就不需要做
        List<Inst> newInsts = getInstructions();
        if(newInsts.get(0) instanceof PHIInst){
            //说明有phi
            List<PCopyInst> pCopyInsts =  new ArrayList<>();
            //根据前驱基本块找入边
            List<BasicBlock> inBlocks = new ArrayList<>(getInBlocks());
            for(BasicBlock inBlock : inBlocks){
                PCopyInst pCopyInst = new PCopyInst(
                        "var_pc_"+ IRParser.indexInFunction++,
                        IRBasicType.NONE,
                        this
                );
                pCopyInsts.add(pCopyInst);
                //如果Bi有多条出边，进入分叉
                if(inBlock.getOutBlocks().size()>1){
                    //创建新的基本块Bi'
                    BasicBlock BiNew = new BasicBlock(
                            "Block_pc_"+ IRParser.indexInFunction++,
                            null,
                            fatherBlock
                    );
                    //首先向functionParent中添加这个基本块
                    //因为要插在两个基本块中间，所以要确定id
                    //因为要插在当前基本块之前，所以是查“this”的位置
                    int index = fatherBlock.getUsees().indexOf(this);
                    fatherBlock.getUsees().add(index,BiNew);
                    //向这个基本块添加PC指令
                    BiNew.addUsee(pCopyInst);
                    //重新设置跳转语句，注意，因为Bi有多条出边，所以不可能是直接跳转语句，一定是BrInstruction
                    Inst lastInst = inBlock.getLastInst();
                    if(lastInst instanceof BranchInst branchInst){
                        //根据跳转的位置，将跳转块设置为Bi_new
                        if(this == branchInst.getUsees().get(1)){
                            //说明原本是true的时候跳转到这个块
                            branchInst.removeUsee(this);
                            branchInst.addUseeAt(BiNew,1);
                        }else if(this == branchInst.getUsees().get(2)){
                            //说明是false跳转过来的
                            branchInst.removeUsee(this);
                            branchInst.addUseeAt(BiNew,2);
                        }
                        //添加增加的块到这个块的直接跳转语句
                        UCJumpInst ucJumpInst = new UCJumpInst(BiNew);
                        ucJumpInst.addUsee(this);
                        BiNew.addUsee(ucJumpInst);
                    }
                    //需要修改控制流图
                    function.getCfgGraph().insertBlock(inBlock,this,BiNew);
                }else{
                    //将指令加入末尾，当然肯定要加在跳转之前，否则运行不到，没有意义
                    //首先获取最后一个跳转指令在usees的位置
                    int pcIndex = inBlock.getUsees().indexOf(inBlock.getLastInst());
                    inBlock.addUseeAt(pCopyInst, pcIndex);
                }
            }

            //现在PC已经插入，需要删除phi指令
            Iterator<Value> iterator = usees.iterator();
            while(iterator.hasNext()){
                Value usee = iterator.next();
                if(usee instanceof PHIInst phiInst){
                    phiInst.changePhiToPC(pCopyInsts);
                    phiInst.deleteAllUsees();
                    iterator.remove();
                }
            }
        }
    }

    /**
     * 将并行化PC指令改写为串行化Move指令
     * 这样才可以转换成Mips
     * 首先，获取基本块需要串行化的PC指令。
     * 根据上一个函数的措施，如果有PC，一定是倒数第二个指令是PC
     * 遍历指令
     * 每次选择目标寄存器未被依赖的pc指令改写为move
     * 如果存在环
     * 那么需要选择某个节点拆开，新建寄存器a'
     */
    public void pcToMove() {
        //首先，判断倒数第二个是不是PC指令
        List<Inst> newInsts = getInstructions();
        if(newInsts.size()>=2 && newInsts.get(newInsts.size()-2) instanceof PCopyInst pCopyInst){
            //需要移出指令
            removeUsee(pCopyInst);
            List<MoveInst> moveInsts = new ArrayList<>();
            List<Value> pcSource = pCopyInst.getSource();
            List<Value> pcTarget = pCopyInst.getTarget();
            for(int i=0;i<pcSource.size();i++){
                MoveInst moveInst = new MoveInst(
                        "var_move_"+IRParser.indexInFunction++,
                        IRBasicType.NONE,
                        this,
                        pcSource.get(i),
                        pcTarget.get(i)
                );
                moveInsts.add(moveInst);
            }
            //现在需要添加拆除循环, 添加新的临时寄存器用于move
            List<MoveInst> tempMove = DeletePhi.generateMoveTemp(function,moveInsts);
            for(MoveInst tempMoveInst : tempMove){
                //在头部插入
                moveInsts.addFirst(tempMoveInst);
            }
            //最终，要在倒数第二个位置依次插入这些move指令
            for(MoveInst moveInst : moveInsts){
                int index = usees.indexOf(getLastInst());
                addUseeAt(moveInst,index);
            }
        }
    }
}
