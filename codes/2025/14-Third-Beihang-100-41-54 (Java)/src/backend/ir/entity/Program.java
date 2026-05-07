package backend.ir.entity;

import backend.ir.optimize.GVN.GlobalCommandMoving;
import backend.ir.optimize.GVN.GlobalVarLocalize;
import backend.ir.optimize.GVN.GlobalVarNumbering;
import backend.ir.optimize.tool.DCETool;
import frontend.semantic.symbol.IRBasicType;
import frontend.semantic.symbol.SymbolTable;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * &#064;Classname Program
 * &#064;Description  TODO
 * &#064;Date 2025/7/12 17:34
 * &#064;Created MuJue
 */
public class Program extends User{
    private final SymbolTable globalTable;
    private int declareIndex;
    public Program(SymbolTable symbolTable){
        super("-1", IRBasicType.NONE);
        globalTable = symbolTable;
        declareIndex = 0;
    }
    public SymbolTable getSymbolTable() {
        return globalTable;
    }
    @Override
    public void printAssembly(BufferedWriter fout) throws IOException {
        for(Value usee : usees){
            usee.printAssembly(fout);
        }
    }

    public void setDeclareIndex(int declareIndex) {
        this.declareIndex = declareIndex;
    }

    @Override
    public void printName(BufferedWriter fout) throws IOException {
        for(Value value: usees){
            value.printName(fout);
        }
    }

    @Override
    public void printUse(BufferedWriter fout) throws IOException {
        for(Value value: usees){
            value.printUse(fout);
        }
    }

    public void addUsee(Value value, int... cnt){
        if(value instanceof Declare){
            declareIndex++;
        }
        super.addUsee(value);
    }
    public void addUseeAt(Value value, int index){
        usees.add(index, value);
    }

    public SymbolTable getGlobalTable() {
        return globalTable;
    }

    public int getDeclareIndex() {
        return declareIndex;
    }
    /*---------------以下是代码优化部分------------------*/

    /**
     * 删除不可到达的代码
     */
    public void DCEBlock(){
        for(Value value : usees){
            if(value instanceof Function function){
                //首先删除所有的无用指令
                function.DCEBlockInst();
            }
        }
        for(Value value : usees){
            if(value instanceof Function function){
                //然后删除所有不可到达的基本块
                DCETool.deleteBlock(function);
            }
        }
    }

    /**
     * 建立cfg控制流图
     */
    public void buildCfgGraph(){
        for(Value value : usees){
            if(value instanceof Function function){
                function.buildCfgGraph();
            }
        }
    }

    /**
     * 输出CFG
     */
    public void printCFG(BufferedWriter fout) throws IOException {
        fout.write("控制流图如下\n\t");
        for(Value value : usees){
            if(value instanceof Function function){
                fout.write(function.getCfgGraph().debugCFG()+"\n\t");
            }
        }
    }

    /**
     * 生成支配树相关
     */
    public void generateDominantTree(){
        for(Value value : usees){
            if(value instanceof Function function){
                function.buildDominantTree();
            }
        }
    }

    /**
     * 输出支配树相关
     */
    public void printDominantTree(BufferedWriter fout) throws IOException {
        fout.write("支配树如下\n\t");
        for(Value value : usees){
            if(value instanceof Function function){
                fout.write("函数: "+function.getName()+"\n\t\t");
                fout.write(function.getDominateTree().printDominantTree()+"\n\t");
            }
        }
    }

    /**
     * 活跃变量分析
     */
    public void activeVarAnalysis(){
        for(Value value : usees){
            if(value instanceof Function function){
                function.activeVarAnalysis();
            }
        }
    }

    /**
     * 输出活跃变量分析文件
     */
    public void printActiveVarAnalysis(BufferedWriter fout) throws IOException {
        fout.write("活跃变量分析如下\n\t");
        for(Value value : usees){
            if(value instanceof Function function){
                fout.write("函数: "+function.getName()+"\n\t\t");
                function.getActiveVarAnalysis().printActiveVarAnalysis(fout);
            }
        }
    }

    /**
     * 输出活跃变量分析文件
     */
    public void printActiveVarAnalysisInst(BufferedWriter fout) throws IOException{
        fout.write("针对语句级别的活跃变量分析如下\n\t");
        for(Value value : usees){
            if(value instanceof Function function){
                fout.write("函数: "+function.getName()+"\n\t\t");
                function.printActiveVarAnalysisInst(fout);
            }
        }
    }

    /**
     * 进行GVN
     */
    public void GVNOptimize(){
        GlobalVarLocalize globalVarLocalize = new GlobalVarLocalize(this,true);
        globalVarLocalize.runOptimize();
        GlobalCommandMoving globalCommandMoving = new GlobalCommandMoving(this,true);
        globalCommandMoving.runOptimize();
        GlobalVarNumbering globalVarNumbering = new GlobalVarNumbering(this,true);
        System.out.println("GVN Number: " + globalVarNumbering.runOptimize());
    }

    /**
     * 获取所有函数
     */
    public List<Function> getFunctions(){
        List<Function> functions = new ArrayList<>();
        for(Value value : usees){
            if(value instanceof Function function){
                functions.add(function);
            }
        }
        return functions;
    }

    /**
     * 对于基本块只有一个跳转指令，且自己只有一个后继，
     * 那么直接让前面的基本块跳到后继即可
     */
    public void deleteUCBlock(){
        List<Function> functions = getFunctions();
        for(Function function : functions){
            function.deleteUCBlock();
        }
    }

    /**
     * 消phi
     */
    public void deletePhi(){
        for(Value value : usees){
            if(value instanceof Function function){
                function.deletePhiInst();
            }
        }
    }
}
