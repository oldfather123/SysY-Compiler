package front.AST.Stmt;

import front.AST.Node;
import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.Instruction.GetElementPtr;
import mid.IntermediatePresentation.Instruction.Store;
import mid.IntermediatePresentation.Value;
import mid.IntermediatePresentation.ValueType;
import utils.type.SyntaxType;

import java.util.ArrayList;

// AssignStmt ==> LVal '=' Exp ';'
public class AssignStmt extends Stmt {

    public AssignStmt(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }

    public Value toIR() {
        Value lval = children.get(0).toIR();
        Value exp = children.get(2).toIR();

        if (!(lval instanceof GetElementPtr)) {
            if (exp instanceof ConstNumber n) {
                symbolTableManager.setVal(getTokenValue(), lval.isFloat() ? n.getVal().floatValue() :
                        n.getVal().intValue());
            } else {
                symbolTableManager.setVal(getTokenValue(), null);
            }
        }
        boolean isInt = (lval.getRefType() == ValueType.I32);
        return new Store(exp.withType(isInt), lval);
    }
}
