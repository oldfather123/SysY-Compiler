#include "partialUnrollLoop.h"

// 有可能会出问题，优先考虑loopUnroll里面的updateCurrent那个函数，有可能是它出问题，需要把reg也放valueMap才行
// 也有可能是部分循环展开时的比较策略不太对（cmp和加减乘除，这里需要着重看一下）

static int partial_unroll_cnt =0;
#define PARTIAL_UNROLL_NUM 4

void updateHeaderPhiNode(BasicBlockPtr header, std::unordered_map<ValuePtr, ValuePtr>& CurrentIncomingValue) {
    for(auto it: header->instructions){
        if(it->type == InsID::Phi){
            auto phiNode = dynamic_cast<PhiInstruction*>(it.get());
            for(int i=0;i<phiNode->from.size();i++){
                if(CurrentIncomingValue.count(phiNode->from[i].first)){
                    phiNode->from[i].first = CurrentIncomingValue[phiNode->from[i].first];
                }
            }
        }
        else break;
    }
}

std::pair<ValuePtr, ValuePtr> calLoopTripData(LoopPtr loop){
    auto header = loop->header;
    auto latch = loop->latchBlock;
    vector<std::pair<ValuePtr,ValuePtr>> latentIndex;
    ValuePtr loopIndex = nullptr;
    ValuePtr loopStride = nullptr;
    for (auto instr : header->instructions) {
        if (instr->type == Phi) {
            auto phi = dynamic_cast<PhiInstruction*>(instr.get());
            for (auto iter : phi->from) {
                if (iter.second == latch) {
                    latentIndex.push_back({phi->reg,iter.first});
                }
            }
        }
        else if(instr->type == Icmp){
            auto icmpInst = dynamic_cast<IcmpInstruction*>(instr.get());
            for(auto it:latentIndex){
                if(it.first==icmpInst->a){
                    loopIndex = icmpInst->a;
                    //loopBound = icmpInst->b;
                    loop->loopEnd = icmpInst->b;
                }
            }
        }
    }
    for (auto instr : latch->instructions) {
        if (instr->type == Binary) {
            auto binaryInst = dynamic_cast<BinaryInstruction*>(instr.get());
            if(binaryInst->a == loopIndex){
                loopStride = binaryInst->b;
            }
        }
    }
    if(loopStride == nullptr)return{nullptr, nullptr};
    return {loopIndex, loopStride};
}

void remapHeaderPhi(BasicBlockPtr preheader, BasicBlockPtr header, BasicBlockPtr headerCpy, BasicBlockPtr superPre, unordered_map<ValuePtr, ValuePtr>& CurrentIncomingValue, std::unordered_map<ValuePtr, ValuePtr> &valueMapInv){
    preheader->removeSuccessor(header);
    header->removePredecessor(preheader);
    header->addPredecessor(superPre);
    superPre->addSuccessor(header);

    for(int i=0;i<header->instructions.size();i++){
        auto inst = header->instructions[i];
        if(inst->type == InsID::Phi){
            auto phiNode = dynamic_cast<PhiInstruction*>(inst.get());
            for(int j=0;j<phiNode->from.size();j++){
                if(phiNode->from[j].second == preheader){
                    phiNode->from[j].second = superPre;
                    auto newPhiNode = dynamic_cast<PhiInstruction*>(headerCpy->instructions[i].get());
                    phiNode->from[j].first = newPhiNode->reg;
                }
            }
        }
        else break;
    }
}

InstructionPtr scanForIndex(BasicBlockPtr latch, BasicBlockPtr header, ValuePtr indexVal){
    ValuePtr indexCore;
    for(auto inst:header->instructions){
        if(inst->type == InsID::Phi){
            if(inst->reg == indexVal){
                auto phiNode = dynamic_cast<PhiInstruction*>(inst.get());
                for(auto it:phiNode->from){
                    if(it.second == latch){
                        indexCore = it.first;
                    }
                }
            }
        }
        else break;
    }
    assert(indexCore && "怎么找不到了？");
    for(auto inst:latch->instructions){
        if(inst->type == InsID::Binary){
            if(inst->reg == indexCore){
                auto binaryInst = dynamic_cast<BinaryInstruction*>(inst.get());
                return inst;
            }
        }
    }
    return nullptr; // 有可能是index用了select指令，所以就不能做部分循环展开了
    // assert(false && "怎么找不到了？");
    // return ' ';
}

bool checkStrideOverflow(BasicBlockPtr header, BasicBlockPtr latch){
    for(auto inst:header->instructions){
        if(inst->type == InsID::Binary){
            auto binaryInst = dynamic_cast<BinaryInstruction*>(inst.get());
            if(binaryInst->b->isConst){
                auto constVal = dynamic_cast<Const*>(binaryInst->b.get());
                if(constVal->intVal >= 1000){
                    return true;
                }
            }
        }
    }
    for(auto inst:latch->instructions){
        if(inst->type == InsID::Binary){
            auto binaryInst = dynamic_cast<BinaryInstruction*>(inst.get());
            if(binaryInst->b->isConst){
                auto constVal = dynamic_cast<Const*>(binaryInst->b.get());
                if(constVal->intVal >= 1000){
                    return true;
                }
            }
        }
    }
    return false;
}

void resetPredSucc(BasicBlockPtr preheader, BasicBlockPtr header, BasicBlockPtr superPre, BasicBlockPtr latch){
    preheader->addSuccessor(header);
    header->addPredecessor(preheader);
    header->addPredecessor(latch);
    latch->addSuccessor(header);
    header->addSuccessor(superPre);
    superPre->addPredecessor(header);
}

LoopPtr copyBigLoop(LoopPtr loop, unordered_map<ValuePtr, ValuePtr>& CurrentIncomingValue, FunctionPtr func){
    auto loopTripData = calLoopTripData(loop);
    if(loopTripData.first == nullptr) return nullptr;
    auto header = loop->header;
    auto latch = loop->latchBlock;
    auto preheader = loop->preHeader;
    InstructionPtr indexInst = scanForIndex(latch, header, loopTripData.first);
    if(indexInst == nullptr) return nullptr;
    auto indexBinaryInst = dynamic_cast<BinaryInstruction*>(indexInst.get());
    char binaryOp = indexBinaryInst->op;
    if(binaryOp != '+' && binaryOp != '-'){
        return nullptr;
    }
    if(checkStrideOverflow(header, latch)) return nullptr;

    string now_num = to_string(partial_unroll_cnt++);
    std::unordered_map<ValuePtr, ValuePtr> valueMap;
    std::unordered_map<ValuePtr, ValuePtr> valueMapInv;
    for (auto instr : header->instructions) {
        if (instr->type == Phi) {
            auto phi = dynamic_cast<PhiInstruction*>(instr.get());
            for (auto iter : phi->from) {
                if (iter.second == latch) {
                    valueMap[phi->reg] = iter.first;
                    valueMapInv[iter.first] = phi->reg;
                }
            }
        }
        else {
            break;
        }
    }
    LabelPtr headerLabel = LabelPtr(new Label(loop->header->label->name+".partialUnrollCopy" + now_num));
    auto headerCpy = BasicBlockPtr(new BasicBlock(headerLabel));

    LabelPtr latchLabel = LabelPtr(new Label(loop->latchBlock->label->name+".partialUnrollCopy" + now_num));
    auto latchCpy = (loop->header == loop->latchBlock) ? headerCpy : BasicBlockPtr(new BasicBlock(latchLabel));
    auto loopCpy = LoopPtr(new Loop(headerCpy,1));
    
    loopCpy->latchBlock = latchCpy;

    LabelPtr superPreheaderLabel = LabelPtr(new Label("superPreheader" + now_num));
    auto superPreheader = BasicBlockPtr(new BasicBlock(superPreheaderLabel));
    auto superBRInst = shared_ptr<BrInstruction>(new BrInstruction(header->label, superPreheader));
    superPreheader->appendInstruction(superBRInst);
    superPreheader->setTerminator(superBRInst);

    unordered_map<LabelPtr, LabelPtr> LabelMap;
    unordered_map<ValuePtr, ValuePtr> mycopyMap;
    unordered_map<BasicBlockPtr, BasicBlockPtr> loopBBMap;
    std::unordered_map<PhiInstruction*, PhiInstruction*> queuePhi;

    for(auto bb : loop->basicBlockSet) {
        if(bb == loop->header) {
            loopBBMap[bb] = headerCpy;
            LabelMap[bb->label] = headerCpy->label;
        }
        else if(bb == loop->latchBlock) {
            loopBBMap[bb] = latchCpy;
            LabelMap[bb->label] = latchCpy->label;
            loopCpy->addBasicBlock(latchCpy);
        }
        else {
            LabelPtr BBLabel = LabelPtr(new Label(bb->label->name+".partialUnrollCopy"));
            auto loopBodyCpy = BasicBlockPtr(new BasicBlock(BBLabel));
            loopBBMap[bb] = loopBodyCpy;
            LabelMap[bb->label] = BBLabel;
            loopCpy->addBasicBlock(loopBodyCpy);
        }
    }

    LabelMap[(*loop->exitBlocks.begin())->label] = (*loop->exitBlocks.begin())->label; 
    vector<pair<InstructionPtr, ValuePtr>> allPhiStore;
    cerr << "hh2" << endl;
    auto copyInstructions = [&](BasicBlockPtr srcBB, BasicBlockPtr dstBB, int cnt) {
        
        for (auto instr : srcBB->instructions) {
            if(instr->type == Br){
                if(cnt != PARTIAL_UNROLL_NUM-1){
                    continue;
                }
            }
            if (instr->type == Phi) {
                auto phiInstr = dynamic_cast<PhiInstruction*>(instr.get());
                auto newInstr = InstructionPtr(new PhiInstruction(nullptr, mapOperandForUnroll(phiInstr->val, CurrentIncomingValue)));
                auto newPhiInstr = dynamic_cast<PhiInstruction*>(newInstr.get());
                if(phiInstr->specialTag) newPhiInstr->specialTag=true;
                for(auto it:phiInstr->from){
                    newPhiInstr->from.push_back(it);
                    if(it.second == latch){
                        newPhiInstr->from[newPhiInstr->from.size()-1].second = latchCpy;
                        allPhiStore.push_back({instr, it.first});
                    }
                }
                CurrentIncomingValue[instr->reg] = newInstr->reg;
                dstBB->appendInstruction(newInstr);
                newInstr->basicblock = dstBB;
                continue;
            }
            auto instrCpy = cloneInstructionForUnroll(instr, CurrentIncomingValue, LabelMap, func);
            dstBB->appendInstruction(instrCpy);
            instrCpy->basicblock = dstBB;
            
            if (instrCpy->type == Binary && instr->type == Binary) {
                auto bi = dynamic_cast<BinaryInstruction*>(instr.get());
                auto biCpy = dynamic_cast<BinaryInstruction*>(instrCpy.get());
                mycopyMap[bi->reg] = biCpy->reg;
            }
            if (instrCpy->type == Load && instr->type == Load) {
                auto ldr = dynamic_cast<LoadInstruction*>(instr.get());
                auto ldrCpy = dynamic_cast<LoadInstruction*>(instrCpy.get());
                mycopyMap[ldr->reg] = ldrCpy->reg;
            }
            if (instr->type == Phi) {
                auto phi = dynamic_cast<PhiInstruction*>(instr.get());
                if (instrCpy->type == Phi) {
                    auto phiCpy = dynamic_cast<PhiInstruction*>(instrCpy.get());
                    queuePhi[phi] = phiCpy;
                }
            }
            if (instr->type == Br){
                dstBB->setTerminator(instrCpy);
            }
        }
        for(auto it:allPhiStore){
            if(cnt != PARTIAL_UNROLL_NUM-1){
                CurrentIncomingValue[it.first->reg] = CurrentIncomingValue[it.second];
            }
        }
        
        
    };

    copyInstructions(loop->header, headerCpy, PARTIAL_UNROLL_NUM-1);
    cerr << "hh2.5" << endl;
    auto mulIndexInst = shared_ptr<BinaryInstruction>(new BinaryInstruction(loopTripData.second, Const::createConstant(Type::getInt(), 4), '*', headerCpy));
    auto addIndexInst = shared_ptr<BinaryInstruction>(new BinaryInstruction(CurrentIncomingValue[loopTripData.first], mulIndexInst->reg, binaryOp, headerCpy));
    cerr << "hh3" << endl;
    for(auto inst:headerCpy->instructions){
        if(inst->type == InsID::Icmp){
            auto icmpInst = dynamic_cast<IcmpInstruction*>(inst.get());
            headerCpy->insertInstructionBefore(mulIndexInst, inst);
            headerCpy->insertInstructionBefore(addIndexInst, inst);
            if(icmpInst->a == CurrentIncomingValue[loopTripData.first]){
                icmpInst->a = addIndexInst->reg;
            }
            else{
                icmpInst->b = addIndexInst->reg;
            }
            break;
        }
    }
    for(int i=0;i<PARTIAL_UNROLL_NUM;i++){
        copyInstructions(loop->latchBlock, latchCpy, i);
    }
    cerr << "hh4" << endl;

    for (const auto& [bb, bbCpy] : loopBBMap) {
        if (bb != loop->header) {
            for (auto pred : bb->predBasicBlocks) {
                bbCpy->predBasicBlocks.insert(loopBBMap[pred]);
            }
        }
        if (bb != loop->latchBlock) {
            for (auto succ : bb->succBasicBlocks) {
                bbCpy->succBasicBlocks.insert(loopBBMap[succ]);
            }
        }
    }

    for (const auto& [phi, phiCpy] : queuePhi) {
        for (const auto& [val, pred] : phi->from) {
            phiCpy->addFrom(val, loopBBMap[pred]);
        }
    }

    std::unordered_map<PhiInstruction*, ValuePtr> phiAssignLater;
    for(auto instr : loop->header->instructions) {
        if(instr->type==Phi) {
            auto phi = dynamic_cast<PhiInstruction*>(instr.get());
            for (const auto& [val, pred] : phi->from) {
                if (pred == loop->latchBlock) {
                    phiAssignLater[phi] = val;
                }
            }
        }
        else {
            break;
        }
    }

    for (const auto& [phi, val] : phiAssignLater) {
        if(mycopyMap.find(val) == mycopyMap.end()) {
            cout<< "Cannot unroll loop!!!" << endl;
        }
        assert(mycopyMap.find(val)!=mycopyMap.end());
        CurrentIncomingValue[phi->reg] = mycopyMap[val];
    }
    
    for(auto inst:headerCpy->instructions){
        if(inst->type == InsID::Br){
            auto brInst = dynamic_cast<BrInstruction*>(inst.get());
            if(brInst->label1 == latchCpy->label){
                brInst->label2 = superPreheaderLabel;
            }
            else{
                brInst->label1 = superPreheaderLabel;
            }
        }
    }

    for(auto inst: header->instructions){
        if(inst->type == InsID::Phi){
            auto phiNode = dynamic_cast<PhiInstruction*>(inst.get());
            for(auto it:phiNode->from){
                if(it.second == preheader){
                    it.second = superPreheader;
                }
            }
        }
    }

    for(auto inst:preheader->instructions){
        if(inst->type == InsID::Br){
            auto brInst = dynamic_cast<BrInstruction*>(inst.get());
            if(brInst->label1 == header->label){
                brInst->label1 = headerCpy->label;
            }
            else{
                assert(false && "这啥玩意嘞");
            }
        }
    }
    auto itt = dynamic_cast<PhiInstruction*>(headerCpy->instructions[0].get());
    remapHeaderPhi(preheader, header, headerCpy, superPreheader, CurrentIncomingValue, valueMapInv);
    auto itt2 = dynamic_cast<PhiInstruction*>(headerCpy->instructions[0].get());
    resetPredSucc(preheader, headerCpy, superPreheader, latchCpy);
    updateHeaderPhiNode(headerCpy, CurrentIncomingValue);
    func->basicBlocks.push_back(headerCpy);
    func->basicBlocks.push_back(latchCpy);
    func->basicBlocks.push_back(superPreheader);

    loop->indexPhi = loopTripData.first->I;
    loopCpy->indexPhi = CurrentIncomingValue[loopTripData.first]->I;
    loopCpy->indexBinary = indexInst;
    loopCpy->hasPartiallyUnroll = true;
    if(loopTripData.second->isConst){
        auto stepConst = dynamic_cast<Const*>(loopTripData.second.get());
        loopCpy->step = stepConst->intVal;
    }

    // 维护一些信息
    loopCpy->preHeader = loop->preHeader;
    loopCpy->exitingBlocks = {header};
    loopCpy->exitBlocks = {superPreheader};
    loop->preHeader = superPreheader;
    loopCpy->friendLoop = loop;

    loopCpy->loopEnd = loop->loopEnd;
    
    return loopCpy;
}

bool checkLegalLoop_partial(LoopPtr loop){
    int minTripCount = 64;
    auto iterationCount = loop->iterationCount;
    if(iterationCount == 0){
        // dynamic 
    }
    else if(iterationCount <= minTripCount){
        return false;
    }
    
    if(loop->exitBlocks.size() != 1 || loop->exitingBlocks.size() != 1 || loop->header != *loop->exitingBlocks.begin()){
        return false;
    }

    int instCount = 0;
    for (auto bb : loop->basicBlockSet) {
        for (auto instr : bb->instructions) {
            instCount++;
            if (instr->type == Call) return false;
            if (instr->type == Icmp){
                auto icmpInst = dynamic_cast<IcmpInstruction*>(instr.get());
                string icmpOp = icmpInst->op;
                if(icmpOp == "==" || icmpOp == "!=")return false;
            }
            if (instr->type == Br){
                auto brInst = dynamic_cast<BrInstruction*>(instr.get());
                if(brInst->label1!=nullptr && brInst->label2!=nullptr && bb != loop->header){
                    return false;
                }
            }
        }
    }
    if(instCount>32){
        return false;
    }
    return true;
}

bool runUnrollLoop(LoopPtr loop, FunctionPtr func) {
    int maxTripCount = 80;
    if(!checkLegalLoop_partial(loop)){
        return false;
    }

    auto header = loop->header;
    auto latch = loop->latchBlock;
    auto exit = *loop->exitBlocks.begin();

    unordered_set<ValuePtr> phiSet;
    bool phiFlag =  false;
    for (auto instr : header->instructions) {
        if (instr->type == Phi) {
            auto phi = dynamic_cast<PhiInstruction*>(instr.get());
            for (auto iter : phi->from) {
                if (iter.second == latch) {
                    phiSet.insert(iter.first);
                }
            }
        }
        else {
            break;
        }
    }

    for (auto instr : header->instructions) {
        if (instr->type == Phi) {
            if (phiSet.count(dynamic_cast<PhiInstruction*>(instr.get())->reg)) {
                phiFlag = true;
                break;
            }
        } else {
            break;
        }
    }

    if(phiFlag) {
        cout<< "Cannot Unroll due to phi loop" << endl;
        return false;
    }

    std::unordered_map<ValuePtr, ValuePtr> CurrentIncomingValue;
    std::vector<InstructionPtr> headerPhi;
    std::unordered_map<ValuePtr, ValuePtr> tmpmap;

    for(auto bb : func->basicBlocks){
        for (auto instr : bb->instructions) {
            updateCurrentValuesMapping(instr, CurrentIncomingValue);
        }
    }
    

    std::vector<BasicBlockPtr> latchHeaderEdge;
    latchHeaderEdge.push_back(latch);
    std::unordered_map<ValuePtr, ValuePtr> copyMap;
    int trip = loop->iterationCount;
    auto loopCpy = copyBigLoop(loop, CurrentIncomingValue, func);
    if(loopCpy == nullptr) return false;
    func->loopList.push_back(loopCpy);
    cerr << "happy1" << endl;
    return true;
}

void partialUnrollLoop(FunctionPtr func) {
    queue<LoopPtr> workQueue;

    for (auto loop : func->loopList) {
        if (loop->subLoops.size() == 0) {
            workQueue.push(loop);
        }
    }

    while (!workQueue.empty()) {
        auto loop = workQueue.front();
        workQueue.pop();

        if(runUnrollLoop(loop, func)){
            cerr << "Successfully unroll one loop.\n";
        }
    }
}