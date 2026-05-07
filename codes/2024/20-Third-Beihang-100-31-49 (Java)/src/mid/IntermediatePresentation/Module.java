package mid.IntermediatePresentation;

import mid.IntermediatePresentation.Function.Function;
import mid.IntermediatePresentation.Function.LibFunction;
import mid.IntermediatePresentation.Function.MainFunction;
import mid.IntermediatePresentation.Instruction.Br;
import mid.IntermediatePresentation.Instruction.GlobalDecl;
import mid.IntermediatePresentation.Instruction.Ret;
import mid.Optimizer.Optimizer;
import mid.Optimizer.RedundancyElim.DeadCode;

import java.util.ArrayList;
import java.util.stream.Collectors;

public class Module extends Value {
    private final ArrayList<GlobalDecl> globalDecls = new ArrayList<>();
    private final ArrayList<ConstString> constStrings = new ArrayList<>();
    private final ArrayList<Function> functions = new ArrayList<>();
    private MainFunction mainFunction = null;

    public Module() {
        super("MODULE", ValueType.NULL);
    }

    public void addGobalDecl(GlobalDecl globalDecl) {
        globalDecls.add(globalDecl);
    }

    public void addConstString(ConstString constString) {
        constStrings.add(constString);
    }

    public void addFunction(Function function) {
        functions.add(function);
    }

    public void setMainFunction(MainFunction mainFunction) {
        this.mainFunction = mainFunction;
    }

    public ArrayList<Function> getFunctions() {
        return functions;
    }

    public MainFunction getMainFunction() {
        return mainFunction;
    }

    public ArrayList<Function> getDecledFunctions() {
        ArrayList<Function> allFunctions = new ArrayList<>(functions);
        allFunctions.add(mainFunction);
        return allFunctions.stream().filter(x -> !(x instanceof LibFunction)).collect(
                Collectors.toCollection(ArrayList::new)
        );
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        for (GlobalDecl globalDecl : globalDecls) {
            sb.append(globalDecl.toString());
        }
        if (globalDecls.size() > 0) {
            sb.append("\n");
        }
        for (ConstString constString : constStrings) {
            sb.append(constString.toString());
        }
        if (constStrings.size() > 0) {
            sb.append("\n");
        }

        for (Function function : functions) {
            sb.append(function.toString());
        }
        if (functions.size() > 0) {
            sb.append("\n");
        }

        sb.append(mainFunction.toString());
        return sb.toString();
    }

    public ArrayList<GlobalDecl> getGlobalDecls() {
        return new ArrayList<>(globalDecls);
    }

    public void removeGlobalDecl(GlobalDecl globalDecl) {
        globalDecls.remove(globalDecl);
    }

    public void check() {
        IRManager.getInstance().setAutoInsert(false);
        for (Function f : getDecledFunctions()) {
            for (BasicBlock b : f.getBlocks()) {
                if (b.getInstructionList().isEmpty() || !(b.getLastInstruction() instanceof Br || b.getLastInstruction() instanceof Ret)) {
                    if (f.isVoid()) {
                        b.addInstruction(new Ret());
                    } else {
                        b.addInstruction(new Ret((new ConstNumber(0)).withType(!f.isFloat()), !f.isFloat()));
                    }
                }
            }
        }
    }
}
