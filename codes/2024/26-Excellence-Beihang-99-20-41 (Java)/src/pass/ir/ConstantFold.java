package pass.ir;

import ir.IrInstr;
import ir.IrModule;
import ir.instr.BinaInstr;
import ir.instr.MoveInstr;
import ir.type.IrType;
import ir.value.Literal;
import ir.value.Value;
import pass.Pass;

import java.util.ArrayList;

import static ir.type.Ty.I32;

public class ConstantFold implements Pass.IrPass {
    @Override
    public void run(IrModule module) {
        for (var func : module.getFunctions()) {
            for (var block : func.getBasicBlocks()) {
                ArrayList<IrInstr> instrsToMaintain = new ArrayList<>();
                for (var instr : block.getInstrs()) {
                    if (!(instr instanceof BinaInstr)) {
                        instrsToMaintain.add(instr);
                        continue;
                    }
                    ArrayList<Value> values = instr.getDependentValues();
                    switch (instr.getOpCode()) {
                        case ADD:
                            if ((values.get(0) instanceof Literal && ((Literal)values.get(0)).asInt() == 0)) {
                                instrsToMaintain.add(new MoveInstr(instr.getInitialValue(),values.get(1)));
                            }
                            else if ((values.get(1) instanceof Literal && ((Literal)values.get(1)).asInt() == 0)) {
                                instrsToMaintain.add(new MoveInstr(instr.getInitialValue(),values.get(0)));
                            }
                            else
                                instrsToMaintain.add(instr);
                            break;
                        case SUB:
                            if ((values.get(1) instanceof Literal && ((Literal)values.get(1)).asInt() == 0)) {
                                instrsToMaintain.add(new MoveInstr(instr.getInitialValue(),values.get(0)));
                            }
                            else
                                instrsToMaintain.add(instr);
                            break;
                        case MUL:
                            if ((values.get(0) instanceof Literal && ((Literal)values.get(0)).asInt() == 1)) {
                                instrsToMaintain.add(new MoveInstr(instr.getInitialValue(),values.get(1)));
                            }
                            else if ((values.get(1) instanceof Literal && ((Literal)values.get(1)).asInt() == 1)) {
                                instrsToMaintain.add(new MoveInstr(instr.getInitialValue(),values.get(0)));
                            }
                            else if ((values.get(0) instanceof Literal && ((Literal)values.get(0)).asInt() == 0) ||
                                    (values.get(1) instanceof Literal && ((Literal)values.get(1)).asInt() == 0)) {
                                instrsToMaintain.add(new MoveInstr(instr.getInitialValue(),Literal.zero(I32)));
                            }
                            else if ((values.get(0) instanceof Literal && ((Literal)values.get(0)).asInt() == 2)) {
                                ((BinaInstr) instr).op = IrInstr.OpCode.ADD;
                                instr.replaceOperand(values.get(0),values.get(1));
                                instrsToMaintain.add(instr);
                            }
                            else if ((values.get(1) instanceof Literal && ((Literal)values.get(1)).asInt() == 2)) {
//                                ((BinaInstr) instr).op = IrInstr.OpCode.ADD;
//                                instr.replaceOperand(values.get(1),values.get(0));
                                IrInstr addInstr = new BinaInstr(values.get(0),values.get(0),IrInstr.OpCode.ADD,instr.getInitialValue());
                                instrsToMaintain.add(addInstr);
                            }
                            else
                                instrsToMaintain.add(instr);
                            break;
                        case SDIV:
                            if (values.get(1) instanceof Literal && ((Literal)values.get(1)).asInt() == 1) {
                                instrsToMaintain.add(new MoveInstr(instr.getInitialValue(),values.get(1)));
                            }
                            else
                                instrsToMaintain.add(instr);
                            break;
                        default:
                            instrsToMaintain.add(instr);
                            break;
                    }
                }
                block.resetInstrs(instrsToMaintain);
            }
        }
    }

    @Override
    public String getName() {
        return "const-fold";
    }
}
