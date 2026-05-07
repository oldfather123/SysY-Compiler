package midend.DCE;

import frontend.ir.Value;
import frontend.ir.instr.otherop.CallInstr;
import frontend.ir.structure.Function;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/*
删除无用参数
List<String> newList = new ArrayList<>();
for (int i = 0; i < list.size(); i++) {
    if (!shouldDelete(i)) newList.add(list.get(i));
}
 */

public class RemoveDeadArgument {
    public static void execute(List<Function> functions) {
        for(Function f : functions) {
            Set<Integer> shouldKeep =  new HashSet<>();
            List<Function.FParam> newFParams = new ArrayList<>();
            for(int i = 0; i < f.getFParams().size(); i++) {
                if(!f.getFParams().get(i).getUserList().isEmpty()) {
                    shouldKeep.add(i);
                    newFParams.add(f.getFParams().get(i));
                }
            }
            if(shouldKeep.size() == f.getFParams().size()) {continue;}

            f.setFParams(newFParams);

            List<Value> newRParams;
            for(CallInstr callInstr: f.getCallInstrList()) {
                newRParams = new ArrayList<>();
                for(int i = 0; i < callInstr.getRParamSize(); i++) {
                    if(shouldKeep.contains(i)) {
                        newRParams.add(callInstr.getRParams().get(i));
                    }
                }
                callInstr.setRParams(newRParams);
            }
        }
    }
}
