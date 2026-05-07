package midend;

import midend.LLVMType.FloatType;
import midend.LLVMType.IntegerType;
import midend.LLVMType.VoidType;
import midend.instr.CallInstr;
import midend.instr.Instruction;
import midend.instr.PhiInstr;
import midend.instr.RetInstr;

import java.io.*;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.LinkedList;

public class Module {
    private ArrayList<Function> functions;
    private BufferedWriter output;
    private ArrayList<GlobalVar> globalVars;

    public Module(ArrayList<GlobalVar> globalVars) {
        this.functions = new ArrayList<>();
        this.globalVars = globalVars;
    }

    public void addFunction(Function function) {
        this.functions.add(function);
    }

    public void outputLLVM(String file) throws IOException {
        output = new BufferedWriter(new FileWriter(file));

        outputLibFunc();

        for (GlobalVar globalVar : globalVars) {
            output.write(globalVar.toString());
            output.write("\n");
        }

        for (Function function : functions) {
            output.write("define ");
            output.write("dso_local ");
            if (function.getRetType() == IntegerType.i32) {
                output.write("i32 ");
            } else if (function.getRetType() == FloatType.f32) {
                output.write("float ");
            } else if (function.getRetType() == VoidType.voidType) {
                output.write("void ");
            }
            output.write(function.getName() + "(");
            ArrayList<Param> params = function.getParams();
            for (int count = 0; count < params.size(); count++) {
                output.write(params.get(count).toString());
                output.write(" " + params.get(count).getName());
                if (count != params.size() - 1) {
                    output.write(", ");
                }
            }
            output.write("){\n");
            LinkedList<BasicBlock> blockList = function.getBlockList();
            for (BasicBlock block : blockList) {
                outputBasicBlock(block);
                output.write("\n");
            }
            output.write("}\n");
        }
        output.close();
    }

    public void outputBasicBlock(BasicBlock block) throws IOException {
        String name = block.getName();
        output.write(name + ":\n");
        for (Instruction instruction : block.getInstrList()) {
            output.write("\t");
            output.write(instruction.getInstr());
        }
    }

    public void outputLibFunc() throws IOException {
        output.write("declare i32 @getint()\n");
        output.write("declare i32 @getch()\n");
        output.write("declare float @getfloat()\n");
        output.write("declare i32 @getarray(i32*)\n");
        output.write("declare i32 @getfarray(float*)\n");
        output.write("declare void @putint(i32)\n");
        output.write("declare void @putch(i32)\n");
        output.write("declare void @putfloat(float)\n");
        output.write("declare void @putarray(i32, i32*)\n");
        output.write("declare void @putfarray(i32, float*)\n");
        output.write("declare void @_sysy_starttime(i32)\n");
        output.write("declare void @_sysy_stoptime(i32)\n");
        output.write("declare void @memset(i32*, i32, i32)\n");
    }

    public ArrayList<GlobalVar> getGlobalVars() {
        return globalVars;
    }

    public ArrayList<Function> getFunctions() {
        return functions;
    }

    public void buildCallRelation() {
        for (Function function : functions) {
            function.getCalledList().clear();
            function.getBeCalledList().clear();
        }
        for (Function function : functions) {
            LinkedList<BasicBlock> basicBlocks = function.getBlockList();
            for (BasicBlock block : basicBlocks) {
                LinkedList<Instruction> instructions = block.getInstrList();
                for (Instruction instruction : instructions) {
                    if (instruction instanceof CallInstr) {
                        Function callFunction = ((CallInstr) instruction).getFunction();
                        if (!callFunction.isLib() && !callFunction.getBeCalledList().contains(function)) {
                            callFunction.getBeCalledList().add(function);
                        }
                        if (!callFunction.isLib() && !function.getCalledList().contains(callFunction)) {
                            function.getCalledList().add(callFunction);
                        }
                    }
                }
            }
        }
    }

    public void removeUseLessFunc() {
        ArrayList<Function> functionsToDelete = new ArrayList<>();
        for (Function function : functions) {
            if (shouldBeDeleted(function)) {
                functionsToDelete.add(function);
            }
        }
        for (GlobalVar globalVar : globalVars) {
            removeReferences(globalVar, functionsToDelete);
        }
        functions.removeAll(functionsToDelete);
    }

    private boolean shouldBeDeleted(Function function) {
        return !function.getName().equals("@main") && function.getBeCalledList().isEmpty();
    }

    private void removeReferences(GlobalVar globalVar, ArrayList<Function> functionsToDelete) {
        ArrayList<Use> uses = new ArrayList<>(globalVar.getUseList());
        for (Use use : uses) {
            Instruction instruction = (Instruction) use.getUser();
            Function parentFunction = instruction.getBasicBlock().getParentFunc();
            if (functionsToDelete.contains(parentFunction)) {
                globalVar.removeUserFromUse(instruction);
            }
        }
    }


    public void setExitBlock() {
        for (Function function : functions) {
            for (BasicBlock block : function.getBlockList()) {
                if (block.getInstrList().getLast() instanceof RetInstr) {
                    function.setExitBlock(block);
                    break;
                }
            }
        }
    }

        public void maintainPhiBlocks() {
        for (Function function : functions) {
            for (BasicBlock block : function.getBlockList()) {
                if (block.getInstrList().getFirst() instanceof PhiInstr) {
                    ArrayList<BasicBlock> phiBlocks = ((PhiInstr) block.getInstrList().getFirst()).getPreBlockList();
                    ArrayList<BasicBlock> preBlocks = block.getPreBlock();
                    for (int count = 0; count < preBlocks.size(); count++) {
                        if (preBlocks.get(count) != phiBlocks.get(count)) {
                            preBlocks.set(count, phiBlocks.get(count));
                        }
                    }
                }
            }
        }
    }
//    public void maintainPhiBlocks() {
//        for (Function function : functions) {
//            for (BasicBlock block : function.getBlockList()) {
//                if (block.getInstrList().getFirst() instanceof PhiInstr) {
//                    PhiInstr phiInstr = (PhiInstr) block.getInstrList().getFirst();
//                    ArrayList<BasicBlock> phiBlocks = phiInstr.getPreBlockList();
//                    ArrayList<BasicBlock> preBlocks = block.getPreBlock();
//                    Iterator<BasicBlock> preBlockIterator = preBlocks.iterator();
//                    Iterator<BasicBlock> phiBlockIterator = phiBlocks.iterator();
//                    while (preBlockIterator.hasNext() && phiBlockIterator.hasNext()) {
//                        BasicBlock preBlock = preBlockIterator.next();
//                        BasicBlock phiBlock = phiBlockIterator.next();
//                        if (preBlock != phiBlock) {
//                            preBlockIterator.remove();
//                            preBlocks.add(phiBlock);
//                        }
//                    }
//                }
//            }
//        }
//    }


    public void addGlobalVar(GlobalVar globalVar) {
        this.globalVars.add(globalVar);
    }
}
