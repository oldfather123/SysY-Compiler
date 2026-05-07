package cn.edu.bit.newnewcc.ir;

import cn.edu.bit.newnewcc.ir.exception.IllegalArgumentException;
import cn.edu.bit.newnewcc.ir.value.ExternalFunction;
import cn.edu.bit.newnewcc.ir.value.Function;
import cn.edu.bit.newnewcc.ir.value.GlobalVariable;

import java.util.Collection;
import java.util.Collections;
import java.util.LinkedHashSet;
import java.util.Set;

/**
 * 模块
 * <p>
 * 每个源文件构成一个模块
 */
public class Module {
    private final Set<Function> functions = new LinkedHashSet<>();
    private final Set<ExternalFunction> externalFunctions = new LinkedHashSet<>();
    private final Set<GlobalVariable> globalVariables = new LinkedHashSet<>();

    public void addFunction(Function function) {
        functions.add(function);
    }

    public void removeFunction(Function function) {
        if (functions.contains(function)) {
            functions.remove(function);
        } else {
            throw new IllegalArgumentException("Function not found in this module.");
        }
    }

    public Collection<Function> getFunctions() {
        return Collections.unmodifiableSet(functions);
    }

    public void addExternalFunction(ExternalFunction externalFunction) {
        externalFunctions.add(externalFunction);
    }

    public Collection<ExternalFunction> getExternalFunctions() {
        return Collections.unmodifiableSet(externalFunctions);
    }

    public void addGlobalVariable(GlobalVariable globalVariable) {
        globalVariables.add(globalVariable);
    }

    public void removeGlobalVariable(GlobalVariable globalVariable) {
        if (globalVariables.contains(globalVariable)) {
            globalVariables.remove(globalVariable);
        } else {
            throw new IllegalArgumentException("Global variable not found in this module.");
        }
    }

    public Collection<GlobalVariable> getGlobalVariables() {
        return Collections.unmodifiableSet(globalVariables);
    }

    public void emitIr(StringBuilder builder) {
        for (ExternalFunction externalFunction : this.getExternalFunctions()) {
            externalFunction.emitIr(builder);
        }
        builder.append('\n');
        for (GlobalVariable globalVariable : this.getGlobalVariables()) {
            globalVariable.emitIr(builder);
        }
        builder.append('\n');
        for (Function function : this.getFunctions()) {
            function.emitIr(builder);
        }
        builder.append('\n');
    }

}
