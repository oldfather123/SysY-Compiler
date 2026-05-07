#include "funcInline.h"

static VariablePtr valCopy(VariablePtr origin){
    VariablePtr ret = Variable::copy(origin);
    ret->name = ret->name+".copy";
    ret->useHead = nullptr;
    ret->useCount = 0;
    return ret;
}

void createValMap(CallInstruction* callIns, vector<ValuePtr>&actArg, unordered_map<ValuePtr,ValuePtr>&valMap, Module &ir){
    for(auto gvar : ir.globalVariables){
        valMap[gvar] = gvar;
    }

    auto callFunc = callIns->func;
    for(auto arg : callFunc->formalArguments){
        valMap[arg] = valCopy(dynamic_pointer_cast<Variable>(arg));
        actArg.push_back(valMap[arg]);
    }
}

vector<BasicBlockPtr> createBbMapAndLabMap(CallInstruction* callIns, unordered_map<BasicBlockPtr, BasicBlockPtr>&bbMap, unordered_map<LabelPtr, LabelPtr>&labMap, int callNum){
    auto callFunc = callIns->func;

    vector<BasicBlockPtr>bbsCopy;
    for(auto bb : callFunc->basicBlocks){
        LabelPtr bbLab = LabelPtr(new Label(callFunc->name+bb->label->name+".copy_"+to_string(callNum)));
        labMap[bb->label] = bbLab;
        bbMap[bb] = BasicBlockPtr(new BasicBlock(bbLab));
        bbsCopy.push_back(bbMap[bb]);
    }

    for(auto bb : callFunc->basicBlocks){
        auto bbCopy = bbMap[bb];

        for(auto succ : bb->succBasicBlocks){
            bbCopy->succBasicBlocks.insert(bbMap[succ]);
        }
        for(auto pred : bb->predBasicBlocks){
            bbCopy->predBasicBlocks.insert(bbMap[pred]);
        }
    }

    return bbsCopy;
}

static ValuePtr valMapping(ValuePtr funcArg, unordered_map<ValuePtr, ValuePtr>&valMap){
    if(funcArg->isConst){
        return funcArg;
    }
    if(valMap.find(funcArg) == valMap.end()){
        if(!dynamic_pointer_cast<Variable>(funcArg)){
            valMap[funcArg] =  ValuePtr(new Reg(funcArg->type, funcArg->name));
            return valMap[funcArg];
        }
        valMap[funcArg] = valCopy(dynamic_pointer_cast<Variable>(funcArg));
    }
    return valMap[funcArg];
}

InstructionPtr insMapping(InstructionPtr Or, unordered_map<ValuePtr, ValuePtr>&valMap, unordered_map<LabelPtr, LabelPtr>&labMap, unordered_map<BasicBlockPtr, BasicBlockPtr>&bbMap){
    auto origin = Or.get();
    switch (origin->type){
    case Binary:{
        auto originBi = dynamic_cast<BinaryInstruction*>(origin);
        if(originBi == nullptr)
            assert(false && "wrong ins type");
        
        auto retIns = InstructionPtr(new BinaryInstruction(valMapping(originBi->a, valMap), valMapping(originBi->b, valMap), originBi->op, BasicBlockPtr(nullptr)));
        
        valMap[origin->reg] = retIns->reg;
        return retIns;
    }
    case Icmp:{
        auto originIcmp = dynamic_cast<IcmpInstruction*>(origin);
        if(originIcmp == nullptr)
            assert(false && "wrong ins type");
        
        auto retIns = InstructionPtr(new IcmpInstruction(nullptr, valMapping(originIcmp->a, valMap), valMapping(originIcmp->b, valMap), originIcmp->op));
        
        valMap[origin->reg] = retIns->reg;
        return retIns;
    }
    case Fcmp:{
        auto originFcmp = dynamic_cast<FcmpInstruction*>(origin);
        if(originFcmp == nullptr)
            assert(false && "wrong ins type");
        
        auto retIns = InstructionPtr(new FcmpInstruction(nullptr, valMapping(originFcmp->a, valMap), valMapping(originFcmp->b, valMap), originFcmp->op));
        
        valMap[origin->reg] = retIns->reg;
        return retIns;
    }
    case Fneg:{
        FnegInstruction* originFneg = dynamic_cast<FnegInstruction*>(origin);
        if(originFneg == nullptr)
            assert(false && "wrong ins type");

        auto retIns =  InstructionPtr(new FnegInstruction(valMapping(originFneg->a, valMap), nullptr));
        valMap[origin->reg] = retIns->reg;
        return retIns;
    }
    case Return:{
        auto originRi = dynamic_cast<ReturnInstruction*>(origin);
        if(originRi == nullptr)
            assert(false && "wrong ins type");

        auto retIns =  InstructionPtr(new ReturnInstruction(valMapping(originRi->retValue,valMap), nullptr));
        
        valMap[origin->reg] = retIns->reg;
        return retIns;
    }
    case Ext:{
        ExtInstruction* originExt = dynamic_cast<ExtInstruction*>(origin);

        auto retIns =  InstructionPtr(new ExtInstruction(valMapping(originExt->from, valMap), originExt->to, originExt->isSigned, nullptr));
        
        valMap[origin->reg] = retIns->reg;
        return retIns;
    }
    case Select:{
        auto originSli = dynamic_cast<SelectInstruction*>(origin);
        if(originSli == nullptr)
            assert(false && "wrong ins type");

        auto retIns =  InstructionPtr(new SelectInstruction(nullptr, valMapping(originSli->exp, valMap), valMapping(originSli->val1, valMap), valMapping(originSli->val2, valMap)));
        valMap[origin->reg] = retIns->reg;
        return retIns;
    }
    case Br:{
        auto originBr = dynamic_cast<BrInstruction*>(origin);
        if(originBr == nullptr)
            assert(false && "wrong ins type");
        if(originBr->exp == nullptr){
            auto retIns = InstructionPtr(new BrInstruction(labMap[originBr->label1], nullptr));
            valMap[origin->reg] = retIns->reg;
            return retIns;
        }else{
            auto retIns = InstructionPtr(new BrInstruction(valMapping(originBr->exp, valMap), labMap[originBr->label1], labMap[originBr->label2], nullptr));
            valMap[origin->reg] = retIns->reg;
            return retIns;
        }
    }
    case Load:{
        auto originLoad = dynamic_cast<LoadInstruction*>(origin);
        if(originLoad == nullptr)
            assert(false && "wrong ins type");
        
        auto retIns =  InstructionPtr(new LoadInstruction(valMapping(originLoad->from, valMap), valMapping(originLoad->to,valMap), nullptr));
        valMap[origin->reg] = retIns->reg;
        return retIns;
        
  
    }
    case Store:{
        auto originSt = dynamic_cast<StoreInstruction*>(origin);
        if(originSt == nullptr)
            assert(false && "wrong ins type");

        if(originSt->gep){
            GetElementPtrInstruction* newGEP = dynamic_cast<GetElementPtrInstruction*>((valMap[originSt->gep->reg])->I);
            auto retIns =  InstructionPtr(new StoreInstruction(dynamic_pointer_cast<GetElementPtrInstruction>(newGEP->getSharedThis()), valMapping(originSt->value,valMap), nullptr));

            valMap[origin->reg] = retIns->reg;
            return retIns;  
        }
        else{
            auto retIns =  InstructionPtr(new StoreInstruction(valMapping(originSt->des,valMap),valMapping(originSt->value,valMap), nullptr));

            valMap[origin->reg] = retIns->reg;
            return retIns;   
        }
    }
    case Alloca:{
        auto originAll = dynamic_cast<AllocaInstruction*>(origin);
        if(originAll == nullptr)
            assert(false && "wrong ins type");

        auto retIns =  InstructionPtr(new AllocaInstruction(valMapping(originAll->des,valMap), nullptr));
        valMap[origin->reg] = retIns->reg;
        return retIns;
    }
    case Call:{
        auto originCall = dynamic_cast<CallInstruction*>(origin);
        if(originCall == nullptr)
            assert(false && "wrong ins type");

        vector<ValuePtr> newArg;
        for(int i = 0; i < originCall->argv.size(); i++){
            newArg.push_back(valMapping(originCall->argv[i], valMap));
        }
        auto retIns =  InstructionPtr(new CallInstruction(originCall->func, newArg, nullptr));

        valMap[origin->reg] = retIns->reg;
        return retIns;
    }
    case Fptosi:{
        auto originFtS = dynamic_cast<FloatToSignInstruction*>(origin);
        if(originFtS == nullptr)
            assert(false && "wrong ins type");

        auto retIns =  InstructionPtr(new FloatToSignInstruction(valMapping(originFtS->from, valMap),nullptr));
        valMap[origin->reg] = retIns->reg;
        return retIns;
    }
    case Sitofp:{
        auto originStF = dynamic_cast<SignToFloatInstruction*>(origin);
        if(originStF == nullptr)
            assert(false && "wrong ins type");

        auto retIns =  InstructionPtr(new SignToFloatInstruction(valMapping(originStF->from, valMap), nullptr));
        valMap[origin->reg] = retIns->reg;
        return retIns;
    }
    case GEP:{
        auto originGep = dynamic_cast<GetElementPtrInstruction*>(origin);
        if(originGep == nullptr)
            assert(false && "wrong ins type");

        vector<ValuePtr> newIndex;
        for(auto oldInx : originGep->index){
            newIndex.push_back(valMapping(oldInx,valMap));
        }
        auto newFrom = valMapping(originGep->from,valMap);
       
        auto retIns = InstructionPtr(new GetElementPtrInstruction(valMapping(originGep->from,valMap),newIndex, nullptr));
         if(originGep->from->type->isPtr()){
            retIns->reg->type = dynamic_cast<PtrType*>(originGep->from->type.get())->inner;
        }
        
        valMap[origin->reg] = retIns->reg;
        return retIns;
    }
    case Phi:{
        auto originPhi = dynamic_cast<PhiInstruction*>(origin);
        if(originPhi == nullptr)
            assert(false && "wrong ins type");

        auto retIns = InstructionPtr(new PhiInstruction(nullptr, valMapping(originPhi->val, valMap))); 
        valMap[origin->reg] = retIns->reg;
        return retIns;
    }
    case Bitcast:{
        auto originBit = dynamic_cast<BitCastInstruction*>(origin);
        if(originBit == nullptr)
            assert(false && "wrong ins type");

        auto retIns =  InstructionPtr(new BitCastInstruction(valMapping(originBit->from, valMap), valMapping(originBit->reg, valMap),nullptr, originBit->targetType));
        valMap[origin->reg] = retIns->reg;
        return retIns;
    }
    
    default:
        assert(false && "wrong Inst type");
    }
}

void bbMapping(CallInstruction* callIns, unordered_map<ValuePtr, ValuePtr>&valMap, unordered_map<BasicBlockPtr,BasicBlockPtr> bbMap, unordered_map<LabelPtr,LabelPtr> labMap, int callNum){
    auto func = callIns->func;

    stack<BasicBlockPtr>worklist;
    unordered_map<BasicBlockPtr, bool>isVis;
    vector<BasicBlockPtr>bbsCopy;
    worklist.push(func->basicBlocks[0]);

    int count = 0;

    while(!worklist.empty()){
        auto bb = worklist.top();
        worklist.pop();
        if(isVis[bb]){
            continue;
        }
        isVis[bb] = true;
        count ++;
        auto bbCopy = bbMap[bb];
        vector<InstructionPtr>inssCopy;
        for(auto ins : bb->instructions){
            auto insCopy = insMapping(ins, valMap, labMap, bbMap);
            if(insCopy->reg != nullptr){
                insCopy->reg->name = func->name + '.' + insCopy->reg->name + ".copy_" + to_string(callNum); 
            }
            if(auto aI = dynamic_cast<AllocaInstruction*>(insCopy.get())){
                aI->des->name = func->name + '.' + aI->des->name + ".copy_" + to_string(callNum); 
            }

            insCopy->basicblock = bbCopy;
            inssCopy.push_back(insCopy);
        }

        bbCopy->instructions = inssCopy;
        bbCopy->setTerminator(inssCopy.back());

        for(auto succ : bb->succBasicBlocks){
            worklist.push(succ);
        }

    }

    for(auto bb : func->basicBlocks){
        for(auto ins : bb->instructions){
            if(auto sI = dynamic_cast<PhiInstruction*>(ins.get())){
                auto tI = dynamic_cast<PhiInstruction*>(valMap[sI->reg]->I);

                for(auto fromVal : sI->from){
                    tI->addFrom(valMapping(fromVal.first, valMap), bbMap[fromVal.second]);
                }
            }
        }
    }

}

bool isInlineAble(FunctionPtr func){
    bool allCalleeIsLib = true;
    for(auto callee : func->calleeMap){
        if(!callee.first->isLibraryFunction){
            allCalleeIsLib = false;
            break;
        }
    }
    bool inlineAble = (func->name != "main") && (!func->isLibraryFunction) && (allCalleeIsLib || func->calleeMap.size()==0);
    return inlineAble;
}

void inlineFunc(CallInstruction* callIns, Module& ir, int callNum){
    auto callerFunc = callIns->basicblock->parentFunction;
    auto callerBb = callIns->basicblock;
    auto callerBbs = callerFunc->basicBlocks;
    auto callFunc = callIns->func;

    unordered_map<ValuePtr, ValuePtr>valMap;
    unordered_map<LabelPtr, LabelPtr>labMap;
    unordered_map<BasicBlockPtr, BasicBlockPtr>bbMap;
    vector<ValuePtr>argCopy;

    LabelPtr labAfterCall = LabelPtr(new Label(callerFunc->name + ".ret"+ '_' + to_string(callNum)));
    BasicBlockPtr bbAfterCall = BasicBlockPtr(new BasicBlock(labAfterCall));
    bbAfterCall->parentFunction = callerFunc;

    auto itIns = callerBb->instructions.begin();
    vector<InstructionPtr>insBeforeCall;
    vector<InstructionPtr>insAfterCall;
    bool isCallVisted = false;

    for(; itIns != callerBb->instructions.end(); itIns++){
        if((*itIns).get() == callIns){
            isCallVisted = true;
            continue;
        }
        if(!isCallVisted){
            (*itIns)->basicblock = callerBb;
            insBeforeCall.push_back((*itIns));
        }else{
            (*itIns)->basicblock = bbAfterCall;
            insAfterCall.push_back((*itIns));
        }
    }
    bbAfterCall->succBasicBlocks = callerBb->succBasicBlocks;
    for(auto succ : callerBb->succBasicBlocks){
        succ->predBasicBlocks.erase(callerBb);
        succ->predBasicBlocks.insert(bbAfterCall);

        for(auto ins : succ->instructions){
            if(ins->type == Phi){
                for(auto&f : dynamic_cast<PhiInstruction*>(ins.get())->from){
                    if(f.second == callerBb){
                        f.second = bbAfterCall;
                    }
                }
            }
        }
    }
    callerBb->succBasicBlocks.clear();

    callerBb->instructions = insBeforeCall;
    bbAfterCall->instructions = insAfterCall;
    bbAfterCall->setTerminator(insAfterCall.back());


    createValMap(callIns, argCopy, valMap, ir);
    auto bbAfterInline = createBbMapAndLabMap(callIns, bbMap, labMap, callNum);

    bbMapping(callIns, valMap, bbMap, labMap, callNum);

    auto entryBbInline = bbAfterInline[0];
    auto brToFuncInline = InstructionPtr(new BrInstruction(entryBbInline->label, callerBb));
    callerBb->instructions.push_back(brToFuncInline);
    callerBb->terminator = nullptr;
    callerBb->setTerminator(callerBb->instructions.back());
    callerBb->succBasicBlocks.insert(entryBbInline);
    entryBbInline->predBasicBlocks.insert(callerBb);

    vector<InstructionPtr>retIns;
    for(auto bb : bbAfterInline){
        if(bb->succBasicBlocks.empty()){
            auto ret = bb->instructions.back();
            if(ret->type == Return){
                retIns.push_back(ret);

                bb->instructions.pop_back();
                auto retBr = InstructionPtr(new BrInstruction(bbAfterCall->label, bb));
                bb->instructions.push_back(retBr);
                bb->terminator = retBr;
            }
            else{
                assert(false&&"dead block");
            }
        }
    }

    for(auto ret : retIns){
        auto bb = ret->basicblock;
        bb->succBasicBlocks.insert(bbAfterCall);
        bbAfterCall->predBasicBlocks.insert(bb);
    }

    if(!callIns->func->returnValue->type->isVoid()){
        if(bbAfterCall->predBasicBlocks.size() > 1){
            auto newIns = InstructionPtr(new PhiInstruction(bbAfterCall, callIns->reg));
            auto retPhi = dynamic_cast<PhiInstruction*>(newIns.get());

            bbAfterCall->instructions.insert(bbAfterCall->instructions.begin(), newIns);
            replaceVarByVar(callIns->reg, retPhi->reg);
            for(auto ret : retIns){
                auto retVal = dynamic_cast<ReturnInstruction*>(retIns[0].get())->retValue;
                retPhi->addFrom(retVal, ret->basicblock);
            }
        }else{
            auto retVal = dynamic_cast<ReturnInstruction*>(retIns[0].get())->retValue;
            replaceVarByVar(callIns->reg, retVal);
        }
    }

    for(int i = 0; i < argCopy.size(); i++){
        auto formArg = argCopy[i];
        auto actArg = callIns->argv[i];

        replaceVarByVar(formArg, actArg);
    }
    vector<InstructionPtr> allocaIns;
    vector<InstructionPtr> otherIns;
    for(auto Ins : bbAfterInline[0]->instructions){
        if(Ins->type==Alloca){
            allocaIns.push_back(Ins);
        }
        else{
            otherIns.push_back(Ins);
        }
    }
    bbAfterInline[0]->instructions = otherIns;


    allocaIns.insert(allocaIns.end(),callerFunc->basicBlocks[0]->instructions.begin(), callerFunc->basicBlocks[0]->instructions.end());
    callerFunc->basicBlocks[0]->instructions = allocaIns;


    vector<BasicBlockPtr>::iterator itBb = callerBbs.begin();
    vector<BasicBlockPtr>newBbs;
    for(; itBb != callerBbs.end(); itBb++){
        if((*itBb) == callerBb){
            newBbs.push_back(*itBb);
            break;
        }
        newBbs.push_back(*itBb);
    }
    for(auto bb : bbAfterInline){
        newBbs.push_back(bb);
    }
    newBbs.push_back(bbAfterCall);
    for(itBb++; itBb != callerBbs.end(); itBb++){
        newBbs.push_back(*itBb);
    }
    callerFunc->basicBlocks = newBbs;
}



void inlineOpt(Module& ir){
    ir.recalculateCallerAndCallee();

    queue<FunctionPtr>worklist;
    unordered_map<FunctionPtr, bool>isInlined;
    for(auto func : ir.globalFunctions){
        if(isInlineAble(func)){
            worklist.push(func);
            isInlined[func] = true;
        }
    }

    while(!worklist.empty()){
        auto funcToinline = worklist.front();
        worklist.pop();    
        int labCount = 0;

        for(auto ins : funcToinline->callerInstructions){
            auto callIns = dynamic_cast<CallInstruction*>(ins.get());
            inlineFunc(callIns, ir, labCount++);
        }
        for(auto call : funcToinline->callerMap){
            auto callerFunc = call.first;
            callerFunc->calleeMap.erase(funcToinline);
        }

        for(auto it = ir.globalFunctions.begin();it!=ir.globalFunctions.end();it++){
            if((*it) == funcToinline){
                ir.globalFunctions.erase(it);
                break;
            }
        }

        for(auto func : ir.globalFunctions){
            if(isInlineAble(func) && isInlined[func] == false){
                worklist.push(func);
                isInlined[func] = true;
            }
        }
    }
}
