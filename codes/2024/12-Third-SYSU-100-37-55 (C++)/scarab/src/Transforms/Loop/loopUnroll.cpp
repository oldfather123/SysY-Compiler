#include "loopUnroll.h"
#include "LICM.h"

#define MAX_TRIP_COUNT 2000

int loopCopyCounter =0;

void removeAllOperandUses(InstructionPtr instr) {
    auto operandCount  = instr->getOperandCount();
    for(int i = 0; i < operandCount ; i++) {
        if(auto operand = instr->getOperand(i)) {
            deleteUser(operand);
        }
    }
}


VariablePtr cloneVariable(VariablePtr originalVar){
    VariablePtr clonedVar = Variable::copy(originalVar);
    clonedVar->name = clonedVar->name+".copy";
    clonedVar->useHead = nullptr;
    clonedVar->useCount = 0;
    return clonedVar;
}

ValuePtr mapOperandForUnroll(ValuePtr original, unordered_map<ValuePtr, ValuePtr>& valueMap){
    if(original->isConst){
        return original;
    }
    if(valueMap.find(original) == valueMap.end()){
        if (!dynamic_pointer_cast<Variable>(original)) {
            valueMap[original] = make_shared<Reg>(original->type, original->name + ".unroll.copy");
        }
        else {
            valueMap[original] = cloneVariable(dynamic_pointer_cast<Variable>(original));
        }
    }
    return valueMap[original];
}


InstructionPtr cloneInstructionForUnroll(InstructionPtr originalInstr, unordered_map<ValuePtr,ValuePtr>& valueMap,
                                        unordered_map<LabelPtr,LabelPtr>& labelMap, FunctionPtr func){
    switch (originalInstr->type) {
        case Return: {
            auto retInstr = dynamic_cast<ReturnInstruction*>(originalInstr.get());
            auto newInstr = InstructionPtr(new ReturnInstruction(mapOperandForUnroll(retInstr->retValue,valueMap),nullptr));
            valueMap[originalInstr->reg] = newInstr->reg;
            return newInstr;
        }
        case Br: {
            auto brInstr = dynamic_cast<BrInstruction*>(originalInstr.get());
            if(brInstr->exp){
                auto newInstr = std::make_shared<BrInstruction>(
                    mapOperandForUnroll(brInstr->exp, valueMap),
                    labelMap[brInstr->label1],
                    labelMap[brInstr->label2],
                    nullptr
                );
                valueMap[originalInstr->reg] = newInstr->reg;
                return newInstr;
            }
            else{
                auto newInstr = InstructionPtr(new BrInstruction(labelMap[brInstr->label1],nullptr));
                valueMap[originalInstr->reg] = newInstr->reg;
                return newInstr;
            }
        }
        case Alloca: {
            auto allocaInstr = dynamic_cast<AllocaInstruction*>(originalInstr.get());
            auto newInstr = InstructionPtr(new AllocaInstruction(mapOperandForUnroll(allocaInstr->des,valueMap),nullptr));
            valueMap[originalInstr->reg] = newInstr->reg;
            return newInstr;
        }
        case Store: {
            auto storeInstr = dynamic_cast<StoreInstruction*>(originalInstr.get());
            if(storeInstr->gep){
                GetElementPtrInstruction* clonedGEP = dynamic_cast<GetElementPtrInstruction*>((valueMap[storeInstr->gep->reg])->I);
                auto newInstr = make_shared<StoreInstruction>(
                    dynamic_pointer_cast<GetElementPtrInstruction>(clonedGEP->getSharedThis()),
                    mapOperandForUnroll(storeInstr->value, valueMap),
                    nullptr
                );
                valueMap[originalInstr->reg] = newInstr->reg;
                return newInstr;  
            }
            else{
                auto newInstr = InstructionPtr(new StoreInstruction(mapOperandForUnroll(storeInstr->des,valueMap),mapOperandForUnroll(storeInstr->value,valueMap),nullptr));
                valueMap[originalInstr->reg] = newInstr->reg;
                return newInstr;   
            }
        }
        case Load: {
            auto loadInstr = dynamic_cast<LoadInstruction*>(originalInstr.get());
            auto tempReg = func->createRegister(loadInstr->from->type);
            auto newInstr = InstructionPtr(new LoadInstruction(mapOperandForUnroll(loadInstr->from, valueMap), mapOperandForUnroll(tempReg,valueMap),nullptr));
            valueMap[originalInstr->reg] = newInstr->reg;
            return newInstr;
        }
        case Call: {
            auto callInstr = dynamic_cast<CallInstruction*>(originalInstr.get());
            vector<ValuePtr> clonedArgs;
            for (const auto& arg : callInstr->argv) {
                clonedArgs.push_back(mapOperandForUnroll(arg, valueMap));
            }
            auto newInstr =  InstructionPtr(new CallInstruction(callInstr->func, clonedArgs, nullptr));
            valueMap[originalInstr->reg] = newInstr->reg;
            return newInstr;
        }
        case Bitcast: {
            auto bitcastInstr = dynamic_cast<BitCastInstruction*>(originalInstr.get());
            auto newInstr = std::make_shared<BitCastInstruction>(
                mapOperandForUnroll(bitcastInstr->from, valueMap),
                mapOperandForUnroll(bitcastInstr->reg, valueMap),
                nullptr,
                bitcastInstr->targetType
            );
            valueMap[originalInstr->reg] = newInstr->reg;
            return newInstr;
        }
        case Ext: {
            auto extInstr = dynamic_cast<ExtInstruction*>(originalInstr.get());
            auto newInstr = std::make_shared<ExtInstruction>(
                mapOperandForUnroll(extInstr->from, valueMap),
                extInstr->to,
                extInstr->isSigned,
                nullptr
            );
            valueMap[originalInstr->reg] = newInstr->reg;
            return newInstr;
        }
        case Sitofp: {
            auto sitofpInstr = dynamic_cast<SignToFloatInstruction*>(originalInstr.get());
            auto newInstr =  InstructionPtr(new SignToFloatInstruction(mapOperandForUnroll(sitofpInstr->from, valueMap), nullptr));
            valueMap[originalInstr->reg] = newInstr->reg;
            return newInstr;
        }
        case Fptosi: {
            auto fptosiInstr = dynamic_cast<FloatToSignInstruction*>(originalInstr.get());
            auto newInstr =  InstructionPtr(new FloatToSignInstruction(mapOperandForUnroll(fptosiInstr->from, valueMap), nullptr));
            valueMap[originalInstr->reg] = newInstr->reg;
            return newInstr;
        }
        case GEP: {
            auto gepInstr = dynamic_cast<GetElementPtrInstruction*>(originalInstr.get());
            vector<ValuePtr> newIndex;
            for(auto oldIdx : gepInstr->index){
                newIndex.push_back(mapOperandForUnroll(oldIdx,valueMap));
            }
            auto newInstr =  InstructionPtr(new GetElementPtrInstruction(mapOperandForUnroll(gepInstr->from,valueMap),newIndex,nullptr));
            if(gepInstr->from->type->isPtr()){
                newInstr->reg->type = dynamic_cast<PtrType*>(gepInstr->from->type.get())->inner;
            }
            valueMap[originalInstr->reg] = newInstr->reg;
            return newInstr;
        }
        case Binary: {
            auto binaryInstr = dynamic_cast<BinaryInstruction*>(originalInstr.get());
            auto newInstr = std::make_shared<BinaryInstruction>(
                mapOperandForUnroll(binaryInstr->a, valueMap),
                mapOperandForUnroll(binaryInstr->b, valueMap),
                binaryInstr->op,
                BasicBlockPtr(nullptr)
            );
            valueMap[originalInstr->reg] = newInstr->reg;
            return newInstr;
        }
        case Fneg: {
            auto fnegInstr = dynamic_cast<FnegInstruction*>(originalInstr.get());
            auto newInstr = InstructionPtr(new FnegInstruction(mapOperandForUnroll(fnegInstr->a, valueMap), nullptr));
            valueMap[originalInstr->reg] = newInstr->reg;
            return newInstr;
        }
        case Icmp: {
            auto icmpInstr = dynamic_cast<IcmpInstruction*>(originalInstr.get());
            auto newInstr = std::make_shared<IcmpInstruction>(
                nullptr,
                mapOperandForUnroll(icmpInstr->a, valueMap),
                mapOperandForUnroll(icmpInstr->b, valueMap),
                icmpInstr->op
            );
            valueMap[originalInstr->reg] = newInstr->reg;
            return newInstr;
        }
        case Fcmp: {
            auto fcmpInstr = dynamic_cast<FcmpInstruction*>(originalInstr.get());
            auto newInstr = std::make_shared<FcmpInstruction>(
                nullptr,
                mapOperandForUnroll(fcmpInstr->a, valueMap),
                mapOperandForUnroll(fcmpInstr->b, valueMap),
                fcmpInstr->op
            );
            valueMap[originalInstr->reg] = newInstr->reg;
            return newInstr;
        }
        case Phi: {
            auto phiInstr = dynamic_cast<PhiInstruction*>(originalInstr.get());
            auto newInstr = InstructionPtr(new PhiInstruction(nullptr, mapOperandForUnroll(phiInstr->val, valueMap)));
            valueMap[originalInstr->reg] = newInstr->reg;
            return newInstr;
        }
        case Select:{
            auto selectInstr = dynamic_cast<SelectInstruction*>(originalInstr.get());
            if(selectInstr == nullptr)
                assert(false && "wrong ins type");

            auto newInstr =  InstructionPtr(new SelectInstruction(
                nullptr, 
                mapOperandForUnroll(selectInstr->exp, valueMap), 
                mapOperandForUnroll(selectInstr->val1, valueMap), 
                mapOperandForUnroll(selectInstr->val2, valueMap)
                ));
            valueMap[originalInstr->reg] = newInstr->reg;
            return newInstr;
        }
        default:
            assert(false && "Unknown instruction type");
            return nullptr;
    }
}


LoopPtr createLoopCopy(LoopPtr loop, std::unordered_map<ValuePtr, ValuePtr>& LoopValueCpy, unordered_map<ValuePtr, ValuePtr>& currentIncomingValues, BasicBlockPtr exitblock, FunctionPtr func) {
    string loopCopyIndex = to_string(loopCopyCounter++);
    LabelPtr headerLabel = LabelPtr(new Label(loop->header->label->name+".copy"+loopCopyIndex));
    auto headerCopy = BasicBlockPtr(new BasicBlock(headerLabel));

    LabelPtr latchLabel = LabelPtr(new Label(loop->latchBlock->label->name+".copy"+loopCopyIndex));
    auto latchCopy = (loop->header == loop->latchBlock) ? headerCopy : BasicBlockPtr(new BasicBlock(latchLabel));
    auto loopCopy = LoopPtr(new Loop(headerCopy,1));
    loopCopy->latchBlock = latchCopy;

    unordered_map<PhiInstruction*, PhiInstruction*> phiMappingQueue;
    unordered_map<ValuePtr,ValuePtr> valueMap;
    unordered_map<BasicBlockPtr, BasicBlockPtr> blockMap;
    unordered_map<LabelPtr, LabelPtr> labelMap;
    unordered_map<InstructionPtr, InstructionPtr> instructionMap;
    unordered_map<ValuePtr, ValuePtr> copiedValuesMap;

    for(auto basicBlock : loop->basicBlockSet) {
        if(basicBlock == loop->header) {
            blockMap[basicBlock] = headerCopy;
            labelMap[basicBlock->label] = headerCopy->label;
        }
        else if(basicBlock == loop->latchBlock) {
            blockMap[basicBlock] = latchCopy;
            labelMap[basicBlock->label] = latchCopy->label;
            loopCopy->addBasicBlock(latchCopy);
        }
        else {
            LabelPtr headerLabel = LabelPtr(new Label(basicBlock->label->name+".unrollcpy"+loopCopyIndex));
            auto loopBodyCpy = BasicBlockPtr(new BasicBlock(headerLabel));
            blockMap[basicBlock] = loopBodyCpy;
            labelMap[basicBlock->label] = headerLabel;
            loopCopy->addBasicBlock(loopBodyCpy);
        }
    }
    labelMap[exitblock->label] = exitblock->label; 

    auto copyInstructionsToBlock = [&](BasicBlockPtr srcBB, BasicBlockPtr dstBB) {
        for (auto instr : srcBB->instructions) {
            if (instr->type == Phi) continue;
            auto instrCopy = cloneInstructionForUnroll(instr, currentIncomingValues, labelMap, func);
            dstBB->appendInstruction(instrCopy);
            instrCopy->basicblock = dstBB;
            
            if (instrCopy->type == Binary && instr->type == Binary) {
                auto binaryInstr = dynamic_cast<BinaryInstruction*>(instr.get());
                auto binaryCopy = dynamic_cast<BinaryInstruction*>(instrCopy.get());
                copiedValuesMap[binaryInstr->reg] = binaryCopy->reg;
            }
            if (instrCopy->type == Load && instr->type == Load) {
                auto loadInstr = dynamic_cast<LoadInstruction*>(instr.get());
                auto loadCopy = dynamic_cast<LoadInstruction*>(instrCopy.get());
                copiedValuesMap[loadInstr->reg] = loadCopy->reg;
            }
            if (instr->type == Phi) {
                auto phiInstr = dynamic_cast<PhiInstruction*>(instr.get());
                if (instrCopy->type == Phi) {
                    auto phiCopy = dynamic_cast<PhiInstruction*>(instrCopy.get());
                    phiMappingQueue[phiInstr] = phiCopy;
                }
            }
        }
    };

    copyInstructionsToBlock(loop->header, headerCopy);
    copyInstructionsToBlock(loop->latchBlock, latchCopy);

    for (const auto& [basicBlock, bbCpy] : blockMap) {
        if (basicBlock != loop->header) {
            for (auto pred : basicBlock->predBasicBlocks) {
                bbCpy->predBasicBlocks.insert(blockMap[pred]);
            }
        }
        if (basicBlock != loop->latchBlock) {
            for (auto succ : basicBlock->succBasicBlocks) {
                bbCpy->succBasicBlocks.insert(blockMap[succ]);
            }
        }
    }

    for (const auto& [phiInstr, phiCopy] : phiMappingQueue) {
        for (const auto& [val, pred] : phiInstr->from) {
            phiCopy->addFrom(val, blockMap[pred]);
        }
    }

    unordered_map<PhiInstruction*, ValuePtr> phiUpdateLater;
    for(auto instr : loop->header->instructions) {
        if(instr->type == Phi) {
            auto phiInstr = dynamic_cast<PhiInstruction*>(instr.get());
            for (const auto& [val, pred] : phiInstr->from) {
                if (pred == loop->latchBlock) {
                    phiUpdateLater[phiInstr] = val;
                }
            }
        }
        else {
            break;
        }
    }

    for (const auto& [phiInstr, val] : phiUpdateLater) {
        assert(copiedValuesMap.find(val) != copiedValuesMap.end() && "Unroll loop fault\n");
        currentIncomingValues[phiInstr->reg] = copiedValuesMap[val];
    }
 
    return loopCopy;
}

void updateCurrentValuesMapping(InstructionPtr instr, std::unordered_map<ValuePtr, ValuePtr>& currentIncomingValues) {
    switch (instr->type) {
        case Binary: {
            auto binaryInstr = dynamic_cast<BinaryInstruction*>(instr.get());
            if (!currentIncomingValues.count(binaryInstr->reg)) {
                currentIncomingValues[binaryInstr->reg] = binaryInstr->reg;
            }
            if (binaryInstr->a && (!currentIncomingValues.count(binaryInstr->a)) && !binaryInstr->a->isConst) {
                currentIncomingValues[binaryInstr->a] = binaryInstr->a;
            }
            if (binaryInstr->b && (!currentIncomingValues.count(binaryInstr->b)) && !binaryInstr->b->isConst) {
                currentIncomingValues[binaryInstr->b] = binaryInstr->b;
            }
            break;
        }
        case GEP: {
            auto gepInstr = dynamic_cast<GetElementPtrInstruction*>(instr.get());
            if (!currentIncomingValues.count(gepInstr->reg)) {
                currentIncomingValues[gepInstr->reg] = gepInstr->reg;
            }
            if (gepInstr->from && (!currentIncomingValues.count(gepInstr->from))) {
                currentIncomingValues[gepInstr->from] = gepInstr->from;
            }
            for (auto idx : gepInstr->index) {
                if (!currentIncomingValues.count(idx)) {
                    currentIncomingValues[idx] = idx;
                }
            }
            break;
        }
        case Store: {
            auto storeInstr = dynamic_cast<StoreInstruction*>(instr.get());
            if (!currentIncomingValues.count(storeInstr->des)) {
                currentIncomingValues[storeInstr->des] = storeInstr->des;
            }
            if (!currentIncomingValues.count(storeInstr->value)) {
                currentIncomingValues[storeInstr->value] = storeInstr->value;
            }
            break;
        }
        case Load: {
            auto loadInstr = dynamic_cast<LoadInstruction*>(instr.get());
            if (!currentIncomingValues.count(loadInstr->from)) {
                currentIncomingValues[loadInstr->from] = loadInstr->from;
            }
            if(!currentIncomingValues.count(loadInstr->reg)) {
                currentIncomingValues[loadInstr->reg] = loadInstr->reg;
            }
            break;
        }
        case Ext: {
            auto extInstr = dynamic_cast<ExtInstruction*>(instr.get());
            if (!currentIncomingValues.count(extInstr->from)) {
                currentIncomingValues[extInstr->from] = extInstr->from;
            }
            if (!currentIncomingValues.count(extInstr->reg)) {
                currentIncomingValues[extInstr->reg] = extInstr->reg;
            }
            break;
        }
        case Sitofp: {
            auto sitofpInstr = dynamic_cast<SignToFloatInstruction*>(instr.get());
            if (!currentIncomingValues.count(sitofpInstr->from)) {
                currentIncomingValues[sitofpInstr->from] = sitofpInstr->from;
            }
            if (!currentIncomingValues.count(sitofpInstr->reg)) {
                currentIncomingValues[sitofpInstr->reg] = sitofpInstr->reg;
            }
            break;
        }
        case Fptosi: {
            auto fptosiInstr = dynamic_cast<FloatToSignInstruction*>(instr.get());
            if (!currentIncomingValues.count(fptosiInstr->from)) {
                currentIncomingValues[fptosiInstr->from] = fptosiInstr->from;
            }
            if (!currentIncomingValues.count(fptosiInstr->reg)) {
                currentIncomingValues[fptosiInstr->reg] = fptosiInstr->reg;
            }
            break;
        }
        case Fneg: {
            auto fnegInstr = dynamic_cast<FnegInstruction*>(instr.get());
            if (!currentIncomingValues.count(fnegInstr->a)) {
                currentIncomingValues[fnegInstr->a] = fnegInstr->a;
            }
            if (!currentIncomingValues.count(fnegInstr->reg)) {
                currentIncomingValues[fnegInstr->reg] = fnegInstr->reg;
            }
            break;
        }
        case Bitcast: {
            auto bitcastInstr = dynamic_cast<BitCastInstruction*>(instr.get());
            if (!currentIncomingValues.count(bitcastInstr->from)) {
                currentIncomingValues[bitcastInstr->from] = bitcastInstr->from;
            }
            if (!currentIncomingValues.count(bitcastInstr->reg)) {
                currentIncomingValues[bitcastInstr->reg] = bitcastInstr->reg;
            }
            break;
        }
        case Call: {
            auto callInstr = dynamic_cast<CallInstruction*>(instr.get());
            if(!currentIncomingValues.count(callInstr->reg)){
                currentIncomingValues[callInstr->reg] = callInstr->reg;
            }
        }
        default:
            break;
    }
}

void updatePhiOperands(ValuePtr& operand, BasicBlockPtr latch, std::unordered_map<ValuePtr, ValuePtr>& currentIncomingValues) {
    if (operand && operand->I && operand->I->type == Phi) {
        auto phiInstr = dynamic_cast<PhiInstruction*>(operand->I);
        bool isLatchPredecessor = false;
        for (auto pred : phiInstr->from) {
            if (pred.second == latch) {
                isLatchPredecessor = true;
                break;
            }
        }
        if (isLatchPredecessor) {
            operand = currentIncomingValues[phiInstr->reg];
        }
    }
}

void updateExitBlockOperands(BasicBlockPtr exitBlock, BasicBlockPtr latch, std::unordered_map<ValuePtr, ValuePtr>& currentIncomingValues) {
    for (auto instr : exitBlock->instructions) {
        if (instr->type == Binary) {
            auto binaryInstr = dynamic_cast<BinaryInstruction*>(instr.get());
            updatePhiOperands(binaryInstr->a, latch, currentIncomingValues);
            updatePhiOperands(binaryInstr->b, latch, currentIncomingValues);
        }
    }
}

void updateHeaderPhiInstructions(std::vector<InstructionPtr>& headerPhiInstructions, LoopPtr loop, std::unordered_map<ValuePtr, ValuePtr>& currentIncomingValues) {
    for (auto instr : headerPhiInstructions) {
        auto phiInstr = dynamic_cast<PhiInstruction*>(instr.get());
        for (auto pred : phiInstr->from) {
            if (pred.second == loop->preHeader) {
                auto user = phiInstr->reg->useHead;
                while (user) {
                    auto nextUser = user->next;
                    auto userInstr = user->user;
                    if (!loop->contains(userInstr->basicblock)) {
                        userInstr->replaceOperand(phiInstr->reg, currentIncomingValues[phiInstr->reg]);
                        user->rmUse();
                        newUse(currentIncomingValues[phiInstr->reg].get(), userInstr);
                    }
                    user = nextUser;
                }
                replaceVarByVar(phiInstr->reg, pred.first);
                removeAllOperandUses(instr);
                phiInstr->basicblock->removeInstruction(instr);
                break;
            }
        }
    }
}

void updateLatchHeaderEdges(std::vector<BasicBlockPtr>& latchHeaderEdges, LoopPtr loop, FunctionPtr func) {
    int index = 0;
    auto blockIter = std::find(func->basicBlocks.begin(), func->basicBlocks.end(), latchHeaderEdges.front());
    vector<BasicBlockPtr> insertList;

    for (auto latchCopy : latchHeaderEdges) {
        ++index;
        if (index == latchHeaderEdges.size()) {
            break;
        }
        auto nextLatch = latchHeaderEdges[index];
        latchCopy->succBasicBlocks.insert(nextLatch);
        nextLatch->predBasicBlocks.clear();
        nextLatch->predBasicBlocks.insert(latchCopy);

        auto branchInstr = std::make_shared<BrInstruction>(nextLatch->label, latchCopy);
        latchCopy->appendInstruction(branchInstr);

        if (!loop->contains(latchCopy)) {
            loop->addBasicBlock(latchCopy);
            insertList.push_back(latchCopy);
        }
    }

    func->basicBlocks.insert(blockIter, insertList.begin(), insertList.end());
}

void updateBasicBlockPredecessors(BasicBlockPtr header, BasicBlockPtr latch, BasicBlockPtr exit, FunctionPtr func) {
    auto lastInstr = --header->instructions.end();
    header->instructions.erase(lastInstr);
    header->terminator = nullptr;
    auto branchInstr = std::make_shared<BrInstruction>(latch->label, header);
    header->appendInstruction(branchInstr);

    auto headerPred = std::find(header->predBasicBlocks.begin(), header->predBasicBlocks.end(), latch);
    if (headerPred != header->predBasicBlocks.end()) {
        header->predBasicBlocks.erase(headerPred);
    }

    auto exitPred = std::find(exit->predBasicBlocks.begin(), exit->predBasicBlocks.end(), header);
    if (exitPred != exit->predBasicBlocks.end()) {
        exit->predBasicBlocks.erase(exitPred);
    }

    unordered_set<BasicBlockPtr> visited(func->basicBlocks.begin(), func->basicBlocks.end());
    for (auto basicBlock : func->basicBlocks) {
        for (auto pred : basicBlock->predBasicBlocks) {
            if (!visited.count(pred)) {
                basicBlock->predBasicBlocks.erase(pred);
            }
        }
    }
}

bool isValidLoop(LoopPtr loop){
    auto iterationCount = loop->iterationCount;
    if(iterationCount <= 0 || iterationCount >= MAX_TRIP_COUNT) return false;
    
    if(loop->exitBlocks.size() != 1 || loop->exitingBlocks.size() != 1 || loop->subLoops.size()) return false;
    
    for (auto basicBlock : loop->basicBlockSet) {
        for (auto instr : basicBlock->instructions) {
            if (instr->type == Call) return false;
            if (instr->type == Icmp && basicBlock != loop->header) return false;
        }
    }
    return true;
}

void processLoop(LoopPtr loop, FunctionPtr func) {
    loopCopyCounter = 0;

    if(!isValidLoop(loop)) {
        return;
    }
    
    auto header = loop->header;
    auto latch = loop->latchBlock;
    auto exit = *loop->exitBlocks.begin();

    unordered_set<ValuePtr> phiSet;
    bool hasPhiValues =  false;
    for (auto instr : header->instructions) {
        if (instr->type == Phi) {
            auto phiInstr = dynamic_cast<PhiInstruction*>(instr.get());
            for (auto pred : phiInstr->from) {
                if (pred.second == latch) {
                    phiSet.insert(pred.first);
                }
            }
        }
        else break;
    }

    for (auto instr : header->instructions) {
        if (instr->type == Phi) {
            if (phiSet.count(dynamic_cast<PhiInstruction*>(instr.get())->reg)) {
                hasPhiValues = true;
                break;
            }
        } 
        else break;
    }

    if(hasPhiValues) {
        return;
    }

    unordered_map<ValuePtr, ValuePtr> currentIncomingValues;
    vector<InstructionPtr> headerPhiInstructions;

    for (auto instr : header->instructions) {
        if (instr->type == Phi) {
            auto phiInstr = dynamic_cast<PhiInstruction*>(instr.get());
            headerPhiInstructions.push_back(instr);
            for (auto pred : phiInstr->from) {
                if (pred.second == latch) {
                    currentIncomingValues[phiInstr->reg] = pred.first;
                }
            }
        }
        else break;
    }
    
    for (auto instr : latch->instructions) {
        updateCurrentValuesMapping(instr, currentIncomingValues);
    }

    auto lastInstr = --latch->instructions.end();
    auto latchCondBr = latch->instructions[latch->instructions.size()-1];
    latch->instructions.pop_back();
    latch->terminator = nullptr;
    removeAllOperandUses(latchCondBr);
    latch->succBasicBlocks.clear();

    vector<BasicBlockPtr> latchHeaderEdges;
    latchHeaderEdges.push_back(latch);
    unordered_map<ValuePtr, ValuePtr> copyMap;
    int tripCount = loop->iterationCount;

    for (int i = 1; i < tripCount; ++i) {
        auto loopCopy = createLoopCopy(loop, copyMap, currentIncomingValues, exit, func);
        latchHeaderEdges.push_back(loopCopy->latchBlock);
    }
    latchHeaderEdges.push_back(exit);

    updateExitBlockOperands(exit, latch, currentIncomingValues);

    updateHeaderPhiInstructions(headerPhiInstructions, loop, currentIncomingValues);

    updateLatchHeaderEdges(latchHeaderEdges, loop, func);

    updateBasicBlockPredecessors(header, latch, exit, func);
    cerr << "successfully unroll " << func->name << '\n';
}

void loopUnroll(FunctionPtr func) {
    queue<LoopPtr> loopQueue;

    for (auto loop : func->loopList) {
        if (loop->parentLoop == nullptr) {
            loopQueue.push(loop);
        }
    }

    while (!loopQueue.empty()) {
        auto loop = loopQueue.front();
        loopQueue.pop();

        processLoop(loop, func);

        for (auto subLoop : loop->subLoops) {
            loopQueue.push(subLoop);
        }
    }
}