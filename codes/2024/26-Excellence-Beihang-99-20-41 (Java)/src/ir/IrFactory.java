package ir;

import ir.traversal.SymbolTable;
import ir.traversal.SymbolTable.*;
import ir.value.BasicBlock;
import ir.value.Function;

import java.io.File;

public class IrFactory {
    private final IrModule module;
    private SymbolTable.SymbolId lastFunc;
    private String insertPoint;

    public IrFactory(String moduleName) {
        module = new IrModule(moduleName);
        lastFunc = null;
        insertPoint = null;
    }

    public void externFunction(SymbolTableEntry entry) {
        module.addExternFunction(entry);
    }

    public void declareGlobalVariable(SymbolTableEntry entry) {
        module.addGlobalVariable(entry);
    }

    public void declareGlobalConstant(SymbolTableEntry entry) {
        module.declareGlobalConstant(entry);
    }

    public void declareFunction(SymbolTableEntry entry) {
        lastFunc = entry.id();
        module.addFunction(entry);
    }

    public String registerBlock() {
        return module.registerBlock();
    }

    public Function getLastFunction() {
        return module.getLastFunction();
    }

    public String registerBlock(IrInstr comment) {
        String result = module.registerBlock();
        module.insertBlockInstr(result, comment);
        return result;
    }

    public void setInsertPoint(String blockId) {
        insertPoint = blockId;
    }

    public String getInsertPoint() {
        return insertPoint;
    }

    public void insertInstr(IrInstr instr) {
        module.insertBlockInstr(insertPoint, instr);
    }

    public boolean hasTerminator(String block) {
        return module.hasTerminator(block);
    }

    public BasicBlock getBlock(String blockId) {
        return module.getBlock(blockId);
    }

    public void emitIr(String filePath) {
        if (filePath == null) {
            System.out.println(module);
        } else {
            // 写入到文件中
            File file = new File(filePath);
            String result = module.toString();
            try {
                java.io.FileWriter writer = new java.io.FileWriter(file);
                writer.write(result);
                writer.close();
            } catch (java.io.IOException e) {
                e.printStackTrace();
            }
        }
    }

    public IrModule getModule() {
        return module;
    }
}
