package backend;
import IR.DomAnalysis;
import IR.IRInstruction.*;
import IR.IRModule;
import IR.IRType.IRArrayType;
import IR.IRType.IRFloatType;
import IR.IRType.IRPointerType;
import IR.IRType.IRType;
import IR.IRValueRef.*;

import java.util.*;
import java.util.stream.Collectors;

import static IR.IRValueRef.IRBaseBlockRef.IRGetFirstInstruction;
import static IR.IRValueRef.IRBaseBlockRef.IRGetNextInstruction;
import static IR.IRValueRef.IRFunctionBlockRef.IRGetFirstBaseBlock;
import static IR.IRValueRef.IRFunctionBlockRef.IRGetNextBaseBlock;

/**
 * 线性扫描寄存器分配算法，在RISCVBuilder翻译代码之前对IR进行扫描，得出所有变量应当存放的地址（寄存器或栈）
 */
public class MemoryRegisterAlloc implements RegisterAllocator {

    private Map<String, List<String>> varLocations ;
    private Map<String, int[]> liveIntervals ;
    private int stackPointer = 0;
    private final int wordSize=4;

    private int registerCount;
    private String[] Reg;
    private boolean[] availReg;
    private Map<String,Integer> occupiedReg;

    private int floatRegCount;
    private String[] floatReg;
    private  boolean[] availFloatReg;
    private Map<String,Integer> occupiedFloatReg;

    private IRFunctionBlockRef functionBlockRef;

    private IRModule module;

    private List<String> arrayVars ;
    private Map<String,Integer> arrayLength;
    private List<String> floatVars;
    private List<String> copyFloatVars;

    private List<IRInstruction> worklist ;//ir指令
    private List<IRInstruction> copylist ;
    private Map<IRInstruction,List<String>> inOfInstructions ;
    private Map<IRInstruction,List<String>> outOfInstructions;

    private List<String> vars ;
    private List<String> copyVars;
    private Map<String,Integer> startPoint ;
    private Map<String,Integer> endPoint ;

    Map<IRBaseBlockRef, List<IRBaseBlockRef>> predecessors=new HashMap<>();
    private List<IRBaseBlockRef> blockWorkList ;
    private List<IRBaseBlockRef> blockCopyList ;
    private Map<IRBaseBlockRef,List<String>> inOfBlocks;
    private Map<IRBaseBlockRef,List<String>> outOfBlocks;
    private Map<IRBaseBlockRef,List<String>> useOfBlocks;
    private Map<IRBaseBlockRef,List<String>> defOfBlocks;

    public Map<IRBaseBlockRef, List<IRBaseBlockRef>> getPredecessors() {
        return new HashMap<>(predecessors);
    }

    @Override
    public void setFunction(IRFunctionBlockRef functionBlockRef) {
        this.functionBlockRef = functionBlockRef;
        registerCount = 15;
        floatRegCount = 21;
        ArrayList<String> regArrayList = new ArrayList<>();
        ArrayList<String> floatRegArrayList = new ArrayList<>();
        for (int i = 1; i < 12; i++) {
            regArrayList.add("s" + i);
            floatRegArrayList.add("fs" + i);
        }
        for (int i = 3; i < 7; i++) {
            regArrayList.add("t" + i);
            floatRegArrayList.add("ft" + i);
        }
        for (int i = 7; i < 12; i++) {
            floatRegArrayList.add("ft" + i);
        }
        floatRegArrayList.add("fs0");
        int floatParams = 0;
        int intParams = 0;
        for (IRValueRef param : functionBlockRef.getParams()) {
            if (param.getType() instanceof IRFloatType) {
                floatParams++;
            }
            else
                intParams++;
        }
        if (floatParams + 1 < 8) {
            for (int i = floatParams + 1; i < 8; i++) {
                floatRegArrayList.add("fa" + i);
                floatRegCount++;
            }
        }
        if (intParams + 1 < 8) {
            for (int i = intParams + 1; i < 8; i++) {
                regArrayList.add("a" + i);
                registerCount++;
            }
        }
        Reg = regArrayList.toArray(new String[0]);
        floatReg = floatRegArrayList.toArray(new String[0]);
        availReg = new boolean[registerCount];
        availFloatReg = new boolean[floatRegCount];

    }

    @Override
    public void setModule(IRModule module) {
        this.module = module;
    }

    private void initListAndMap(){
        stackPointer=0;
        worklist=new ArrayList<>();
        copylist=new ArrayList<>();
        blockWorkList=new ArrayList<>();
        blockCopyList=new ArrayList<>();
        useOfBlocks=new HashMap<>();
        defOfBlocks=new HashMap<>();
        vars=new ArrayList<>();
        copyVars=new ArrayList<>();
        arrayVars=new ArrayList<>();
        arrayLength=new HashMap<>();
        floatVars=new ArrayList<>();
        copyFloatVars=new ArrayList<>();
        varLocations=new HashMap<>();
        liveIntervals=new HashMap<>();
        inOfInstructions=new HashMap<>();
        outOfInstructions=new HashMap<>();
        inOfBlocks=new HashMap<>();
        outOfBlocks=new HashMap<>();
        startPoint=new HashMap<>();
        endPoint=new HashMap<>();
        occupiedReg=new HashMap<>();
        occupiedFloatReg=new HashMap<>();
    }

    /*
     * 把函数所有基本块加入blockworklist中，并初始化
     * 把函数的所有指令加入worklist中,并初始化每个指令的in和out
     */
    private void setWorklist() {
        for(IRBaseBlockRef blockRef =IRGetFirstBaseBlock(functionBlockRef);blockRef!=null;
            blockRef=IRGetNextBaseBlock(functionBlockRef,blockRef)){
            //TODO: baseblocks有去重吗？
            if(!blockWorkList.contains(blockRef)){
                blockWorkList.add(blockRef);
                blockCopyList.add(blockRef);
                inOfBlocks.put(blockRef,new ArrayList<>());
                outOfBlocks.put(blockRef,new ArrayList<>());
                useOfBlocks.put(blockRef,new ArrayList<>());
                defOfBlocks.put(blockRef,new ArrayList<>());
                //每个基本块逐条添加，初始化
                for (IRInstruction inst =IRGetFirstInstruction(blockRef);inst!=null;
                     inst=IRGetNextInstruction(blockRef,inst)){
                    worklist.add(inst);
                    copylist.add(inst);
                    inOfInstructions.put(inst,new ArrayList<>());
                    outOfInstructions.put(inst,new ArrayList<>());
                }
            }
        }
        Collections.reverse(worklist);//从后往前遍历
        Collections.reverse(blockWorkList);
        //计算基本块的use和def
        for (IRBaseBlockRef baseBlockRef: blockWorkList){
            calculateUseAndDefOfBlock(baseBlockRef);
        }
    }

    /*
     * 获取基本块的直接后继基本块
     */
    public List<IRBaseBlockRef> getImmediateSuccessors(IRBaseBlockRef block) {
        List<IRBaseBlockRef> successors = new ArrayList<>();
        List<IRInstruction> instructions=block.getInstructionList();
        int last=instructions.size()-1;
        IRInstruction firstBrInstruction =instructions.get(last);
        if(firstBrInstruction instanceof ReturnInstruction)return successors;
        while(last >= 0 && instructions.get(last)!=null && instructions.get(last) instanceof BranchInstruction){
            firstBrInstruction=instructions.get(last);
            last--;
        }
        IRBaseBlockRef block1=((BranchInstruction) firstBrInstruction).getBaseBlock1();
        List<IRBaseBlockRef> temp=new ArrayList<>();
        temp.add(block);
        successors.add(block1);
        if(((BranchInstruction) firstBrInstruction).getBaseBlock2() != null){
            IRBaseBlockRef block2=((BranchInstruction) firstBrInstruction).getBaseBlock2();
            successors.add(block2);
            if(predecessors.get(block2)==null)predecessors.put(block2,temp);
            else predecessors.get(block2).addAll(temp);
        }
        if(predecessors.get(block1)==null)predecessors.put(block1, temp);
        else predecessors.get(block1).addAll(temp);
        return successors;
    }

    /*
     * 判断类型是否为变量
     */
    private boolean notVariable(IRValueRef var){
        return !(var instanceof IRArrayRef||var instanceof IRVirtualRegRef||var instanceof IRGlobalRegRef);
    }

    /*
     * 获取指令里使用的变量
     */
    private List<String> getUsedVarsOfInstructions(IRInstruction inst){
        List<String> usedVars = new ArrayList<>();
        List<IRValueRef> operands = inst.getOperands();
        operands.removeIf(this::notVariable);
        if(!operands.isEmpty()){
            if(inst instanceof AllocateInstruction||inst instanceof CalculateInstruction
                    ||inst instanceof CompareInstruction||inst instanceof GetElementPointerInstruction
                    ||inst instanceof LoadInstruction||inst instanceof TypeTransferInstruction
                    ||inst instanceof ZextInstruction||inst instanceof PhiInstruction){
                operands.remove(0);
            }
            if (inst instanceof PhiInstruction){
                for (IRBaseBlockRef keySet: ((PhiInstruction) inst).getIncomingValues().keySet()){
                    String var=((PhiInstruction) inst).getIncomingValues().get(keySet).getText();
                    if(Character.isDigit(var.charAt(0)))continue;
                    vars.add(var);
                    usedVars.add(var);
                }
            }
            vars.addAll(operands.stream().map(IRValueRef::getText).toList());
            vars=vars.stream().distinct().collect(Collectors.toList());
            usedVars.addAll(operands.stream().map(IRValueRef::getText).toList());
        }
        usedVars=usedVars.stream().distinct().collect(Collectors.toList());
        return usedVars;
    }

    /*
     * 获取指令里定义的变量
     * allocate, calculate, compare, getelementp, load, type, zext
     */
    private List<String> getDefVarsOfInstructions(IRInstruction inst){
        List<String> defVars = new ArrayList<>();
        List<IRValueRef> operands = inst.getOperands();
        operands.removeIf(this::notVariable);
        if(!operands.isEmpty()){
            if(inst instanceof AllocateInstruction||inst instanceof CalculateInstruction
                    ||inst instanceof CompareInstruction||inst instanceof GetElementPointerInstruction
                    ||inst instanceof LoadInstruction||inst instanceof TypeTransferInstruction
                    ||inst instanceof ZextInstruction||inst instanceof PhiInstruction){
                vars.addAll(operands.stream().map(IRValueRef::getText).toList());
                vars=vars.stream().distinct().collect(Collectors.toList());
                defVars.add(operands.get(0).getText());//以上指令的第一个操作数是被define的
            }
        }
        defVars=defVars.stream().distinct().collect(Collectors.toList());
        return defVars;
    }

    private void calculateUseAndDefOfBlock(IRBaseBlockRef block){
        List<String> use=new ArrayList<>();
        List<String> def=new ArrayList<>();
        for (IRInstruction inst = IRGetFirstInstruction(block); inst != null;
             inst = IRGetNextInstruction(block, inst)){
            List<String> usedVars = getUsedVarsOfInstructions(inst);
            List<String> defVars = getDefVarsOfInstructions(inst);
            for(String usedVar : usedVars){
                if(!def.contains(usedVar))use.add(usedVar);
            }
            for (String defVar : defVars){
                if(!use.contains(defVar))def.add(defVar);
            }
        }
        use=use.stream().distinct().collect(Collectors.toList());
        def=def.stream().distinct().collect(Collectors.toList());
        useOfBlocks.put(block,use);
        defOfBlocks.put(block,def);
    }

    /*
     * 以基本块为单位，计算每个基本块的in和out
     */
    private void WorkListAlgorithm(){
        boolean change=true;
        while (change){
            change=false;
            for (IRBaseBlockRef blockRef : blockWorkList) {
                List<String> oldOutOfBlocks = new ArrayList<>(outOfBlocks.get(blockRef));
                //out=all in of successors
                List<String> curOutOfBlocks = new ArrayList<>();
                List<IRBaseBlockRef> successors = getImmediateSuccessors(blockRef);
                for (IRBaseBlockRef successor : successors) {
                    List<String> inOfBlock = inOfBlocks.get(successor);
                    curOutOfBlocks.addAll(inOfBlock);
                }
                curOutOfBlocks = curOutOfBlocks.stream().distinct().collect(Collectors.toList());
                List<String> copyOutOfBlocks=new ArrayList<>(curOutOfBlocks);
                //in=use+(out-def)
                List<String> curInOfBlocks = new ArrayList<>(useOfBlocks.get(blockRef));
                copyOutOfBlocks.removeAll(defOfBlocks.get(blockRef));
                curInOfBlocks.addAll(copyOutOfBlocks);
                curInOfBlocks = curInOfBlocks.stream().distinct().collect(Collectors.toList());
                //if out changed,push all successors into worklist
                List<String> oldInOfBlocks = new ArrayList<>(inOfBlocks.get(blockRef));
                if (!listEqual(curOutOfBlocks, oldOutOfBlocks) || !listEqual(curInOfBlocks, oldInOfBlocks)) {
                    change = true;
                }
                //update in and out of blocks
                outOfBlocks.put(blockRef, curOutOfBlocks);
                inOfBlocks.put(blockRef, curInOfBlocks);
            }
        }
    }

    /*
     * 时清凯的完整工作集算法
     */
    private void TestWorkList(){
        while(!blockWorkList.isEmpty()){
            IRBaseBlockRef blockRef = blockWorkList.remove(0);
            //out=all in of successors
            List<String> curOutOfBlocks = new ArrayList<>();
            List<IRBaseBlockRef> successors = getImmediateSuccessors(blockRef);
            for (IRBaseBlockRef successor : successors) {
                List<String> inOfBlock = inOfBlocks.get(successor);
                curOutOfBlocks.addAll(inOfBlock);
            }
            curOutOfBlocks = curOutOfBlocks.stream().distinct().collect(Collectors.toList());
            List<String> copyOutOfBlocks=new ArrayList<>(curOutOfBlocks);
            //in=use+(out-def)
            List<String> curInOfBlocks = new ArrayList<>(useOfBlocks.get(blockRef));
            copyOutOfBlocks.removeAll(defOfBlocks.get(blockRef));
            curInOfBlocks.addAll(copyOutOfBlocks);
            curInOfBlocks = curInOfBlocks.stream().distinct().collect(Collectors.toList());
            List<String> oldInOfBlocks = new ArrayList<>(inOfBlocks.get(blockRef));
            //if out changed,push all successors into worklist
            if (!listEqual(curInOfBlocks, oldInOfBlocks) ) {
                List<IRBaseBlockRef> predecessor=predecessors.get(blockRef);
                if(predecessor!=null){
                    blockWorkList.addAll(predecessor);
                    blockWorkList=blockWorkList.stream().distinct().collect(Collectors.toList());
                }
            }
            //update in and out of blocks
            outOfBlocks.put(blockRef, curOutOfBlocks);
            inOfBlocks.put(blockRef, curInOfBlocks);
        }
    }

    private void calculateInstructions(){
        for (IRBaseBlockRef blockRef : blockCopyList){
            //逐条遍历计算最后的in和out
            List<String> inOfBlock = inOfBlocks.get(blockRef);
            List<String> outOfBlock = outOfBlocks.get(blockRef);
            List<IRInstruction> instructions = new ArrayList<>(blockRef.getInstructionList());
            Collections.reverse(instructions);
            for(int i=0;i<instructions.size();i++){
                IRInstruction inst = instructions.get(i);
                if(i==0){//最后一条指令的out就是这个基本块的out
                    outOfInstructions.put(inst,outOfBlock);
                }else{//其他的指令的out等于下一条i--指令的in
                    outOfInstructions.put(inst,inOfInstructions.get(instructions.get(i-1)));
                }

                if(i==instructions.size()-1){//第一条指令的in就是这个基本块的in
                    inOfInstructions.put(inst,inOfBlock);
                }else{//in=use+(out-def)
                    List<String> usedVars = new ArrayList<>(getUsedVarsOfInstructions(inst));
                    List<String> intersection = new ArrayList<>(getDefVarsOfInstructions(inst));
                    List<String> outOfInst=new ArrayList<>(outOfInstructions.get(inst));
                    intersection.retainAll(outOfInst);
                    outOfInst.removeAll(intersection);
                    usedVars.addAll(outOfInst);
                    usedVars=usedVars.stream().distinct().collect(Collectors.toList());
                    inOfInstructions.put(inst,usedVars);
                }
            }
        }
    }

    /*
     * 蚂蚁的遍历法
     */
    private void worklistAlgorithm(){
        boolean change=true;
        while (change){
            change=false;
            for (IRInstruction inst : worklist) {
                List<String> oldOutOfInstructions = new ArrayList<>(outOfInstructions.get(inst));
                //out=all in of successors
                List<String> curOutOfInstructions = new ArrayList<>();
                IRInstruction nextInst = IRGetNextInstruction(inst.getBaseBlock(), inst);
                List<IRBaseBlockRef> postDominators = getImmediateSuccessors(inst.getBaseBlock());
                if (nextInst == null) { //inst is last instruction of this baseblock
                    for (IRBaseBlockRef postDominator : postDominators) {
                        IRInstruction firstInst = IRGetFirstInstruction(postDominator);
                        curOutOfInstructions.addAll(inOfInstructions.get(firstInst));
                    }
                } else curOutOfInstructions.addAll(inOfInstructions.get(nextInst));
                curOutOfInstructions=curOutOfInstructions.stream().distinct().collect(Collectors.toList());
                //in=use+(out-def)
                List<String> curInOfInstructions = new ArrayList<>(getUsedVarsOfInstructions(inst));
                List<String> intersection = new ArrayList<>(getDefVarsOfInstructions(inst));
                intersection.retainAll(oldOutOfInstructions);//out^def
                oldOutOfInstructions.removeAll(intersection);//out-def
                curInOfInstructions.addAll(oldOutOfInstructions);
                curInOfInstructions=curInOfInstructions.stream().distinct().collect(Collectors.toList());
                //if out or in changed continue
                oldOutOfInstructions = outOfInstructions.get(inst);
                List<String> oldInOfInstructions = inOfInstructions.get(inst);
                if(!listEqual(curInOfInstructions,oldInOfInstructions)||!listEqual(curOutOfInstructions,oldOutOfInstructions)){
                    change=true;
                }
                //update in and out of blocks
                outOfInstructions.put(inst, curOutOfInstructions);
                inOfInstructions.put(inst, curInOfInstructions);
            }
        }
    }

    /*
     * 判断两个列表是否含有相同元素（顺序不影响）
     */
    boolean listEqual(List<String> list1,List<String> list2){
        return list1.size()==list2.size()&& new HashSet<>(list1).containsAll(list2);
    }

    /*
     * 计算每个变量的live range
     */
    private void calculateIntervals(){
        int cnt=0;
        for(IRInstruction inst : copylist){
            cnt++;
            List<String> outOfInst=outOfInstructions.get(inst);
            List<String> inOfInst=inOfInstructions.get(inst);
            for (String inVal: inOfInst){
                startPoint.putIfAbsent(inVal,cnt);
                endPoint.put(inVal,cnt);
            }
            for (String outVal : outOfInst){
                //第一次出现
                startPoint.putIfAbsent(outVal, cnt);
                endPoint.put(outVal, cnt);
            }
        }
        List<String> copy=new ArrayList<>(vars);
        for(String var:copy){
            if(startPoint.get(var)==null){
                vars.remove(var);
                List<String> location=new ArrayList<>();
                location.add(String.valueOf(-1));
                varLocations.put(var,location);
            }
        }
        for (String varName : vars){
            int[] liveInterval = new int[2];
            liveInterval[0]=startPoint.get(varName);
            liveInterval[1]=endPoint.get(varName);
            liveIntervals.put(varName,liveInterval);
        }
    }

    /*
     * 遍历所有变量，把数组变量及其长度记录到list里
     * 把float类型记录，在之后单独处理
     */
    private void setArrayAndFloatVars(){
        for(IRInstruction inst : copylist){
            List<IRValueRef> operands = inst.getOperands();
            operands.removeIf(this::notVariable);
            if(!operands.isEmpty()){
                for(IRValueRef operand : operands){
                    IRType type = operand.getType();
                    if(inst instanceof AllocateInstruction&&type instanceof IRPointerType &&((IRPointerType) type).getBaseType() instanceof IRArrayType){
                        arrayVars.add(operand.getText());
                        List<Integer> lengthList=((IRArrayType) ((IRPointerType) type).getBaseType()).getLengthList();
                        int len=1;
                        for (int length : lengthList){
                            len*=length;
                        }
                        arrayLength.put(operand.getText(),len);
                    }else if(type instanceof IRFloatType){
                        floatVars.add(operand.getText());
                        vars.remove(operand.getText());
                    }
                }
            }
        }
        arrayVars=arrayVars.stream().distinct().collect(Collectors.toList());
        floatVars=floatVars.stream().distinct().collect(Collectors.toList());
        Arrays.fill(availReg, true);
        Arrays.fill(availFloatReg,true);
    }

    /*
     * 线性分配算法
     */
    private void linearScanAllocate(){
        List<String> active=new ArrayList<>();
        List<String> sortedVars=sortVarsByStartPoint(vars);// in order of increasing start point
        for(String var : sortedVars){
            if(arrayVars.contains(var)){//if var is an array
                int len=arrayLength.get(var)+1; //put array name on stack
                storeToStack(var,len*wordSize);
                continue;
            }
            copyVars.add(var);
            int start=startPoint.get(var);
            active=expireOldVars(start,active,false);
            if(active.size()>=registerCount){
                spill(var,active,false);
            }else{
                // a register removed from pool of free registers
                String reg=getOneAvailReg(var,false);
                List<String>location=new ArrayList<>();
                location.add(reg);
                varLocations.put(var,location);
                //add i to active, sorted by increasing end point
                addVarByEndPoint(active,var);
            }
        }
    }

    /*
     * 按start递增对活跃周期进行排序
     */
    private List<String> sortVarsByStartPoint(List<String> vars){
        List<String> sortedVars = new ArrayList<>(vars);
        sortedVars.sort(Comparator.comparingInt(varName -> startPoint.get(varName)));
        return sortedVars;
    }

    /*
     * 按照end递增往active里添加一个变量
     */
    private void addVarByEndPoint(List<String> active,String var){
        active.add(var);
        active.sort(Comparator.comparingInt(varName -> endPoint.get(varName)));
    }

    /*
     * 把active里元素end时间早于cutTime的变量都去掉
     */
    private List<String> expireOldVars(int curTime,List<String> active,boolean f){
        List<String> copy=new ArrayList<>(active);
        for (String var : active){//in order of increasing end point
            if(endPoint.get(var)>=curTime)return copy;
            //remove var from active
            copy.remove(var);
            //add register to pool of free registers
            if(f){//float type
                int reg=occupiedFloatReg.get(var);
                availFloatReg[reg]=true;
            }else{
                int reg=occupiedReg.get(var);
                availReg[reg]=true;
            }
        }
        return copy;
    }

    /*
     * 选择一个结束最晚的变量溢出到栈上
     */
    private void spill(String var,List<String> active,boolean f){
        //last interval in active
        int latestEndPoint=endPoint.get(active.get(active.size()-1));
        if(endPoint.get(var)>latestEndPoint){
            storeToStack(var,wordSize);
        }else{
            int reg=f?occupiedFloatReg.get(active.get(active.size()-1))
                    : occupiedReg.get(active.get(active.size()-1));
            List<String> regLocation=new ArrayList<>();
            regLocation.add(f?floatReg[reg]:Reg[reg]);
            varLocations.put(var,regLocation);

            if(f)occupiedFloatReg.put(var,reg);
            else occupiedReg.put(var,reg);

            storeToStack(active.get(active.size()-1),wordSize);
            active.remove(active.get(active.size()-1));
            addVarByEndPoint(active,var);
        }
    }

    /*
     * 把变量存到栈上
     */
    private void storeToStack(String var,int size){
        stackPointer+=size;//new stack location
        List<String> location=new ArrayList<>();
        location.add(String.valueOf(stackPointer));
        varLocations.put(var,location);
    }

    /*
     * 获取一个空的寄存器
     */
    private String getOneAvailReg(String var,boolean f){
        if(f){
            for(int i=0;i<floatRegCount;i++){
                if(availFloatReg[i]){
                    availFloatReg[i]=false;
                    occupiedFloatReg.put(var,i);
                    return floatReg[i];
                }
            }
        }else{
            for (int i=0;i<registerCount;i++){
                if(availReg[i]){
                    availReg[i]=false;
                    occupiedReg.put(var,i);
                    return Reg[i];
                }
            }
        }
        return null;
    }

    /*
     * 为浮点数类型的变量分配寄存器
     */
    private void floatAllocate(){
        List<String> active=new ArrayList<>();
        List<String> sortedVars=sortVarsByStartPoint(floatVars);// in order of increasing start point
        for(String var : sortedVars){
            copyFloatVars.add(var);
            int start=startPoint.get(var);
            active=expireOldVars(start,active,true);
            if(active.size()>=floatRegCount){
                spill(var,active,true);
            }else{
                // a register removed from pool of free registers
                String reg=getOneAvailReg(var,true);
                List<String>location=new ArrayList<>();
                location.add(reg);
                varLocations.put(var,location);
                //add i to active, sorted by increasing end point
                addVarByEndPoint(active,var);
            }
        }
    }

    private void allocateArrayVars(){
        for(String var:arrayVars){//if var is an array
            int len=arrayLength.get(var)+1; //put array name on stack
            storeToStack(var,len*wordSize);
        }
        vars.removeAll(arrayVars);
        copyVars.addAll(vars);
        copyFloatVars.addAll(floatVars);
    }

    /*
     * 图着色寄存器分配
     */
    private void colorAllocate(List<String> vars,int regCount){
        Stack<String> stack=new Stack<>();
        Map<String,String> regAllocate=new HashMap<>();
        //遍历liveintervals，每寄存器对应一个List<String>
        Map<String,List<String>> conflictVars = calculateConflictVars(vars);//TODO: 浮点数
        Map<String,List<String>> conflictCopy=new HashMap<>(conflictVars);
        //如果每个点的度都小于寄存器数量，则加入栈中
        addVertexsToStack(vars,conflictCopy,regCount,stack);
        //剩下的点入度都大于等于寄存器数量
        while(!vars.isEmpty()){
            //Choose a vertex immediately to spill
            storeToStack(vars.get(0), 4);
            //remove the vertex from the graph
            removeVertexFromGraph(vars.get(0),vars,conflictCopy);
            vars.remove(0);
            addVertexsToStack(vars,conflictCopy,regCount,stack);
        }
        //给栈里的元素分配寄存器
        while(!stack.isEmpty()){
            String var=stack.pop();
            boolean f = regCount==floatRegCount;
            List<String> conflictedRegs=getConflictedRegs(regAllocate,conflictVars.get(var));
            String reg = "";
            if(f){
                for (int i=0;i<floatRegCount;i++){
                    if(!conflictedRegs.contains(floatReg[i])){
                        reg=floatReg[i];
                        break;
                    }
                }
            }else{
                for (int i=0;i<registerCount;i++){
                    if(!conflictedRegs.contains(Reg[i])){
                        reg=Reg[i];
                        break;
                    }
                }
            }
            regAllocate.put(var,reg);
            List<String>location=new ArrayList<>();
            location.add(reg);
            varLocations.put(var,location);
        }
    }

    //获取当前节点的相邻节点已经占用的寄存器
    private List<String> getConflictedRegs(Map<String,String> regAllocate,List<String> conflictVars){
        List<String> conflictedRegs=new ArrayList<>();
        for (String conflictVar : conflictVars){
            if(regAllocate.get(conflictVar)!=null){
                conflictedRegs.add(regAllocate.get(conflictVar));
                conflictedRegs=conflictedRegs.stream().distinct().collect(Collectors.toList());
            }
        }
        return conflictedRegs;
    }

    /*
     * 把入度小于k的点都加到栈里，并从图中删去
     */
    private void addVertexsToStack(List<String> vars,Map<String,List<String>> conflictVars,int regCount,Stack<String> stack){
        List<String> copy;
        while(true){
            boolean find=false;
            copy=new ArrayList<>(vars);
            for (String var: copy){
                List<String> conflict=conflictVars.get(var);
                int size=conflict.size();
                if(size<regCount){
                    find=true;
                    removeVertexFromGraph(var,vars,conflictVars);
                    stack.push(var);
                    vars.remove(var);
                }
            }
            if(!find)break;
        }
    }

    /*
     * 从图中删去一个点的所有边
     */
    private void removeVertexFromGraph(String var,List<String> vars,Map<String,List<String>> conflictVars){
        for (String variable: vars){
            if(var.equals(variable))continue;
            conflictVars.get(variable).remove(var);
        }
    }

    /*
     * 分别计算每个变量冲突的变量list
     */
    private Map<String,List<String>> calculateConflictVars(List<String> vars){
        Map<String,List<String>> conflictVars=new HashMap<>();
        vars=sortVarsByStartPoint(vars);
        for (String var:vars){
            int end=endPoint.get(var);
            for (String conflictVar:vars){
                if(startPoint.get(conflictVar)>end)break;
                List<String> temp=new ArrayList<>();
                temp.add(conflictVar);
                temp=temp.stream().distinct().collect(Collectors.toList());
                if(conflictVars.get(var)==null)conflictVars.put(var,temp);
                else conflictVars.get(var).addAll(temp);
                temp=new ArrayList<>();
                temp.add(var);
                temp=temp.stream().distinct().collect(Collectors.toList());
                if(conflictVars.get(conflictVar)==null)conflictVars.put(conflictVar,temp);
                else conflictVars.get(conflictVar).addAll(temp);
            }
            conflictVars.computeIfAbsent(var, k -> new ArrayList<>());
        }
        return conflictVars;
    }

    /*
     * 在函数调用的地方保留所有活跃寄存器的信息并记录栈空间
     */
    private void functionCallPreserve(){
        int cnt=0;
        for (IRInstruction inst : copylist){
            cnt++;
            if(inst instanceof CallInstruction){
                List<String> active=getCurrentActiveVars(cnt);
                for (String s : active) {
                    //storeToStack(s,4);
                    stackPointer+=4;//new stack location
                    List<String> location=new ArrayList<>(varLocations.get(s));
                    location.add(String.valueOf(stackPointer));
                    varLocations.put(s,location);
                }
                storeToStack("ra",4);
            }
        }
    }

    /*
     * 获得当前活跃的变量集合
     */
    private List<String> getCurrentActiveVars(int cutTime){
        List<String> active=new ArrayList<>();
        for (String var : copyVars){
            int[] liveInterval=liveIntervals.get(var);
            if(liveInterval[0]<=cutTime&&liveInterval[1]>=cutTime){
                List<String> location=varLocations.get(var);
                String curLocation=location.get(location.size()-1);
                if(!Character.isDigit(curLocation.charAt(0))){//只计算在寄存器里的
                    active.add(var);
                }
            }
        }
        for (String var:copyFloatVars){
            int[] liveInterval=liveIntervals.get(var);
            if(liveInterval[0]<=cutTime&&liveInterval[1]>=cutTime){
                List<String> location=varLocations.get(var);
                String curLocation=location.get(0);
                if(!Character.isDigit(curLocation.charAt(0))){//只计算在寄存器里的
                    active.add(var);
                }
            }
        }
        return active;
    }

    /**
     * 去除没有使用的常量定义
     */
    private void removeUnusedVarDefinitions() {
        Set<String> usedVars = new HashSet<>(startPoint.keySet());
        Iterator<IRInstruction> iterator = copylist.iterator();
        while (iterator.hasNext()) {
            IRInstruction inst = iterator.next();
            List<String> defVars = getDefVarsOfInstructions(inst);
            if (!defVars.isEmpty() && !usedVars.contains(defVars.get(0))) {
                iterator.remove();
            }
        }
    }

    @Override
    public int allocate(){
        initListAndMap();
        setWorklist();
        //WorkListAlgorithm();
        TestWorkList();
        calculateInstructions();
        calculateIntervals();
//        removeUnusedVarDefinitions();
        setArrayAndFloatVars();
//        if(this.functionBlockRef.getBaseBlocks().size()>3000||this.vars.size()>3000){
            linearScanAllocate();
            floatAllocate();
//        }else{
//            allocateArrayVars();
//            colorAllocate(vars,registerCount);
//            colorAllocate(floatVars,floatRegCount);
//        }
        functionCallPreserve();
        return stackPointer;
    }

    @Override
    public List<String> getRegister(String variable) {
        return varLocations.get(variable);
    }
}
