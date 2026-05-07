package Backend.component;

import Backend.instruction.ObjInstr;
import Backend.operand.ObjReg;
import IR.Value.BasicBlock;
import IR.Value.Function;
import IR.Value.Instructions.Instruction;
import IR.Value.Value;
import Utils.DataStruct.IList;

import java.util.ArrayList;

public class ObjBlock {

    private static int index = 0;
    private String name;
    private IList.INode<ObjBlock, ObjFunction> node;
    private IList<ObjInstr, ObjBlock> instrs;
    private ObjBlock trueBlock = null, falseBlock = null;
    public final ArrayList<ObjReg> liveIns=new ArrayList<>();
    public final ArrayList<ObjReg> liveOuts=new ArrayList<>();
    public final ArrayList<ObjReg> Use=new ArrayList<>();
    public final ArrayList<ObjReg> Def=new ArrayList<>();
    public final ArrayList<ObjReg> livePIns=new ArrayList<>();
    public final ArrayList<ObjReg> livePOuts=new ArrayList<>();
    public final ArrayList<ObjReg> PUse=new ArrayList<>();
    public final ArrayList<ObjReg> PDef=new ArrayList<>();
    public final ArrayList<ArrayList<ObjReg>> LocalInterfere = new ArrayList<>();
    public int depth;

    private ArrayList<ObjBlock> preBlocks, nxtBlocks;
    public ObjBlock(String name) {
        this.name = name;
        this.node = new IList.INode<>(this);
        this.instrs = new IList<>(this);
        trueBlock = null;        falseBlock = null;

        preBlocks = new ArrayList<>();
        nxtBlocks = new ArrayList<>();
    }

    public void addPreBlock(ObjBlock objBlock) {
        preBlocks.add(objBlock);
    }
    public void addNxtBlock(ObjBlock objBlock) {
        nxtBlocks.add(objBlock);
    }
    public ArrayList<ObjBlock> getPreBlocks() {
        return preBlocks;
    }
    public ArrayList<ObjBlock> getNxtBlocks() {
        return nxtBlocks;
    }

    public ObjBlock getTrueBlock() {
        return trueBlock;
    }

    public ObjBlock getFalseBlock() {
        return falseBlock;
    }
    public void printBbDetail()
    {
        System.out.println("===========");
        System.out.println("\t"+this.getName());
//        System.out.print("PREV:   [");
//        for(ObjBlock o :this.getPreBlocks())
//        {
//            System.out.print(o.getName()+", ");
//        }
//        System.out.println("]");
//        System.out.print("NEXT:   [");
//        for(ObjBlock o :this.getNxtBlocks())
//        {
//            System.out.print(o.getName()+", ");
//        }
//        System.out.println("]");
//        for( IList.INode<ObjInstr, ObjBlock> inst : this.instrs)
//        {
//            System.out.println(inst.getValue().toString());
////            System.out.print("DEF: ");
////            System.out.println(inst.getValue().regDef);
////            System.out.print("USE: ");
////            System.out.println(inst.getValue().regUse);
//            System.out.println(inst.getValue().livein);
//        }
//        System.out.println("DEF:    "+Def);
//        System.out.println("USE:    "+Use);
        System.out.println("IN:    "+liveIns);
        System.out.println("OUT:    "+liveOuts);
//        System.out.println("LOCAL INTERF");
//        for(ArrayList<ObjReg> x : LocalInterfere)
//        {
//            System.out.println(x);
//        }
        System.out.println("===========");
    }
    public void setTrueBlock(ObjBlock trueBlock) {
        this.trueBlock = trueBlock;
    }

    public void setFalseBlock(ObjBlock falseBlock) {
        this.falseBlock = falseBlock;
    }

    public IList<ObjInstr, ObjBlock> getInstrs() {
        return instrs;
    }

    public IList.INode<ObjBlock, ObjFunction> getNode(){
        return node;
    }
    public void addInstr(ObjInstr instr) {
        instr.getNode().insertListEnd(instrs);
    }
    public void addInstrHead(ObjInstr instr) {
        instr.getNode().insertListHead(instrs);
    }
    public void print() {
         System.out.println(name + ":");
        for(IList.INode<ObjInstr, ObjBlock> instr : instrs) {
            ObjInstr i = instr.getValue();
            System.out.println("\t" + i.toString());
        }
    }

    public ArrayList<ObjReg> getLiveIns(){
        return liveIns;
    }

    public ArrayList<ObjReg> getLiveOuts(){
        return liveOuts;
    }

    public ArrayList<ObjReg> getDef(){
        return Def;
    }

    public ArrayList<ObjReg> getUse(){
        return Use;
    }

    public String getName() {
        return name;
    }
    public void setName(String name) {
        this.name = name;
    }
}
