package ir.pass.opt;

import ir.Value;
import ir.instr.Instr;
import ir.pass.analyze.SideEffectAnalyzer;
import ir.instr.Call;
import ir.value.ConstNumber;
import ir.value.Module;
import ir.value.user.Function;

import java.util.Iterator;
import java.util.LinkedHashSet;

public class CRP implements Pass{
    //Call Replace
    //Only in the same block cuz we're out of time
    //The right way to handle it is Fun GVN & GCM
    //早知道多参考一下代码了,太消愁了
    @Override
    public void run(Module module) {
        SideEffectAnalyzer.calSideEffect(module,true);
        for(var function:module.functions.values()){
            if(function.blocks.size()>0) CRPForFunction(function);
        }
    }

    public void CRPForFunction(Function function){
        LinkedHashSet<Instr> toDelete = new LinkedHashSet<>();
        LinkedHashSet<Instr> vis = new LinkedHashSet<>();
        for(var BB:function.blocks){
            for(var instr:BB.getInsts()){
                if(toDelete.contains((Instr)instr)) continue;
                if(instr instanceof Call&&!((Call)instr).getFunction().hasSideEffect){
                    System.out.println(instr+":");
                    vis.add((Instr) instr);
                    for(var other:BB.getInsts()){
                        if(toDelete.contains((Instr)other)) continue;
                        if(vis.contains((Instr)other)) continue;
                        if(other instanceof Call&&((Call)other).getFunction().equals(((Call)instr).getFunction())){
                            if(((Call) instr).getOperands().size()!=((Call) other).getOperands().size()) continue;
                            System.out.println(other);
                            boolean check  = true;
                            for(int i=0;i<((Call) instr).getOperands().size();++i){
                                Value a = ((Call) instr).getOperands().get(i);
                                Value b = ((Call) other).getOperands().get(i);
                                System.out.println(a);
                                System.out.println(b);
                                if(!a.getType().equals(b.getType())){
                                    check = false;
                                    break;
                                }
                                if(a instanceof ConstNumber && b instanceof ConstNumber) {
                                    check = ((ConstNumber)a).value.equals(((ConstNumber)b).value);
                                    if(!check) break;
                                }
                                else{
                                    check = a.equals(b);
                                    if(!check) break;
                                }
                            }
                            if(check){
                                toDelete.add((Instr) other);
                                ((Call) other).delete();
                                other.replaceAllUsers(instr);
                            }
                        }
                    }
                }
            }
        }
        for(var basicBlock:function.blocks){
                Iterator<Value> it = basicBlock.getInsts().iterator();
                while (it.hasNext()) {
                    Instr instr = (Instr) it.next();
                    if (toDelete.contains(instr))
                    {
                        instr.delete();
                        it.remove();
                    }
                }
            }
        }

}
