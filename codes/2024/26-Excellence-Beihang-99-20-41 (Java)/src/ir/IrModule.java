package ir;

import ir.instr.PhiInstr;
import ir.instr.TermInstr;
import ir.traversal.SymbolTable.*;
import ir.value.*;

import java.util.ArrayList;

public class IrModule {
    private String moduleName;
    public ArrayList<Function> functions;
    public ArrayList<Variable> globalVariables;
    public ArrayList<Constant> globalConstants;
    public ArrayList<Function> externFunction;

    public IrModule(String moduleName) {
        functions = new ArrayList<>();
        globalVariables = new ArrayList<>();
        externFunction = new ArrayList<>();
        globalConstants = new ArrayList<>();
        this.moduleName = moduleName;
    }

    public void removeFunction(Function function) {
        functions.remove(function);
    }

    public void addExternFunction(SymbolTableEntry entry) {
        Function function = (Function) entry.value();
        externFunction.add(function);
    }

    public void addGlobalVariable(SymbolTableEntry entry) {
        Variable variable = (Variable) entry.value();
        globalVariables.add(variable);
    }

    public void declareGlobalConstant(SymbolTableEntry entry) {
        Constant constant = (Constant) entry.value();
        globalConstants.add(constant);
    }

    public void addFunction(SymbolTableEntry entry) {
        Function function = (Function) entry.value();
        functions.add(function);
    }

    public String registerBlock() {
        return functions.get((functions.size()) - 1).addBasicBlock();
    }

    public void insertBlockInstr(String blockId, IrInstr instr) {
        for (Function function : functions) {
            function.insertBlockInstr(blockId, instr);
        }
    }

    public boolean hasTerminator(String blockId) {
        for (Function function : functions) {
            if (function.hasTerminator(blockId)) {
                return true;
            }
        }
        return false;
    }

    public BasicBlock getBlock(String blockId) {
        for (Function function : functions) {
            BasicBlock block = function.getBlock(blockId);
            if (block != null) {
                return block;
            }
        }
        return null;
    }

    public Function getLastFunction() {
        return functions.get((functions.size()) - 1);
    }

    public ArrayList<Function> getFunctions() {
        return functions;
    }

    public ArrayList<Constant> getGlobalConstants() {
        return globalConstants;
    }

    public ArrayList<Function> getExternFunction() {
        return externFunction;
    }

    public ArrayList<Variable> getGlobalVariables() {
        return globalVariables;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("; module_name: \t\t").append(moduleName).append("\n");
        sb.append("; global variables: \n");
        for (Variable variable : globalVariables) {
            sb.append(variable.getMangle()).append(" = global ").append(variable.getType())
                    .append(" ").append(variable.getDisplayValue()).append("\t; ")
                    .append(variable.getName()).append("\n");
        }
        sb.append("; global constants: \n");
        for (Constant constant : globalConstants) {
            sb.append(constant.getMangle()).append(" = constant ").append(constant.getType())
                    .append(" ").append(constant.getDisplayValue()).append("\t; ")
                    .append(constant.getName()).append("\n");
        }
        sb.append("; extern functions: \n");
        for (Function function : externFunction) {
            sb.append("declare ").append(function.getReturnType()).append(" ").append(
                    function.getName()).append("(").append(function.getArgumentsString(false)).append(")\n");
        }
        sb.append("; functions: \n");
        for (Function function : functions) {
            sb.append("define ").append(function.getReturnType()).append(" ").append(
                    function.getName()).append("(").append(function.getArgumentsString(true)).append(")\n");
            sb.append("{\n");
            sb.append(function.getStatementString());
            sb.append("}\n");
        }
        return sb.toString();
    }

    public void emitDotBlock(String filePath) {
        StringBuilder sb = new StringBuilder();
        sb.append("digraph module {\n");
        for (Function function : functions) {
            sb.append("  subgraph cluster_").append(function.getName().substring(1)).append(" {\n");
            sb.append("    graph [label=\"").append(function.getName()).append("\";];\n");
            for (BasicBlock block : function.getBasicBlocks()) {
                sb.append("    ").append(block.getName()).append(" [shape=record, label=\"{").append(block.getName()).append("|");
                var phis = block.getInstrs().stream().filter(instr -> instr instanceof PhiInstr).toList();
                for (IrInstr instr : phis) {
                    sb.append(instr).append("\\l");
                }
                if (!phis.isEmpty()) {
                    sb.append("|");
                }
                var calls = block.getInstrs().stream().filter(instr -> instr instanceof TermInstr && ((TermInstr) instr).op.equals(IrInstr.OpCode.CALL)).toList();
                for (IrInstr instr : calls) {
                    sb.append(instr).append("\\l");
                }
                if (!calls.isEmpty()) {
                    sb.append("|");
                }
                sb.append(block.getTerminator().toString()).append("\\l");
                sb.append("}\"]\n");
            }
            sb.append("\n");
            for (BasicBlock block : function.getBasicBlocks()) {
                for (BasicBlock successor : block.getSuccessors()) {
                    sb.append("    ").append(block.getName()).append(" -> ").append(successor.getName()).append("\n");
                }
            }
            sb.append("  }\n");
        }
        sb.append("}\n");
        try {
            java.io.FileWriter fw = new java.io.FileWriter(filePath);
            fw.write(sb.toString());
            fw.close();
        } catch (java.io.IOException e) {
            e.printStackTrace();
        }
    }
}
