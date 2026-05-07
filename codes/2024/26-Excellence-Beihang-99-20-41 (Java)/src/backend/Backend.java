package backend;

import backend.component.ObjModule;
import backend.process.ParseIr;
import backend.regalloc.GraphColoringAlloc;
import backend.process.RemovePhi;
import ir.IrModule;

import java.io.File;

public class Backend {
    private IrModule irModule;
    private ObjModule objModule;
    private ParseIr parseIr;

    public Backend(IrModule irModule) {
        this.irModule = irModule;
        parseIr = new ParseIr(irModule);
        this.objModule = parseIr.parse();//前后端存储结构转换
    }

    //方便在没有前端的情况下调试
    public Backend(ObjModule objModule) {
        this.objModule = objModule;
        this.irModule = null;
    }

    public ObjModule getModule() {
        return objModule;
    }

    //去除phi指令
    public void removePhi() {
        new RemovePhi().run(objModule);
    }

    public void regAlloc() {
        new GraphColoringAlloc(objModule).run();
    }

    public void allocateReg() {

    }

    public void emitBackend(String filePath) {
        if (filePath != null) {
            if (filePath.equals("<stdout>")) {
                System.out.println(getModule());
            } else {
                File file = new File(filePath);
                try {
                    java.io.FileWriter writer = new java.io.FileWriter(file);
                    writer.write(getModule().toString());
                    writer.close();
                } catch (java.io.IOException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    public void setExit() {
        parseIr.setExit();
    }
}
/*
ObjModule->ArrayList<ObjGlobalVar>+ArrayList<ObjFunction>
ObjFunction->LinkedList<ObjBlock>
ObjBlock->LinkedList<ObjInstr>
ObjInstr->type+ObjOperand

代码->全局变量+函数
函数->基本块
基本块->指令
指令->类型+操作数
*/
