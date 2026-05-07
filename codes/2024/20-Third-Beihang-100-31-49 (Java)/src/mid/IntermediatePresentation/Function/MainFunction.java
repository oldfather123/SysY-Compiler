package mid.IntermediatePresentation.Function;

import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.Instruction.Instruction;
import mid.IntermediatePresentation.ValueType;
public class MainFunction extends Function {
    public MainFunction() {
        super("@main", new Param(), ValueType.I32);
        IRManager.getModule().setMainFunction(this);
    }
}
