#include "optimizeLocalToGlobal.h"

bool hasStoreBeforeLoad(ValuePtr array, FunctionPtr func) {
    bool loadDetected = false;
    unordered_set<ValuePtr> relevantGEPs = {array};

    for (auto& basicBlock : func->basicBlocks) {
        for (auto& instr : basicBlock->instructions) {
            switch (instr->type) {
                case Load: {
                    auto loadInstr = dynamic_cast<LoadInstruction*>(instr.get());
                    if (relevantGEPs.count(loadInstr->from)) {
                        loadDetected = true;
                    }
                    break;
                }
                case Store: {
                    auto storeInstr = dynamic_cast<StoreInstruction*>(instr.get());
                    if (relevantGEPs.count(storeInstr->des) && loadDetected) {
                        return false;
                    }
                    break;
                }
                case GEP: {
                    auto gepInstr = dynamic_cast<GetElementPtrInstruction*>(instr.get());
                    if (relevantGEPs.count(gepInstr->from)) {
                        relevantGEPs.insert(gepInstr->reg);
                    }
                    break;
                }
                case Bitcast: {
                    auto castInstr = dynamic_cast<BitCastInstruction*>(instr.get());
                    if (relevantGEPs.count(castInstr->from)) {
                        relevantGEPs.insert(castInstr->reg);
                    }
                    break;
                }
                case Call: {
                    auto callInstr = dynamic_cast<CallInstruction*>(instr.get());
                    for (auto& arg : callInstr->argv) {
                        if (relevantGEPs.count(arg)) {
                            return false;
                        }
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }
    return true;
}

vector<int> extractShape(TypePtr type) {
    if (type->isArr()) {
        auto arrayType = dynamic_cast<ArrType*>(type.get());
        vector<int> dimensions{arrayType->size};
        auto innerDimensions = extractShape(arrayType->inner);
        dimensions.insert(dimensions.end(), innerDimensions.begin(), innerDimensions.end()); 
        return dimensions;
    }
    else if (type->isInt() || type->isFloat()) {
        return {};
    }
    throw std::runtime_error("Unsupported type in extractShape: " + type->toString());
}

void traverseUses(InstructionPtr instr, function<bool(InstructionPtr)> shouldSkip, function<void(InstructionPtr)> action) {
    if (shouldSkip(instr)) return;
    action(instr);
    ValuePtr registerValue = nullptr;
    if (instr->type == Alloca) {
        registerValue = dynamic_cast<AllocaInstruction*>(instr.get())->des;
    }
    else if (instr->type == GEP || instr->type == Bitcast) {
        registerValue = instr->reg;
    }

    if (!registerValue) return;

    for (auto use = registerValue->useHead; use; use = use->next) {
        if (use->user) {
            traverseUses(use->user->getSharedThis(), shouldSkip, action);
        }
    }
}

bool optimizeLocalToGlobal(Module &module) {
    bool changesMade = false;
    for (auto& func: module.globalFunctions) {
        if (func->isLibraryFunction) continue;
        unordered_set<InstructionPtr> instructionsToRemove;
        vector<ValuePtr> localArrays;
        for (auto& instr: func->getEntryBlock()->instructions) {
            if (instr->type != Alloca) continue;
            auto allocInstr = dynamic_cast<AllocaInstruction*>(instr.get());
            if (allocInstr->des->type->isArr()) {
                localArrays.emplace_back(allocInstr->des);
            }
        }

        for (auto& array: localArrays) {
            if (!hasStoreBeforeLoad(array, func)) continue;
            auto dimensions = extractShape(array->type);
            if (dimensions.size() != 1) continue;

            unordered_map<ValuePtr, int> indexMap;
            vector<vector<StoreInstruction*>> storeInstructions(dimensions[0]);
            unordered_map<ValuePtr, bool> isGEPMutable;
            unordered_set<InstructionPtr> visitedInstructions;
            unordered_set<StoreInstruction*> storeInstrSet;
            unordered_set<LoadInstruction*> loadInstrSet;
            unordered_set<GetElementPtrInstruction*> gepInstrSet;
            bool isValidArray = true;
            isGEPMutable[array] = false;

            traverseUses(array->I->getSharedThis(), [&](InstructionPtr instr) -> bool {
                if (visitedInstructions.count(instr))
                    return true;
                visitedInstructions.insert(instr);
                return false;
            }, [&](InstructionPtr instr) -> void {
                if (!isValidArray || !instr) return;
                switch (instr->type) {
                    case Store: {
                        auto storeInstr = dynamic_cast<StoreInstruction*>(instr.get());
                        auto gepInstr = storeInstr->des;
                        if (gepInstr->I) {
                            assert(gepInstr->I->type == GEP);
                        }
                        if (!storeInstr->value->isConst || isGEPMutable[gepInstr]) {
                            isValidArray = false;
                        }
                        else {
                            assert(indexMap.count(gepInstr));
                            int index = indexMap[gepInstr];
                            auto constant = dynamic_cast<Const*>(storeInstr->value.get());
                            assert(constant != nullptr);
                            storeInstrSet.insert(storeInstr);
                            storeInstructions[index].push_back(storeInstr);
                        }
                        break;
                    }
                    case Load: {
                        auto loadInstr = dynamic_cast<LoadInstruction*>(instr.get());
                        auto gepInstr = loadInstr->from;
                        assert(gepInstr->I->type == GEP);
                        if (!indexMap.count(gepInstr) || isGEPMutable[gepInstr]) {
                            isValidArray = false;
                        }
                        else {
                            loadInstrSet.insert(loadInstr);
                        }
                        break;
                    }
                    case GEP: {
                        auto gepInstr = dynamic_cast<GetElementPtrInstruction*>(instr.get());
                        auto index = (gepInstr->index.size() == 1) ? gepInstr->index[0] : gepInstr->index[1];
                        if (auto constant = dynamic_cast<Const*>(index.get())) {
                            indexMap[gepInstr->reg] = constant->intVal;
                            isGEPMutable[gepInstr->reg] = isGEPMutable[gepInstr->from];
                        }
                        else {
                            isGEPMutable[gepInstr->reg] = true;
                        }
                        gepInstrSet.insert(gepInstr);
                        break;
                    }
                    case Call: {
                        isValidArray = false;
                        break;
                    }
                    default:
                        break;
                }
            });

            vector<int> constantValues(dimensions[0], 0);
            if (isValidArray) {
                bool isStoreOrderValid = true;
                for (int i = 0; i < dimensions[0]; i++) {
                    if (storeInstructions[i].empty()) continue;
                    auto finalStore = storeInstructions[i][0];
                    for (int j = 1; j < storeInstructions[i].size() && isStoreOrderValid; j++) {
                        auto currentStore = storeInstructions[i][j];
                        auto currentBlock = currentStore->basicblock;
                        auto finalBlock = finalStore->basicblock;
                        if (currentBlock != finalBlock) {
                            if (currentBlock->allDominators.count(finalBlock)) {
                                finalStore = currentStore;
                            }
                            if (finalBlock->allDominators.count(currentBlock)) {
                            }
                            else {
                                isStoreOrderValid = false;
                                break;
                            }
                        }
                        else {
                            bool orderFound = false;
                            for (auto& instr: currentBlock->instructions) {
                                if (instr == currentStore->getSharedThis()){
                                    finalStore = currentStore;
                                    orderFound = true;
                                    break;
                                }
                                if (instr == finalStore->getSharedThis()){
                                    orderFound = true;
                                    break;
                                }
                            }
                            assert(orderFound);
                        }
                    }
                    auto constantValue = dynamic_cast<Const*>(finalStore->value.get());
                    assert(constantValue && constantValue->type->isInt());
                    constantValues[i] = constantValue->intVal;
                }

                for (auto& loadInstr: loadInstrSet) {
                    assert(indexMap.count(loadInstr->from));
                    int index = indexMap[loadInstr->from];
                    auto constantValue = Const::createConstant(Type::getInt(), constantValues[index], "local_arrary_propagation" + to_string(index));
                    replaceVarByVar(loadInstr->reg, constantValue);
                    deleteUser(loadInstr->reg);
                    instructionsToRemove.insert(loadInstr->getSharedThis());
                }
                for (auto storeInstr: storeInstrSet) {
                    instructionsToRemove.insert(storeInstr->getSharedThis());
                }
                for (auto gepInstr: gepInstrSet) {
                    instructionsToRemove.insert(gepInstr->getSharedThis());
                }
                instructionsToRemove.insert(array->I->getSharedThis());
            }
        }
        if (!instructionsToRemove.empty()) {
            changesMade = true;
            for (auto &basicBlock : func->basicBlocks) {
                auto &instructions = basicBlock->instructions;
                instructions.erase(std::remove_if(instructions.begin(), instructions.end(), [&](InstructionPtr instr) {
                    return instructionsToRemove.count(instr) > 0;
                }), instructions.end());
            }
        }
    }
    return changesMade;
}