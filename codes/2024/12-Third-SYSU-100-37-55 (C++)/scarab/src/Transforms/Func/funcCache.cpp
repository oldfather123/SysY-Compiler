#include "funcCache.h"

int recursiveCount(FunctionPtr func){
    uint32_t cnt = 0;
    for(auto block : func->basicBlocks){
        for(auto& inst : block->instructions) {
            if(inst->type == InsID::Call) {
                auto callInst = dynamic_cast<CallInstruction*>(inst.get());
                if(callInst->func->name == func->name){
                    cnt++;
                }
                else return false;
            }
        }
    }
    return cnt;
}

bool isComplicatedFunc(FunctionPtr func){
    for(auto block : func->basicBlocks){
        for(auto& inst : block->instructions) {
            if(inst->type == InsID::Return){
                auto returnInst = dynamic_cast<ReturnInstruction*>(inst.get());
                for(auto& inst1 : block->instructions){
                    if(returnInst->retValue == inst1->reg){
                        if(inst1->type == InsID::Phi){
                            auto phiNode = dynamic_cast<PhiInstruction*>(inst1.get());
                            return phiNode->from.size()>=3;
                        }
                    }
                }
            }
        }
    }
    return false;
}

bool funcCache(FunctionPtr func, Module &ir){
    // 还是可能存在哈希冲突的隐患，可以通过扩大数组大小的方法来缓解
    if(recursiveCount(func)<2){
        return false;
    }
    auto retType = func->returnValue->type;
    if(!(retType->isInt() || retType->isFloat())){
        return false;
    }
    vector<ValuePtr> funcArgs;
    for(auto it:func->formalArguments){
        if(it->type->isInt() || it->type->isFloat()){
            funcArgs.push_back(it);
        }
    }
    if(funcArgs.size() > 2 || funcArgs.size()==0){
        return false;
    }
    if(isComplicatedFunc(func)){
        return false;
    }
    auto returnBlock = func->returnBasicBlock;
    for(auto &inst: returnBlock->instructions){
        if(inst->type == InsID::Return){
            auto returnInst = dynamic_cast<ReturnInstruction*>(inst.get());
            if(returnInst->retValue->isConst){
                return false;                    
            }
        }
    }

    auto oldEntryBlock = func->getEntryBlock();
    
    LabelPtr cachePreprocessBlockLabel = LabelPtr(new Label("cachePreprocess"));
    auto cachePreprocessBlock = BasicBlockPtr(new BasicBlock(cachePreprocessBlockLabel));
    func->basicBlocks.emplace(func->basicBlocks.begin(), cachePreprocessBlock);
    cachePreprocessBlock->parentFunction = func;
    cerr << "new-2\n";
    LabelPtr cacheBlockLabel = LabelPtr(new Label("cacheBlock"));
    auto cacheBlock = BasicBlockPtr(new BasicBlock(cacheBlockLabel));
    func->basicBlocks.push_back(cacheBlock);
    cacheBlock->parentFunction = func;
    cerr << "new-1\n";
    for(int i=0;i<funcArgs.size();i++){
        auto arg = funcArgs[i];
        if(arg->type->isFloat()){
            auto fp2SiInst = shared_ptr<FloatToSignInstruction>(new FloatToSignInstruction(arg, cachePreprocessBlock));
            cachePreprocessBlock->appendInstruction(fp2SiInst);
            funcArgs[i]=fp2SiInst->reg;
        }
    }
    cerr << "newnew\n";
    auto preprocessCmpInst = shared_ptr<IcmpInstruction>(new IcmpInstruction(cachePreprocessBlock, funcArgs[0], Const::createConstant(Type::getInt(), int(0)), ">="));
    cachePreprocessBlock->appendInstruction(preprocessCmpInst);
    cerr << "new0\n";
    if(funcArgs.size()==2){
        LabelPtr cachePreprocessBlockLabel2 = LabelPtr(new Label("cachePreprocess2"));
        auto cachePreprocessBlock2 = BasicBlockPtr(new BasicBlock(cachePreprocessBlockLabel2));
        func->basicBlocks.push_back(cachePreprocessBlock2);
        cachePreprocessBlock2->parentFunction = func;

        auto preprocessCondBr = shared_ptr<BrInstruction>(new BrInstruction(preprocessCmpInst->reg, cachePreprocessBlock2->label, oldEntryBlock->label, cachePreprocessBlock));
        cachePreprocessBlock->appendInstruction(preprocessCondBr);
        cachePreprocessBlock->setTerminator(preprocessCondBr);

        auto preprocessCmpInst2 = shared_ptr<IcmpInstruction>(new IcmpInstruction(cachePreprocessBlock2, funcArgs[1], Const::createConstant(Type::getInt(), int(0)), ">="));
        cachePreprocessBlock2->appendInstruction(preprocessCmpInst2);
        auto preprocessCondBr2 = shared_ptr<BrInstruction>(new BrInstruction(preprocessCmpInst2->reg, cacheBlock->label, oldEntryBlock->label, cachePreprocessBlock2));
        cachePreprocessBlock2->appendInstruction(preprocessCondBr2);
        cachePreprocessBlock2->setTerminator(preprocessCondBr2);
    }
    else{
        auto preprocessCondBr = shared_ptr<BrInstruction>(new BrInstruction(preprocessCmpInst->reg, cacheBlock->label, oldEntryBlock->label, cachePreprocessBlock));
        cachePreprocessBlock->appendInstruction(preprocessCondBr);
        cachePreprocessBlock->setTerminator(preprocessCondBr);
    }

    ValuePtr raw_index = funcArgs[0];
    cerr << "new1\n";
    if(funcArgs.size()==2){
        auto shlInst = shared_ptr<BinaryInstruction>(new BinaryInstruction(funcArgs[1], Const::createConstant(Type::getInt(), int(16)), ',', cacheBlock));
        cacheBlock->appendInstruction(shlInst);
        //funcArgs[1] = shlInst->reg;
        auto addInst = shared_ptr<BinaryInstruction>(new BinaryInstruction(funcArgs[0], shlInst->reg, '+', cacheBlock));
        cacheBlock->appendInstruction(addInst);
        //funcArgs[0] = addInst->reg;
        raw_index = addInst->reg;
    }
    auto remInst = shared_ptr<BinaryInstruction>(new BinaryInstruction(raw_index, Const::createConstant(Type::getInt(), int(2039)), '%', cacheBlock));
    cacheBlock->appendInstruction(remInst);
    auto cacheIndex = remInst->reg;
    cerr << "new2\n";
    auto visitArrType = shared_ptr<ArrType>(new ArrType(TypePtr(new Type(TypeID::IntID)), int(2048)));
    auto cacheArrType = shared_ptr<ArrType>(new ArrType(retType, 2048));
    VariablePtr visitArr = shared_ptr<Arr>(new Arr("visit", true, false, visitArrType));
    VariablePtr cacheArr = shared_ptr<Arr>(new Arr("cache", true, false, cacheArrType));
    ir.addGlobalVariable(visitArr);
    ir.addGlobalVariable(cacheArr);

    vector<ValuePtr> visitIndex;
    visitIndex.push_back(Const::createConstant(Type::getInt64(), (long long)(0)));
    auto sextIns = shared_ptr<ExtInstruction>(new ExtInstruction(cacheIndex, Type::getInt64(), true, cacheBlock));
    cacheBlock->appendInstruction(sextIns);
    visitIndex.push_back(sextIns->reg);
    auto visitGEP = shared_ptr<GetElementPtrInstruction>(new GetElementPtrInstruction(visitArr, visitIndex, cacheBlock));
    cacheBlock->appendInstruction(visitGEP);
    auto visitLoadInst = shared_ptr<LoadInstruction>(new LoadInstruction(visitGEP->reg, func->createRegister(visitGEP->reg->type), cacheBlock));
    cacheBlock->appendInstruction(visitLoadInst);
    auto cmpInst = shared_ptr<IcmpInstruction>(new IcmpInstruction(cacheBlock, visitLoadInst->reg));
    cacheBlock->appendInstruction(cmpInst);

    LabelPtr earlyExitBlockLabel = LabelPtr(new Label("earlyExit"));
    auto earlyExitBlock = BasicBlockPtr(new BasicBlock(earlyExitBlockLabel));
    func->basicBlocks.push_back(earlyExitBlock);
    earlyExitBlock->parentFunction = func;

    auto condBr = shared_ptr<BrInstruction>(new BrInstruction(cmpInst->reg, earlyExitBlock->label, func->basicBlocks[1]->label, cacheBlock));
    cacheBlock->appendInstruction(condBr);
    cacheBlock->setTerminator(condBr);

    auto cacheGEP = shared_ptr<GetElementPtrInstruction>(new GetElementPtrInstruction(cacheArr, visitIndex, earlyExitBlock));
    earlyExitBlock->appendInstruction(cacheGEP);
    auto cacheLoadInst = shared_ptr<LoadInstruction>(new LoadInstruction(cacheGEP->reg, func->createRegister(cacheGEP->reg->type), earlyExitBlock));
    earlyExitBlock->appendInstruction(cacheLoadInst);
    auto br2ret = InstructionPtr(new BrInstruction(func->returnBasicBlock->label, earlyExitBlock));
    earlyExitBlock->appendInstruction(br2ret);
    earlyExitBlock->setTerminator(br2ret);
    for(auto &inst: returnBlock->instructions){
        if(inst->type == InsID::Return){
            auto returnInst = dynamic_cast<ReturnInstruction*>(inst.get());
            for(auto &inst2: returnBlock->instructions){
                if(inst2->reg == returnInst->retValue){
                    if(inst2->type == InsID::Phi){
                        auto phiNode = dynamic_cast<PhiInstruction*>(inst2.get());
                        for(auto it:phiNode->from){
                            if(it.first->isConst) continue;
                            cerr << "cache1\n";
                            auto predBlock = it.second;
                            auto oldFirst = predBlock->instructions[0];
                            if(funcArgs.size()==2){
                                auto shlInst_new = shared_ptr<BinaryInstruction>(new BinaryInstruction(funcArgs[1], Const::createConstant(Type::getInt(), int(16)), ',', predBlock));
                                predBlock->insertInstructionBefore(shlInst_new, oldFirst);
                                funcArgs[1] = shlInst_new->reg;
                                auto addInst_new = shared_ptr<BinaryInstruction>(new BinaryInstruction(funcArgs[0], funcArgs[1], '+', predBlock));
                                predBlock->insertInstructionBefore(addInst_new, oldFirst);
                                funcArgs[0] = addInst_new->reg;
                            }
                            auto remInst_new = shared_ptr<BinaryInstruction>(new BinaryInstruction(funcArgs[0], Const::createConstant(Type::getInt(), int(2039)), '%', predBlock));
                            predBlock->insertInstructionBefore(remInst_new, oldFirst);
                            auto cacheIndex_new = remInst_new->reg;

                            vector<ValuePtr> visitIndex_new;
                            visitIndex_new.push_back(Const::createConstant(Type::getInt64(), (long long)(0)));
                            auto sextIns = shared_ptr<ExtInstruction>(new ExtInstruction(cacheIndex_new, Type::getInt64(), true, predBlock));
                            predBlock->insertInstructionBefore(sextIns, oldFirst);
                            visitIndex_new.push_back(sextIns->reg);

                            auto visitGEP_new = shared_ptr<GetElementPtrInstruction>(new GetElementPtrInstruction(visitArr, visitIndex_new, predBlock));
                            predBlock->insertInstructionBefore(visitGEP_new, predBlock->terminator);
                            auto visitStore = shared_ptr<StoreInstruction>(new StoreInstruction(visitGEP_new->reg, Const::createConstant(Type::getInt(), int(1)), predBlock));
                            cerr << "cache2\n";
                            predBlock->insertInstructionBefore(visitStore, predBlock->terminator);
                            auto cacheGEP2 = shared_ptr<GetElementPtrInstruction>(new GetElementPtrInstruction(cacheArr, visitIndex_new, predBlock));
                            cerr << "cache3\n";
                            predBlock->insertInstructionBefore(cacheGEP2, predBlock->terminator);
                            auto cacheStoreInst = shared_ptr<StoreInstruction>(new StoreInstruction(cacheGEP2->reg, it.first, predBlock));
                            predBlock->insertInstructionBefore(cacheStoreInst, predBlock->terminator);
                        }
                        phiNode->addFrom(cacheLoadInst->to, earlyExitBlock);
                    }
                    else {
                        cout << "BaoBaoBaoBaoBao\n";
                        return false;
                    }
                }
            }
            break;
        }
    }
    return true;
}
