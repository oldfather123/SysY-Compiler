package Utils;

import Driver.Config;
import IR.IRModule;
import IR.Value.*;
import IR.Value.Instructions.Instruction;
import IR.Value.Instructions.Move;
import Utils.DataStruct.IList;

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;

public class IRDump {
    private static  BufferedWriter out;
//    static {
//        try {
//            out = new BufferedWriter(new FileWriter(Config.iroutFile));
//        } catch (IOException e) {
//            throw new RuntimeException(e);
//        }
//    }

    private static void DumpGlobalVar(GlobalVar globalVar) throws IOException {
        out.write(globalVar.toInstString());
        out.write("\n");
    }

    private static void DumpInstruction(Instruction inst) throws IOException {
        out.write(inst.getInstString() + "\n");
    }

    private static void DumpBasicBlock(BasicBlock bb) throws IOException {
        String bbName = bb.getName();
        out.write(bbName + ":\n");
        IList<Instruction, BasicBlock> insts = bb.getInsts();
        for(IList.INode<Instruction, BasicBlock> instNode : insts){
            out.write("\t");
            DumpInstruction(instNode.getValue());
        }
    }

    private static void DumpFunction(Function function) throws IOException {
        out.write("define ");
        if(function.getType().isIntegerTy()){
            out.write("i32 ");
        }
        else if(function.getType().isFloatTy()){
            out.write("float ");
        }
        else out.write("void ");

        out.write(function.getName() + "(");

        ArrayList<Argument> args = function.getArgs();
        for(int i = 0; i < args.size(); i++){
            out.write(args.get(i).toString());
            if(i != args.size() - 1) out.write(", ");
        }

        out.write(") {\n");
        IList<BasicBlock, Function> basicBlocks = function.getBbs();

        for(IList.INode<BasicBlock, Function> bbNode : basicBlocks){
            DumpBasicBlock(bbNode.getValue());
        }

        out.write("}\n");

    }

    //  Name BasicBlock, Inst to let shit llvm run my damn program.
    private static void RenameFunction(Function function){
        int nowNum = 0;
        int blockNum = 0;

        ArrayList<Argument> args = function.getArgs();
        for(Argument arg : args){
            arg.setName("%" + nowNum++);
        }

        IList<BasicBlock, Function> basicBlocks = function.getBbs();
        for (IList.INode<BasicBlock, Function> bbNode : basicBlocks) {
            BasicBlock basicBlock = bbNode.getValue();
            basicBlock.setName("b" + blockNum++);
            IList<Instruction, BasicBlock> instructions = basicBlock.getInsts();
            for (IList.INode<Instruction, BasicBlock> instNode : instructions) {
                Instruction inst = instNode.getValue();
                if(inst instanceof Move)
                {
                    if (inst.hasName() ) {
                        //�����һ����Ҫ���ֵ�moveָ��
                        //��Ҫ���ֵ������û��pair����pair����֮ǰ���ֵ�pair�й�����
                        inst.setName("%" + nowNum++);
                        if(((Move)inst).pair.size()!=0)
                        {
                            for(Move x : ((Move)inst).pair)
                            {
                                x.setName("%" +( nowNum-1));
                                x.hasname=false;
                            }

                        }
                    }
                }else
                {
                    if (inst.hasName() ) {
                        inst.setName("%" + nowNum++);
                    }
                }


            }
        }
    }

    private static void DumpLib() throws IOException {
        out.write("declare void @memset(i32*, i32, i32)\n");
        out.write("declare i32 @getint()\n");
        out.write("declare i32 @getch()\n");
        out.write("declare i32 @getfarray(float*)\n");
        out.write("declare void @putint(i32)\n");
        out.write("declare void @putfarray(i32, float*)\n");
        out.write("declare void @_sysy_starttime(i32)\n");
        out.write("declare void @_sysy_stoptime(i32)\n");
        out.write("declare i32 @getarray(i32*)\n");
        out.write("declare void @putarray(i32, i32*)\n");
        out.write("declare void @putfloat(float)\n");
        out.write("declare void @putch(i32)\n");
        out.write("declare float @getfloat()\n");
        out.write("declare i32 @parallel_start()\n");
        out.write("declare void @parallel_end(i32)\n");
    }

    public static void DumpModule(IRModule irModule) throws IOException {
        DumpModule(irModule, Config.iroutFile);
    }
    public static void DumpModule(IRModule irModule,String filename) throws IOException {
        out = new BufferedWriter(new FileWriter(filename));
        DumpLib();
        ArrayList<GlobalVar> globalVars = irModule.getGlobalVars();
        for(GlobalVar globalVar : globalVars){
            DumpGlobalVar(globalVar);
        }

        ArrayList<Function> functions = irModule.getFunctions();
        for (Function function : functions) {
            if(!Config.noRename) {
                RenameFunction(function);
            }
            DumpFunction(function);
            out.write("\n");
        }
        out.close();
    }
}
