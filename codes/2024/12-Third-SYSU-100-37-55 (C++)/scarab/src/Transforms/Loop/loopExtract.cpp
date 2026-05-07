#include "loopExtract.h"

// 有可能会出问题，优先考虑loopUnroll里面的updateCurrent那个函数，有可能是它出问题，需要把reg也放valueMap才行
// 也有可能是部分循环展开时的比较策略不太对（cmp和加减乘除，这里需要着重看一下）

static int totalSize = 0;
static int parallelNum = 0;


bool scanParams(BasicBlockPtr bb, vector<pair<ValuePtr, int>> &newParams, unordered_map<ValuePtr, ValuePtr> &valueMap, LoopPtr loop){
    for(auto inst: bb->instructions){
        if(inst == bb->terminator) continue;

        if(inst->reg) valueMap[inst->reg] = nullptr;
        if(inst->type == Store){
            auto storeInst = dynamic_cast<StoreInstruction*>(inst.get());
            if(!valueMap.count(storeInst->des)){
                auto desVar = dynamic_cast<Variable*>(storeInst->des.get());
                if(desVar && desVar->isGlobal){
                    // 那就完全没必要当param了，因为本来就可以在外面拿到
                }
                else{
                    if(storeInst->des->I && storeInst->des->I->type == GEP)return false;
                    newParams.push_back({storeInst->des, totalSize});
                    totalSize+=8;
                    valueMap[storeInst->des] = nullptr;
                }
                
            }
            if(!valueMap.count(storeInst->value) && !storeInst->value->isConst){
                newParams.push_back({storeInst->value, totalSize});
                totalSize+=8;
                valueMap[storeInst->value] = nullptr;
            }
        }
        else if(inst->type == Load){
            auto loadInst = dynamic_cast<LoadInstruction*>(inst.get());
            if(!valueMap.count(loadInst->from)){
                auto fromVar = dynamic_cast<Variable*>(loadInst->from.get());
                if(fromVar && fromVar->isGlobal){
                    // 那就完全没必要当param了，因为本来就可以在外面拿到
                }
                else{
                    if(loadInst->from->I && loadInst->from->I->type == GEP)return false;
                    newParams.push_back({loadInst->from, totalSize});
                    totalSize+=8;
                    valueMap[loadInst->from] = nullptr;
                }
                
            }
        }
        else if(inst->type == Binary){
            auto binaryInst = dynamic_cast<BinaryInstruction*>(inst.get());
            if(!valueMap.count(binaryInst->a) && !binaryInst->a->isConst){
                newParams.push_back({binaryInst->a, totalSize});
                totalSize+=8;
                valueMap[binaryInst->a] = nullptr;
            }
            if(!valueMap.count(binaryInst->b) && !binaryInst->b->isConst){
                newParams.push_back({binaryInst->b, totalSize});
                totalSize+=8;
                valueMap[binaryInst->b] = nullptr;
            }
        }
        else if(inst->type == Phi){
            auto phiNode = dynamic_cast<PhiInstruction*>(inst.get());
            if(phiNode->from.size() > 2) return false;
            for(auto it:phiNode->from){
                if(it.second == loop->latchBlock) continue;
                if(!valueMap.count(it.first) && !it.first->isConst){
                    newParams.push_back({it.first, totalSize});
                    totalSize+=8;
                    valueMap[it.first] = nullptr;
                }
            }
        }
        else if(inst->type == Ext){
            auto extInst = dynamic_cast<ExtInstruction*>(inst.get());
            if(!valueMap.count(extInst->from) && !extInst->from->isConst){
                newParams.push_back({extInst->from, totalSize});
                totalSize+=8;
                valueMap[extInst->from] = nullptr;
            }
        }
        else if(inst->type == Sitofp){
            auto sitofpInst = dynamic_cast<SignToFloatInstruction*>(inst.get());
            if(!valueMap.count(sitofpInst->from) && !sitofpInst->from->isConst){
                newParams.push_back({sitofpInst->from, totalSize});
                totalSize+=8;
                valueMap[sitofpInst->from] = nullptr;
            }
        }
        else if(inst->type == Fptosi){
            auto fptosiInst = dynamic_cast<FloatToSignInstruction*>(inst.get());
            if(!valueMap.count(fptosiInst->from) && !fptosiInst->from->isConst){
                newParams.push_back({fptosiInst->from, totalSize});
                totalSize+=8;
                valueMap[fptosiInst->from] = nullptr;
            }
        }
        else if(inst->type == GEP){
            auto gepInst = dynamic_cast<GetElementPtrInstruction*>(inst.get());
            if(!valueMap.count(gepInst->from) && !gepInst->from->isConst){
                auto fromVar = dynamic_cast<Variable*>(gepInst->from.get());
                if(fromVar && fromVar->isGlobal){
                    // 那就完全没必要当param了，因为本来就可以在外面拿到
                }
                else{
                    if(gepInst->from->I && gepInst->from->I->type == GEP) return false;
                    auto tt = gepInst->from->type;
                    auto pp = dynamic_cast<PtrType*>(tt.get());
                    newParams.push_back({gepInst->from, totalSize});
                    totalSize+=8;
                    valueMap[gepInst->from] = nullptr;
                }
                
            }
            for(auto it:gepInst->index){
                if(!valueMap.count(it) && !it->isConst){
                    newParams.push_back({it, totalSize});
                    totalSize+=8;
                    valueMap[it] = nullptr;
                }
            }
        }
        else if(inst->type == Icmp){
            continue; // 因为只有一条Icmp，所以这里认为它总是在做(index < loopEnd)的条件判断，其中index在循环内得到，loopEnd在函数入参中得到
        }
        else assert(false && "loopExtract:这是什么类型?");
    }
    return true;
}

VariablePtr makePayload(vector<pair<ValuePtr, int>> &newParams, Module &ir){
    auto payloadType = std::make_shared<ArrType>(Type::getInt8(), totalSize);

    string now_num = to_string(parallelNum);
    VariablePtr payloadVar = std::make_shared<Arr>("parallelForBody_" + now_num, true, false, payloadType);
    ir.addGlobalVariable(payloadVar);
    return payloadVar;
}

void storePayload(LoopPtr loop, vector<pair<ValuePtr, int>> &newParams, unordered_map<ValuePtr, VariablePtr> &payloadMap, Module &ir){
    auto preHeader = loop->preHeader;

    // 不知道为什么一开始terminator是有问题的
    preHeader->terminator = nullptr;
    preHeader->terminator = preHeader->instructions[preHeader->instructions.size()-1];
    auto preHeaderTerminator = preHeader->terminator;

    for(auto it : newParams){
        // vector<ValuePtr> gepIndex;
        // gepIndex.push_back(Const::createConstant(Type::getInt64(), 0));
        // gepIndex.push_back(Const::createConstant(Type::getInt64(), it.second));
        // auto gep = shared_ptr<GetElementPtrInstruction>(new GetElementPtrInstruction(payload, gepIndex, preHeader));

        // // 这里并不是通用操作，只适用于payload为一维的操作，请注意
        // gep->reg->type = make_shared<PtrType>(it.first->type);
        // auto prt = dynamic_cast<PtrType*>(it.first->type.get());
        // //gepPtr->inner = it.first->type;

        // preHeader->insertInstructionBefore(gep, preHeaderTerminator);
        // auto storeInst = shared_ptr<StoreInstruction>(new StoreInstruction(gep->reg, it.first, preHeader));
        // preHeader->insertInstructionBefore(storeInst, preHeaderTerminator);

        VariablePtr payloadVar;
        if(it.first->type->isInt()){
            auto payloadType = std::make_shared<Type>(it.first->type->ID);
            string now_num = to_string(parallelNum);

            payloadVar = std::make_shared<Int>(it.first->type, "parallelForBody_" + it.first->name + "_" + now_num, true, false);
            //payloadVar->type->ID = it.first->type->ID;
            ir.addGlobalVariable(payloadVar);
        }
        else if(it.first->type->isFloat()){
            auto payloadType = std::make_shared<Type>(it.first->type->ID);
            string now_num = to_string(parallelNum);

            payloadVar = std::make_shared<Float>("parallelForBody_" + it.first->name + "_" + now_num, true, false);
            ir.addGlobalVariable(payloadVar);
        }
        else if(it.first->type->isArr()){

            auto arrType = dynamic_cast<ArrType*>(it.first->type.get());
            auto payloadType = std::make_shared<PtrType>(it.first->type);
            string now_num = to_string(parallelNum);

            payloadVar = std::make_shared<Ptr>("parallelForBody_" + it.first->name + "_" + now_num, true, false, payloadType);
            ir.addGlobalVariable(payloadVar);

            
            // auto ptrType = std::make_shared<PtrType>(it.first->type);
            // payloadVar = std::make_shared<Ptr>("parallelForBody_array_" + it.first->name + "_" + now_num, false, false, payloadType);
            // vector<ValuePtr> gepIndex;
            // gepIndex.push_back(Const::createConstant(Type::getInt64(), 0));
            // gepIndex.push_back(Const::createConstant(Type::getInt64(), 0));
            // auto gep = shared_ptr<GetElementPtrInstruction>(new GetElementPtrInstruction(it.first, gepIndex, preHeader));
            // preHeader->insertInstructionBefore(gep, preHeaderTerminator);

            auto storeInst = shared_ptr<StoreInstruction>(new StoreInstruction(payloadVar, it.first, preHeader));
            preHeader->insertInstructionBefore(storeInst, preHeaderTerminator);
            payloadMap[it.first] = payloadVar;
            continue;
        }
        else if(it.first->type->isPtr()){
            auto ptrType = dynamic_cast<PtrType*>(it.first->type.get());
            auto payloadType = std::make_shared<PtrType>(ptrType->inner);
            string now_num = to_string(parallelNum);

            payloadVar = std::make_shared<Ptr>("parallelForBody_" + it.first->name + "_" + now_num, true, false, payloadType);
            ir.addGlobalVariable(payloadVar);
        }
        else{
            assert(false && "loopExtract:出现了错误的type");
        }
        auto storeInst = shared_ptr<StoreInstruction>(new StoreInstruction(payloadVar, it.first, preHeader));
        preHeader->insertInstructionBefore(storeInst, preHeaderTerminator);
        payloadMap[it.first] = payloadVar;
    }
}

static VariablePtr copyVariable(VariablePtr old){
    VariablePtr newVar = Variable::copy(old);
    newVar->name = newVar->name+".loopExtract_copy";
    newVar->useHead = nullptr;
    newVar->useCount = 0;
    return newVar;
}

ValuePtr getNewOperand_extract(ValuePtr old, unordered_map<ValuePtr, ValuePtr>&ValueMap){
    if(old->isConst){
        return old;
    }
    if(ValueMap.find(old) == ValueMap.end()){
        if (!dynamic_pointer_cast<Variable>(old)) {
            ValueMap[old] = make_shared<Reg>(old->type, old->name + ".loopExtract_copy");
        }
        else {
            auto theVar = dynamic_pointer_cast<Variable>(old);
            if(theVar->isGlobal) return theVar;
            ValueMap[old] = copyVariable(dynamic_pointer_cast<Variable>(old));
        }
    }
    return ValueMap[old];
}

InstructionPtr copyInstruction_extract(InstructionPtr old, unordered_map<ValuePtr,ValuePtr>&ValueMap, FunctionPtr func){
    switch (old->type) {
        case Store: {
            auto storeInstr = dynamic_cast<StoreInstruction*>(old.get());
            if(storeInstr->gep){
                GetElementPtrInstruction* newGEP = dynamic_cast<GetElementPtrInstruction*>((ValueMap[storeInstr->gep->reg])->I);
                auto newInstr = std::make_shared<StoreInstruction>(
                    std::dynamic_pointer_cast<GetElementPtrInstruction>(newGEP->getSharedThis()),
                    getNewOperand_extract(storeInstr->value, ValueMap),
                    nullptr
                );
                ValueMap[old->reg] = newInstr->reg;
                return newInstr;  
            }
            else{
                auto newInstr = InstructionPtr(new StoreInstruction(getNewOperand_extract(storeInstr->des,ValueMap),getNewOperand_extract(storeInstr->value,ValueMap),nullptr));
                ValueMap[old->reg] = newInstr->reg;
                return newInstr;   
            }
        }
        case Load: {
            auto loadInstr = dynamic_cast<LoadInstruction*>(old.get());
            auto nreg = func->createRegister(loadInstr->from->type);
            auto newInstr = InstructionPtr(new LoadInstruction(getNewOperand_extract(loadInstr->from, ValueMap), getNewOperand_extract(nreg,ValueMap),nullptr));
            ValueMap[old->reg] = newInstr->reg;
            return newInstr;
        }
        case Call: {
            auto callInstr = dynamic_cast<CallInstruction*>(old.get());
            vector<ValuePtr> newArgv;
            for (const auto& arg : callInstr->argv) {
                newArgv.push_back(getNewOperand_extract(arg, ValueMap));
            }
            auto newInstr =  InstructionPtr(new CallInstruction(callInstr->func, newArgv, nullptr));
            ValueMap[old->reg] = newInstr->reg;
            return newInstr;
        }
        case Bitcast: {
            auto bitcastInstr = dynamic_cast<BitCastInstruction*>(old.get());
            auto newInstr = std::make_shared<BitCastInstruction>(
                getNewOperand_extract(bitcastInstr->from, ValueMap),
                getNewOperand_extract(bitcastInstr->reg, ValueMap),
                nullptr,
                bitcastInstr->targetType
            );
            ValueMap[old->reg] = newInstr->reg;
            return newInstr;
        }
        case Ext: {
            auto extInstr = dynamic_cast<ExtInstruction*>(old.get());
            auto newInstr = std::make_shared<ExtInstruction>(
                getNewOperand_extract(extInstr->from, ValueMap),
                extInstr->to,
                extInstr->isSigned,
                nullptr
            );
            ValueMap[old->reg] = newInstr->reg;
            return newInstr;
        }
        case Sitofp: {
            auto sitofpInstr = dynamic_cast<SignToFloatInstruction*>(old.get());
            auto newInstr =  InstructionPtr(new SignToFloatInstruction(getNewOperand_extract(sitofpInstr->from, ValueMap), nullptr));
            ValueMap[old->reg] = newInstr->reg;
            return newInstr;
        }
        case Fptosi: {
            auto fptosiInstr = dynamic_cast<FloatToSignInstruction*>(old.get());
            auto newInstr =  InstructionPtr(new FloatToSignInstruction(getNewOperand_extract(fptosiInstr->from, ValueMap), nullptr));
            ValueMap[old->reg] = newInstr->reg;
            return newInstr;
        }
        case GEP: {
            auto gepInstr = dynamic_cast<GetElementPtrInstruction*>(old.get());
            vector<ValuePtr> newIndex;
            for(auto oldInd:gepInstr->index){
                newIndex.push_back(getNewOperand_extract(oldInd,ValueMap));
            }
            auto newInstr =  InstructionPtr(new GetElementPtrInstruction(getNewOperand_extract(gepInstr->from,ValueMap),newIndex,nullptr));
            if(gepInstr->from->type->isPtr()){
                newInstr->reg->type = dynamic_cast<PtrType*>(gepInstr->from->type.get())->inner;
            }
            ValueMap[old->reg] = newInstr->reg;
            return newInstr;
        }
        case Binary: {
            auto binaryInstr = dynamic_cast<BinaryInstruction*>(old.get());
            auto newInstr = std::make_shared<BinaryInstruction>(
                getNewOperand_extract(binaryInstr->a, ValueMap),
                getNewOperand_extract(binaryInstr->b, ValueMap),
                binaryInstr->op,
                BasicBlockPtr(nullptr)
            );
            ValueMap[old->reg] = newInstr->reg;
            return newInstr;
        }
        case Fneg: {
            auto fnegInstr = dynamic_cast<FnegInstruction*>(old.get());
            auto newInstr = InstructionPtr(new FnegInstruction(getNewOperand_extract(fnegInstr->a, ValueMap), nullptr));
            ValueMap[old->reg] = newInstr->reg;
            return newInstr;
        }
        case Icmp: {
            auto icmpInstr = dynamic_cast<IcmpInstruction*>(old.get());
            auto newInstr = std::make_shared<IcmpInstruction>(
                nullptr,
                getNewOperand_extract(icmpInstr->a, ValueMap),
                getNewOperand_extract(icmpInstr->b, ValueMap),
                icmpInstr->op
            );
            ValueMap[old->reg] = newInstr->reg;
            return newInstr;
        }
        case Fcmp: {
            auto fcmpInstr = dynamic_cast<FcmpInstruction*>(old.get());
            auto newInstr = std::make_shared<FcmpInstruction>(
                nullptr,
                getNewOperand_extract(fcmpInstr->a, ValueMap),
                getNewOperand_extract(fcmpInstr->b, ValueMap),
                fcmpInstr->op
            );
            ValueMap[old->reg] = newInstr->reg;
            return newInstr;
        }
        case Phi: {
            auto phiInstr = dynamic_cast<PhiInstruction*>(old.get());
            auto newInstr = InstructionPtr(new PhiInstruction(nullptr, getNewOperand_extract(phiInstr->val, ValueMap)));
            ValueMap[old->reg] = newInstr->reg;
            return newInstr;
        }
        case Select:{
            auto selectInstr = dynamic_cast<SelectInstruction*>(old.get());
            if(selectInstr == nullptr)
                assert(false && "wrong ins type");

            auto retIns =  InstructionPtr(new SelectInstruction(
                nullptr, 
                getNewOperand_extract(selectInstr->exp, ValueMap), 
                getNewOperand_extract(selectInstr->val1, ValueMap), 
                getNewOperand_extract(selectInstr->val2, ValueMap)
                ));
            ValueMap[old->reg] = retIns->reg;
            return retIns;
        }
        case Return: {
            assert(false && "loopExtract:不应该出现return的copy");
            return nullptr;
        }
        case Br: {
            assert(false && "loopExtract:不应该出现Br的copy");
            return nullptr;
        }
        case Alloca: {
            assert(false && "loopExtract:不应该出现Alloca的copy");
            return nullptr;
        }
        default:
            assert(false && "loopExtract:Unknown instruction type when copying");
            return nullptr;
    }
}

FunctionPtr extractToFunction(LoopPtr loop, Module &ir, vector<pair<ValuePtr, int>> &newParams, unordered_map<ValuePtr, ValuePtr> &valueMap, unordered_map<ValuePtr, VariablePtr> &payloadMap){
    string now_num = to_string(parallelNum);
    string name = "ParallelLoop_" + now_num;
    auto type = TypePtr(Type::getVoid());

    vector<ValuePtr> params = {std::make_shared<Reg>(Type::getInt(), "indexStart"), std::make_shared<Reg>(Type::getInt(), "indexEnd"),};
    valueMap[loop->loopEnd] = params[1];

    auto func = FunctionPtr(new Function(type, name, params));
    func->initializeReturnBlock();
    // ir.addFunction(func); 不要在这里add，否则可能会导致optimize对func的遍历产生迭代器混乱
    func->returnValue = Void::get();

    // BasicBlockPtr returnBlock = nullptr;
    // for(auto it:func->basicBlocks){
    //     if(it->label->name == "return"){
    //         returnBlock = it;
    //     }
    // }
    
    BasicBlockPtr returnBlock = func->returnBasicBlock;

    assert(returnBlock && "loopExtract:怎么会没有returnBlock的?");

    auto entry = BasicBlockPtr(new BasicBlock(LabelPtr(new Label("entry"))));
    auto header = loop->header;
    auto headerCpy = BasicBlockPtr(new BasicBlock(LabelPtr(new Label(header->label->name + "_parallel"))));
    auto latch = loop->latchBlock;
    auto latchCpy = BasicBlockPtr(new BasicBlock(LabelPtr(new Label(latch->label->name + "_parallel"))));
    for(auto it : newParams){
        // vector<ValuePtr> gepIndex;
        // gepIndex.push_back(Const::createConstant(Type::getInt64(), 0));
        // gepIndex.push_back(Const::createConstant(Type::getInt64(), it.second));
        // auto gep = shared_ptr<GetElementPtrInstruction>(new GetElementPtrInstruction(payload, gepIndex, entry));
        
        // gep->reg->type = make_shared<PtrType>(it.first->type);

        // entry->appendInstruction(gep);
        // auto loadInst = shared_ptr<LoadInstruction>(new LoadInstruction(gep->reg, func->createRegister(it.first->type), entry));
        // valueMap[it.first] = loadInst->reg;
        // entry->appendInstruction(loadInst);
        auto loadInst = shared_ptr<LoadInstruction>(new LoadInstruction(payloadMap[it.first], func->createRegister(it.first->type), entry));
        entry->appendInstruction(loadInst);
        valueMap[it.first] = loadInst->reg;
    }
    auto entryBr = std::make_shared<BrInstruction>(headerCpy->label, entry);
    entry->appendInstruction(entryBr);
    entry->setTerminator(entryBr);

    func->addBasicBlock(entry);
    func->addBasicBlock(headerCpy);
    func->addBasicBlock(latchCpy);

    // 接下来解决friendLoop
    auto friendLoop = loop->friendLoop;
    auto superPreHeader = friendLoop->preHeader;
    auto superPreHeaderCpy = BasicBlockPtr(new BasicBlock(LabelPtr(new Label(superPreHeader->label->name + "_parallel"))));
    auto friendHeader = friendLoop->header;
    auto friendHeaderCpy = BasicBlockPtr(new BasicBlock(LabelPtr(new Label(friendHeader->label->name + "_parallel"))));
    auto friendLatch = friendLoop->latchBlock;
    auto friendLatchCpy = BasicBlockPtr(new BasicBlock(LabelPtr(new Label(friendLatch->label->name + "_parallel"))));

    func->addBasicBlock(superPreHeaderCpy);
    func->addBasicBlock(friendHeaderCpy);
    func->addBasicBlock(friendLatchCpy);


    auto copyInstructions = [&](BasicBlockPtr srcBB, BasicBlockPtr dstBB, bool isFriend) {
        for (auto instr : srcBB->instructions) {
            if (instr->type == Br) continue;
            if (instr->type == Phi) {
                auto phiInstr = dynamic_cast<PhiInstruction*>(instr.get());
                auto newInstr = InstructionPtr(new PhiInstruction(nullptr, getNewOperand_extract(phiInstr->val, valueMap)));
                auto newPhiInstr = dynamic_cast<PhiInstruction*>(newInstr.get());
                for(auto it:phiInstr->from){
                    newPhiInstr->from.push_back(it);
                    if(isFriend){
                        if(it.second == friendLatch){
                            newPhiInstr->from[newPhiInstr->from.size()-1].second = friendLatchCpy;
                        }
                        else{
                            newPhiInstr->from[newPhiInstr->from.size()-1].first = valueMap[it.first]; // loop的start值
                            newPhiInstr->from[newPhiInstr->from.size()-1].second = superPreHeaderCpy;
                        }
                    }
                    else{
                        if(it.second == latch){
                            newPhiInstr->from[newPhiInstr->from.size()-1].second = latchCpy;
                        }
                        else{
                            if(!phiInstr->specialTag){
                                newPhiInstr->from[newPhiInstr->from.size()-1].first = func->formalArguments[0]; // loop的start值
                            }
                            
                            newPhiInstr->from[newPhiInstr->from.size()-1].second = entry;
                        }
                    }
                    
                }
                valueMap[instr->reg] = newInstr->reg;
                dstBB->appendInstruction(newInstr);
                newInstr->basicblock = dstBB;
                continue;
            }
            auto instrCpy = copyInstruction_extract(instr, valueMap, func);
            dstBB->appendInstruction(instrCpy);
            instrCpy->basicblock = dstBB;
            
        }
    };

    copyInstructions(header, headerCpy, false);
    copyInstructions(latch, latchCpy, false);

    auto newInstr_br_super = std::make_shared<BrInstruction>(friendHeaderCpy->label, superPreHeaderCpy);
    superPreHeaderCpy->appendInstruction(newInstr_br_super);
    superPreHeaderCpy->setTerminator(newInstr_br_super);
    copyInstructions(friendHeader, friendHeaderCpy, true);
    copyInstructions(friendLatch, friendLatchCpy, true);

    // 重新设置header的phi
    for(auto instr : headerCpy->instructions){
        if(instr->type == Phi){
            auto phiNode = dynamic_cast<PhiInstruction*>(instr.get());
            for(int i=0;i<phiNode->from.size();i++){
                if(phiNode->from[i].second == latchCpy){
                    phiNode->from[i].first = valueMap[phiNode->from[i].first];
                }
            }
        }
        else break;
    }

    for(auto instr : friendHeaderCpy->instructions){
        if(instr->type == Phi){
            auto phiNode = dynamic_cast<PhiInstruction*>(instr.get());
            for(int i=0;i<phiNode->from.size();i++){
                if(phiNode->from[i].second == friendLatchCpy){
                    phiNode->from[i].first = valueMap[phiNode->from[i].first];
                }
            }
        }
        else break;
    }

    // 维护各个块pred和succ的信息
    entry->addSuccessor(headerCpy);
    headerCpy->addPredecessor(entry);

    headerCpy->addSuccessor(latchCpy);
    latchCpy->addPredecessor(headerCpy);

    latchCpy->addSuccessor(headerCpy);
    headerCpy->addPredecessor(latchCpy);

    headerCpy->addSuccessor(superPreHeaderCpy);
    superPreHeaderCpy->addPredecessor(headerCpy);

    superPreHeaderCpy->addSuccessor(friendHeaderCpy);
    friendHeaderCpy->addPredecessor(superPreHeaderCpy);

    friendHeaderCpy->addSuccessor(friendLatchCpy);
    friendLatchCpy->addPredecessor(friendHeaderCpy);

    friendLatchCpy->addSuccessor(friendHeaderCpy);
    friendHeaderCpy->addPredecessor(friendLatchCpy);

    friendHeaderCpy->addSuccessor(returnBlock);
    returnBlock->addPredecessor(friendHeaderCpy);


    // 手动为headerCpy和latchCpy设置Br指令
    for(auto instr : header->instructions){
        if(instr->type == Br){
            auto brInst = dynamic_cast<BrInstruction*>(instr.get());
            if(brInst->label1 == latch->label){
                auto newInstr = std::make_shared<BrInstruction>(getNewOperand_extract(brInst->exp, valueMap), latchCpy->label, superPreHeaderCpy->label, headerCpy);
                headerCpy->appendInstruction(newInstr);
                headerCpy->setTerminator(newInstr);
                break;
            }
            else{
                assert(false && "loopExtract:设置br时出了问题");
            }
        }
    }
    auto newInstr_br = std::make_shared<BrInstruction>(headerCpy->label, latchCpy);
    latchCpy->appendInstruction(newInstr_br);
    latchCpy->setTerminator(newInstr_br);
    
    
    for(auto instr : friendHeader->instructions){
        if(instr->type == Br){
            auto brInst = dynamic_cast<BrInstruction*>(instr.get());
            if(brInst->label1 == friendLatch->label){
                auto newInstr = std::make_shared<BrInstruction>(getNewOperand_extract(brInst->exp, valueMap), friendLatchCpy->label, returnBlock->label, friendHeaderCpy);
                friendHeaderCpy->appendInstruction(newInstr);
                friendHeaderCpy->setTerminator(newInstr);
                break;
            }
            else{
                assert(false && "loopExtract:设置br时出了问题");
            }
        }
    }
    auto newInstr_br_friendLatch = std::make_shared<BrInstruction>(friendHeaderCpy->label, friendLatchCpy);
    friendLatchCpy->appendInstruction(newInstr_br_friendLatch);
    friendLatchCpy->setTerminator(newInstr_br_friendLatch);

    func->allocateLocalVariables();
    func->assignBlocksToFunction();
    // func->processReturnBlock(); // 这个要跟下面这条结合着用才有用，所以也删了吧
    // func->processEndInstructions(); // 这个没必要了，只是处理retValue，并且再搞一下terminator而已，我前面已经做了，就不要重复了

    auto retInst = make_shared<ReturnInstruction>(func->returnValue, func->returnBasicBlock);
    func->returnBasicBlock->appendInstruction(retInst);
    func->returnBasicBlock->setTerminator(retInst);
    func->addBasicBlock(func->returnBasicBlock);

    func->isReentrant = false;

    // 将旧的块从原来的函数中删除
    auto oldFunc = loop->header->parentFunction;
    vector<BasicBlockPtr> newBasicblocks;
    for(auto it:oldFunc->basicBlocks){
        if(it==header || it==latch|| it==superPreHeader || it==friendHeader || it==friendLatch){

        }
        else newBasicblocks.push_back(it);
    }
    oldFunc->basicBlocks = newBasicblocks;

    return func;
}

void processAtomicAdd(LoopPtr loop, FunctionPtr func, unordered_map<ValuePtr, ValuePtr> &valueMap){
    auto exitBlock = *loop->exitBlocks.begin();
    unordered_set<int> instToerase;
    for(int i=0; i<exitBlock->instructions.size();i++){
        auto inst = exitBlock->instructions[i];
        if(inst->type == Store){
            auto storeInst = dynamic_cast<StoreInstruction*>(inst.get());
            auto storeVar = dynamic_cast<Variable*>(storeInst->des.get());
            if(storeVar && storeVar->isGlobal){
                if(i>=1){
                    auto lastInst = exitBlock->instructions[i-1];
                    if(lastInst->type == Binary){
                        auto binaryInst = dynamic_cast<BinaryInstruction*>(lastInst.get());
                        if(binaryInst->op=='+' && binaryInst->reg == storeInst->value){
                            auto returnBlock = func->returnBasicBlock;
                            //assert(returnBlock->instructions.size()==1);
                            auto leftVal = binaryInst->a;
                            auto atomicAdd = shared_ptr<Instruction>(new AtomicAddInstruction(valueMap[binaryInst->a], storeInst->des, returnBlock));
                            vector<InstructionPtr> newInstructions;
                            newInstructions.push_back(atomicAdd);
                            for(auto oldInst:returnBlock->instructions){
                                newInstructions.push_back(oldInst);
                            }
                            returnBlock->instructions = newInstructions;
                            if(i>=2){
                                auto llastInst = exitBlock->instructions[i-2];
                                if(llastInst->type == Load){
                                    auto llastLoad = dynamic_cast<LoadInstruction*>(llastInst.get());
                                    if(llastLoad->from == storeInst->des){
                                        instToerase.insert(i-2);
                                    }
                                }
                            }
                            instToerase.insert(i-1);
                            instToerase.insert(i);
                        }
                    }
                }
            }
        }
    }
    vector<InstructionPtr> newInstructions;
    for(int i=0;i<exitBlock->instructions.size();i++){
        if(!instToerase.count(i)){
            newInstructions.push_back(exitBlock->instructions[i]);
        }
    }
    exitBlock->instructions = newInstructions;
}

FunctionPtr loopExtract(LoopPtr loop, Module &ir){
    // 进来这里之前应该要有一些条件
    // header、latch只能有且仅有一个，loop只有这两个块
    // 整个loop是void的，即循环外没有指令用到循环内的值
    // 只能有一条icmp指令（icmp都是用来给branch做条件的）
    if(!loop->hasPartiallyUnroll) return nullptr;
    if(loop->step != 1) return nullptr;
    vector<pair<ValuePtr, int>> newParams;
    unordered_map<ValuePtr, ValuePtr> valueMap;
    unordered_map<ValuePtr, VariablePtr> payloadMap;

    totalSize = 0;
    if(!scanParams(loop->header, newParams, valueMap, loop)) return nullptr;
    if(!scanParams(loop->latchBlock, newParams, valueMap, loop)) return nullptr;

    // auto payloadVar = makePayload(newParams, ir);

    storePayload(loop, newParams, payloadMap, ir);

    auto func = extractToFunction(loop, ir, newParams, valueMap, payloadMap);
    processAtomicAdd(loop->friendLoop, func, valueMap);
    parallelNum++;

    return func;
}