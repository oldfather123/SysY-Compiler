package midend;

import frontend.ir.Value;
import frontend.ir.constant.IRConst;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.memop.AllocaInstr;
import frontend.ir.objecttype.derived.TArray;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import frontend.ir.symbol.ArrayInitVal;
import frontend.ir.symbol.GlbObjPtr;
import frontend.ir.symbol.SymTab;
import frontend.ir.symbol.Symbol;

import java.util.List;

public class LocalArrayExtraction {
    public static void execute(List<Function> functions, SymTab globalSymTab) {
        int cnt = 0;
        
        for (Function function : functions) {
            for (BasicBlock basicBlock : function.getBasicBlockList()) {
                for (Instruction instruction : basicBlock.getInstrList()) {
                    if (instruction instanceof AllocaInstr.ArrayAlloca arrayAlloca) {
                        String ident = "extracted_local_array_" + cnt;
                        cnt++;
                        TArray arrayType = (TArray) arrayAlloca.getReferencedType();
                        Value initVal = arrayAlloca.getArrayInitVal();
                        
                        if (!(initVal instanceof ArrayInitVal arrayInitVal)) {
                            continue;
                        }
                        boolean hasNonConst = false;
                        for (Value innerVal : arrayInitVal.getInitValues()) {
                            if (!(innerVal instanceof IRConst)) {
                                hasNonConst = true;
                                break;
                            }
                        }
                        if (hasNonConst) {
                            continue;
                        }
                        
                        Value pointer = new GlbObjPtr(arrayType, ident);
                        Symbol newSym = new Symbol(ident, false, true, arrayType, initVal, pointer);
                        globalSymTab.addObject(newSym, -1);
                        
                        arrayAlloca.replaceUseWith(pointer);
                        arrayAlloca.removeFromList();
                        
                        Instruction initIns = arrayAlloca.getInitEndIns();
                        while (initIns != arrayAlloca.getInitBeginIns()) {
                            initIns.removeFromList();
                            initIns = (Instruction) initIns.getPrev();
                        }
                        initIns.removeFromList();
                    }
                }
            }
        }
    }
}
