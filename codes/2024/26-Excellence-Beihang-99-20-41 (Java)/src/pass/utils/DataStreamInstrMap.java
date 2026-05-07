package pass.utils;

import ir.IrInstr;
import ir.IrModule;
import ir.value.BasicBlock;
import ir.value.Function;
import ir.value.Value;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.LinkedList;

public class DataStreamInstrMap {

    // 后面两个参数传进去后是要被修改的
    public static HashMap<Integer, ArrayList<Integer>> getDataStreamMap(
            Function function,HashMap<Integer,String> InstrToBlock,
            HashMap<String,IrInstr> ValueToInstr,
            HashMap<String,BasicBlock> blockMap) {
        HashMap<Integer, ArrayList<Integer>> dataStreamMap = new HashMap<>();
        LinkedList<BasicBlock> basicBlocks = function.getBasicBlocks();
        initialize(InstrToBlock, ValueToInstr, basicBlocks, blockMap);
        for (BasicBlock block : basicBlocks) {
            LinkedList<IrInstr> instrs = block.getInstrs();
            for (IrInstr instr : instrs) {
                ArrayList<Value> dependentValues = instr.getDependentValues();
                if (dependentValues != null) {
                    ArrayList<Integer> dependentInstrs = new ArrayList<>();
                    for (Value value : dependentValues) {
                        if (ValueToInstr.containsKey(value.getName())) {
                            dependentInstrs.add(ValueToInstr.get(value.getName()).getGlobalInstrId());
                        }
                    }
                    if (dataStreamMap.containsKey(instr.getGlobalInstrId())) {
                        dataStreamMap.get(instr.getGlobalInstrId()).addAll(dependentInstrs);
                    } else {
                        dataStreamMap.put(instr.getGlobalInstrId(), dependentInstrs);
                    }
                }
            }
        }
        return dataStreamMap;
    }

    public static void initialize(HashMap<Integer, String> InstrToBlock, HashMap<String, IrInstr> ValueToInstr,
                                  LinkedList<BasicBlock> basicBlocks, HashMap<String, BasicBlock> blockMap) {
        for (BasicBlock block : basicBlocks) {
            blockMap.put(block.getName(), block);
            LinkedList<IrInstr> instrs = block.getInstrs();
            for (IrInstr instr : instrs) {
                if (instr.getInitialValue() != null) {
                    ValueToInstr.put(instr.getInitialValue().getName(), instr);
                }
                InstrToBlock.put(instr.getGlobalInstrId(), block.getName());
            }
        }
    }
}
