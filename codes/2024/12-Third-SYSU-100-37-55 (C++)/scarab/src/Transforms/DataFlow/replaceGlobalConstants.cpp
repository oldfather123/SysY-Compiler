#include "replaceGlobalConstants.h"

ValuePtr createConstantFromVariable(Variable* variable) {
    if (variable->type->isInt())
        return Const::createConstant(Type::getInt(), static_cast<Int*>(variable)->intVal);
    else if (variable->type->isFloat())
        return Const::createConstant(Type::getFloat(), static_cast<Float*>(variable)->floatVal);
    return nullptr;
}

void replaceGlobalConstants(Module& module) {
    unordered_set<Variable*> modifiedVariables;

    for (auto& globalVar : module.globalVariables) {
        if (globalVar->isConst) continue;

        for (auto use = globalVar->useHead; use; use = use->next) {
            auto instr = use->user;

            if (auto storeInstr = dynamic_cast<StoreInstruction*>(instr)) {
                if (storeInstr->des == globalVar) {
                    modifiedVariables.insert(globalVar.get());
                    break;
                }
            }
            else if (auto gepInstr = dynamic_cast<GetElementPtrInstruction*>(instr)) {
                if (!gepInstr->from->isReg && gepInstr->from->type->isArr()) {
                    auto array = dynamic_cast<Arr*>(gepInstr->from.get());
                    if (array == dynamic_cast<Arr*>(globalVar.get())) {
                        for (auto pointer = gepInstr->reg->useHead; pointer != nullptr; pointer = pointer->next) {
                            if (pointer->user->type == InsID::Call || pointer->user->type == InsID::Store || pointer->user->type == InsID::GEP) {
                                modifiedVariables.insert(array);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    for (auto& variable : module.globalVariables) {
        if (modifiedVariables.count(variable.get())) continue;

        if (variable->type->isArr()) {
            auto array = static_cast<Arr*>(variable.get());
            array->fill();

            for (auto use = variable->useHead; use != nullptr;) {
                auto user = use->user;
                auto nextUse = use->next;

                if (user->type == InsID::Load) {
                    auto loadInstr = static_cast<LoadInstruction*>(user);
                    auto destination = loadInstr->to;

                    if (auto gepInstr = loadInstr->gep) {
                        auto currentArray = array;
                        for (size_t i = 1; i < gepInstr->index.size(); i++) {
                            auto index = static_cast<Const*>(gepInstr->index[i].get())->intVal;
                            currentArray = static_cast<Arr*>(currentArray->inner[index].get());
                        }

                        auto constantValue = createConstantFromVariable(currentArray);
                        replaceVarByVar(destination, constantValue);
                        user->removeFromParent();
                    }
                }
                
                use->rmUse();
                use = nextUse;
            }
        } 
        else {
            auto constantValue = createConstantFromVariable(variable.get());

            for (auto use = variable->useHead; use != nullptr;) {
                auto user = use->user;
                auto nextUse = use->next;

                if (user->type == InsID::Load) {
                    auto loadInstr = static_cast<LoadInstruction*>(user);
                    replaceVarByVar(loadInstr->to, constantValue);
                    user->removeFromParent();
                }
                else {
                    user->replaceOperand(variable, constantValue);
                }

                use->rmUse();
                use = nextUse;
            }
        }
    }
}