package backend.risc_v.entity;

import backend.risc_v.entity.data.RiscVData;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.ArrayList;

public class RiscVProgram {
    private final ArrayList<RiscVMacro> macros;
    private final ArrayList<RiscVExtern> declares;
    private final ArrayList<RiscVData> globalVars;
    private final ArrayList<RiscVFunction> functions;
    private RiscVFunction mainFunction;

    public RiscVProgram() {
        this.macros = new ArrayList<>();
        this.declares = new ArrayList<>();
        this.globalVars = new ArrayList<>();
        this.functions = new ArrayList<>();
        this.mainFunction = new RiscVFunction("main");
    }

    public void addMacro(RiscVMacro macro){macros.add(macro);}
    public void addDeclare(RiscVExtern declare) {
        declares.add(declare);
    }

    public void addGlobalVar(RiscVData globalVar) {
        globalVars.add(globalVar);
    }

    public void addFunction(RiscVFunction function) {
        functions.add(function);
    }

    public void setMainFunction(RiscVFunction mainFunction) {
        this.mainFunction = mainFunction;
    }

    public void printRiscV(BufferedWriter out) throws IOException {
        for(RiscVMacro macro : macros){
            out.write(macro.toString());
            out.newLine();
        }
        for (RiscVExtern declare : declares) {
            out.write(declare.toString());
            out.newLine();
        }
        out.write(".data\n");
        for (RiscVData globalVar : globalVars) {
            out.write(globalVar.toString());
            out.newLine();
        }
        out.write("\n.text\n");
        out.write(".global main\n");
        for (RiscVFunction function : functions) {
            out.write(function.toString());
            out.newLine();
        }
        out.write(mainFunction.toString());

    }

}
