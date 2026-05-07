package ir.pass.opt;

import backend.component.RiscvGlobalFloat;
import backend.component.RiscvGlobalInt;
import ir.Value;
import ir.instr.Alloca;
import ir.instr.Global;
import ir.instr.Instr;
import ir.instr.Store;
import ir.type.*;
import ir.value.*;
import ir.value.Module;
import ir.value.user.Function;

import java.util.ArrayList;

import tools.IRBuilder;
import tools.IRBuilder.*;
import tools.IrRegDispatcher;

import java.util.LinkedHashSet;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;

public class GlobalVariableLocalize implements Pass{
    private LinkedHashMap<ConstVariable, LinkedHashSet<Function>> constGlobal = new LinkedHashMap<>();
    private LinkedHashMap<Variable, LinkedHashSet<Function>> global = new LinkedHashMap<>();

    public GlobalVariableLocalize() {

    }

    @Override
    public void run(Module module) {
        for(ConstVariable constVariable:module.constGlobals.values()) {
            if(constVariable.getType() instanceof ArrayType) {
                continue;
            }
            LinkedHashSet<Function> funcs = new LinkedHashSet<>();
            for(User user:constVariable.allocInst.getUsers()) {
                assert user instanceof Instr;
                assert ((Instr) user).getParent().getUsers().get(0) instanceof Function;
                funcs.add((Function) ((Instr) user).getParent().getUsers().get(0));
            }
            constGlobal.put(constVariable, funcs);
        }
        for(Variable variable:module.globals.values()) {
            if(variable.getType() instanceof ArrayType) {
                continue;
            }
            LinkedHashSet<Function> funcs = new LinkedHashSet<>();
            for(User user:variable.allocInst.getUsers()) {
                assert user instanceof Instr;
                assert ((Instr) user).getParent().getUsers().get(0) instanceof Function;
                funcs.add((Function) ((Instr) user).getParent().getUsers().get(0));
            }
            global.put(variable, funcs);
        }

        for(ConstVariable constVariable: constGlobal.keySet()) {
            if(constGlobal.get(constVariable).size() <= 1) {
                for(Function function:constGlobal.get(constVariable)) {
                    if(!function.name.equals("main")) {
                        break;
                    }
                    Alloca alloca = IRBuilder.buildAlloca1(
                            "global2local"+ constVariable.getName(), constVariable.getType(),
                            function.blocks.get(0), function.getIrRegDispatcher());
                    constVariable.allocInst.replaceAllUsers(alloca);
                    if(constVariable.getType() instanceof FloatType) {
                        function.blocks.get(0).addValueFirst(new Store(alloca,
                                new ConstNumber((Double)(((Global)constVariable.allocInst).getInitval()))
                                , function.blocks.get(0)));
                    } else if(constVariable.getType() instanceof IntType) {
                        function.blocks.get(0).addValueFirst(new Store(alloca,
                                new ConstNumber((Integer) (((Global)constVariable.allocInst).getInitval()))
                                , function.blocks.get(0)));
                    }
                    function.blocks.get(0).addValueFirst(alloca);
                }
            }
        }
        for(Variable variable: global.keySet()) {
            if(global.get(variable).size() <= 1) {
                for(Function function:global.get(variable)) {
                    if(!function.name.equals("main")) {
                        break;
                    }
                    Alloca alloca = IRBuilder.buildAlloca1(
                            "global2local"+ variable.getName(), variable.getType(),
                            function.blocks.get(0), function.getIrRegDispatcher());
                    variable.allocInst.replaceAllUsers(alloca);
                    if(variable.getType() instanceof FloatType) {
                        function.blocks.get(0).addValueFirst(new Store(alloca,
                                new ConstNumber((Double)(((Global)variable.allocInst).getInitval()))
                                , function.blocks.get(0)));
                    } else if(variable.getType() instanceof IntType) {
                        function.blocks.get(0).addValueFirst(new Store(alloca,
                                new ConstNumber((Integer) (((Global)variable.allocInst).getInitval()))
                                , function.blocks.get(0)));
                    }
                    function.blocks.get(0).addValueFirst(alloca);
                }
            }
        }
    }
}
