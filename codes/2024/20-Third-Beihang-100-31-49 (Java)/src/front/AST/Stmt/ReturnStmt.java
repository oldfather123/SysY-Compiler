package front.AST.Stmt;

import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.Instruction.Load;
import mid.IntermediatePresentation.Instruction.Ret;
import mid.IntermediatePresentation.Value;
import mid.IntermediatePresentation.ValueType;
import front.AST.Exp.Exp;
import front.AST.Node;
import utils.type.SyntaxType;

import java.util.ArrayList;

// ReturnStmt ==> 'return' [ Exp ] ';'
public class ReturnStmt extends Stmt {

    public ReturnStmt(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }

    public Ret toIR() {
        if (children.get(1) instanceof Exp exp) {
            Value retVal = exp.toIR();
            ValueType retType = irManager.getCurFunction().getType();
            boolean isInt = retType == ValueType.I32;
            if (retVal.isPointer()) {
                retVal = new Load(irManager.declareTempVar(), retVal);
            }
            if (retVal instanceof ConstNumber constNumber) {
                Number val = constNumber.getVal();
                if (isInt) {
                    retVal = new ConstNumber(val.intValue());
                } else {
                    retVal = new ConstNumber(val.floatValue());
                }
            }
            return new Ret(retVal.withType(isInt), isInt);
        } else {
            return new Ret();
        }
    }

}
