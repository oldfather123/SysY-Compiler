#define NumOp 12
#define Threshold 10

#include "reassociate.h"

unordered_map<ValuePtr, unsigned> valRankMap;
unordered_map<Instruction*, unsigned> insRankMap;
unordered_map<BasicBlockPtr, unsigned> bbRankMap;
map<pair<ValuePtr, ValuePtr>, int> paiMap[NumOp];

unordered_set<Instruction*> isInRedoInss;
unordered_set<Instruction*> eraseSet;

vector<Instruction* > redoInss;

bool isInsLaOrSt(InstructionPtr ins) {
    if(ins->type == Load || ins->type == Store || ins->type == Return || ins->type == Phi|| ins->type == Br || ins->type == Alloca)
        return true;

    if(auto callInst = dynamic_cast<CallInstruction*>(ins.get())){
        return !(callInst->func->isReentrant);
    }
    return false;
}

bool isSameBb(Instruction* ins1, Instruction* ins2){
    return ins1->basicblock == ins2->basicblock;
}

BinaryInstruction* isSameOp(Instruction* ins,unsigned Opcode){
    auto binIns = dynamic_cast<BinaryInstruction*> (ins);
    if(binIns && binIns->reg->hasSingleUse() && binIns->getOpcode() == Opcode){
        if(!binIns->getOperand(0)->type->isFloat()){
            return binIns;
        }
    }
    return nullptr;
}

BinaryInstruction* isSameOp(Instruction* ins,unsigned Opcode1, unsigned Opcode2){
    auto BI = dynamic_cast<BinaryInstruction*> (ins);
    if(BI&&BI->reg->hasSingleUse() && (BI->getOpcode() == Opcode1 || BI->getOpcode() == Opcode2)){
        return BI;
    }
    return nullptr;
}

// maybe wrong
void buildMap(FunctionPtr func, vector<BasicBlockPtr>& rpoBB) {
    int insRank = 0;

    for (auto arg : func->formalArguments) {
        valRankMap[arg] = ++insRank;
    }

    for (auto bb : rpoBB) {
        int bbRank = bbRankMap[bb] = ++insRank << 16;
        for (auto& ins : bb->instructions) {
            if (isInsLaOrSt(ins)) {
                insRankMap[ins.get()] = ++bbRank;
            }
            if (!ins->isAssociative || (ins->reg && ins->reg->hasSingleUse() && ins->reg->useHead->user->type == ins->type)){
                continue;
            }

            vector<ValuePtr> worklist;
            vector<ValuePtr> Ops;
            worklist.push_back(ins->getOperand(0));
            worklist.push_back(ins->getOperand(1));

            while (!worklist.empty()) {
                ValuePtr Op = worklist.back();
                worklist.pop_back();
                auto OpI = Op->I;
                auto binOpI = dynamic_cast<BinaryInstruction*>(OpI);
                auto binI = dynamic_cast<BinaryInstruction*>(ins.get());

                if (!OpI || !Op->hasSingleUse() || OpI->type != ins->type || (binOpI && binOpI->getOpcode() != binI->getOpcode())) {
                    Ops.push_back(Op);
                    continue;
                }
                worklist.push_back(OpI->getOperand(0));
                worklist.push_back(OpI->getOperand(1));

                if(Ops.size() > Threshold){
                    break;
                }
            }

            if (Ops.size() > Threshold) {
                continue;
            }

            auto binaryOp = dynamic_cast<BinaryInstruction*>(ins.get())->getOpcode();
            set<pair<ValuePtr, ValuePtr>> isVis;
            for (int i = 0; i < Ops.size() - 1; i++) {
                ValuePtr Op0 = Ops[i];
                ValuePtr Op1 = Ops[i + 1];

                if (Op1.get() < Op0.get()) {
                    std::swap(Op0, Op1);
                }

                if (isVis.insert({Op0, Op1}).second) {
                    paiMap[binaryOp].insert({{Op0, Op1}, 1});
                }
            }
        }
    }
}

static void rankWeight(int& lhs,int &rhs,unsigned Opcode){
    if (rhs == 0) {
        return;
    }else if (lhs == 0) {
        lhs = rhs;
        return;
    }else if(Opcode == Xor){
        lhs = 0;
        return;
    }else if(Opcode == Add||Opcode == FAdd){
        lhs += rhs;
        return;
    }

    unsigned bitWidth = 32;   
    unsigned cm = 1U << (bitWidth - 2);     
    unsigned limit = cm + bitWidth;

    lhs += rhs;
    while(lhs >= limit){
        lhs-=cm;
    }
}

unsigned getRank(Instruction* ins) {
    if (ins == nullptr) {
        return 0;
    }

    if (insRankMap.find(ins) != insRankMap.end()) {
        return insRankMap[ins];
    }

    unsigned rank = 0;
    unsigned maxRank = bbRankMap[ins->basicblock];

    for (unsigned i = 0, e = ins->getOperandCount(); i != e && rank != maxRank; ++i) {
        auto operand = ins->getOperand(i);
        if (operand && operand->I) {
            rank = max(rank, getRank(operand->I));
        }
    }
    return insRankMap[ins] = rank;
}

unsigned getRank(ValuePtr arg) {
    if (arg->I){
        return getRank(arg->I);
    }

    auto it = valRankMap.find(arg);
    if(it != valRankMap.end()){
        return it->second;
    }else{
        return 0;
    }
}

bool linearizeExpTree(Instruction* ins, vector<pair<ValuePtr, int>>& Ops){
    unsigned Bitwidth;
    if(ins->reg->type->isBool()){
        Bitwidth = 1;
    }else if(ins->reg->type->isPtr()){
        Bitwidth = 64;
    }else{
        Bitwidth = 32;
    }
    unsigned Opcode = dynamic_cast<BinaryInstruction*>(ins)->getOpcode();

    bool changed = false;
    vector<pair<Instruction*, int>> worklist = {{ins, 1}};
    vector<ValuePtr> leafOrder;
    unordered_map<ValuePtr, int> leaves;
    
    while(!worklist.empty()){
        pair<Instruction*, int> P = worklist.back();
        worklist.pop_back();
        ins = P.first;

        for(unsigned OpIdx = 0;OpIdx< ins->getOperandCount();OpIdx++){
            ValuePtr Op = ins->getOperand(OpIdx);
            int weight = P.second;

            if(auto BO = isSameOp(Op->I,Opcode)){
                worklist.push_back({BO,weight});
                continue;
            }

            auto it = leaves.find(Op);
            if(it == leaves.end()){
                if(!Op->hasSingleUse()){
                    leafOrder.push_back(Op);
                    leaves[Op] = weight;
                    continue;
                }
            }
            else{
                rankWeight(it->second, weight, Opcode);
                if(!Op->hasSingleUse()){
                    continue;
                }

                weight = it -> second;
                leaves.erase(it);
            }
            leafOrder.push_back(Op);
            leaves[Op] = weight;
        }
    }

    for (auto& V : leafOrder) {
        auto it = leaves.find(V);
        if(it == leaves.end()){
            continue;
        }
        int weight = it->second;
        if(weight == 0){
            continue;
        }
        it->second = 0;
        Ops.push_back({V,weight});
    }
    if(Ops.empty()){
        ValuePtr newOp;
        if(Opcode == Add||Opcode == Xor||Opcode == Shl||Opcode==AShr||Opcode == FAdd){
            newOp = Const::createZero(ins->reg->type);
        }
        else if(Opcode == Mul||Opcode == FMul){
            newOp = Const::createOne(ins->reg->type);
        }
        Ops.emplace_back(newOp,1);
    }
    return changed;
}

ValuePtr getBinOpIdentity(unsigned Opcode, TypePtr type) {
    if(Opcode == Add || Opcode == FAdd || Opcode == Xor || Opcode == Shl || Opcode == AShr){
        return Const::createZero(type);
    }else if(Opcode == Mul || Opcode == FMul){
        return Const::createOne(type);
    }
    return nullptr;
}


ValuePtr getBinOpAbsorber(unsigned Opcode, TypePtr type) {
    if(Opcode == Mul || Opcode == FMul){
        return Const::createZero(type);
    }
    return nullptr;
}

void redoExpTree(BinaryInstruction* ins, vector<valEntry>&Ops){
    vector<BinaryInstruction*> insToRewrite;
    auto Opcode = ins->getOpcode();
    auto Op = ins;
    unordered_set<ValuePtr> cannotRewrite;

    for(int i = 0, e = Ops.size(); i != e; i++){
        cannotRewrite.insert(Ops[i].Op);
    }
    BinaryInstruction* expChangeBegin = nullptr,*expChangeEnd = nullptr;
    for(int i = 0; ; i++){
        if(i+2 == Ops.size()){
            auto lhs = Ops[i].Op;
            auto rhs = Ops[i+1].Op;
            auto oldLhs = Op->getOperand(0);
            auto oldRhs = Op->getOperand(1);
            if(lhs == oldLhs && rhs == oldRhs){
                break;
            }

            if(lhs == oldRhs && rhs == oldLhs){
                Op->swapOperands();
                break;
            }
            if(lhs != oldLhs){
                auto BI = isSameOp(oldLhs->I, Opcode);
                if(BI && !cannotRewrite.count(BI->reg)){
                    insToRewrite.push_back(BI);
                }
                Op->setOperands(0, lhs);
            }
            if(rhs != oldRhs){
                auto BI = isSameOp(oldRhs->I, Opcode);
                if(BI && !cannotRewrite.count(BI->reg)){
                    insToRewrite.push_back(BI);
                }
                Op->setOperands(1, rhs);
            }
            expChangeBegin = Op;
            if(!expChangeEnd){
                expChangeEnd = Op;
            }
            break;
        }
        auto NewRHS = Ops[i].Op;
        if(NewRHS != Op->getOperand(1)){
            if(NewRHS == Op->getOperand(0)){
                Op->swapOperands();
            }
            else{
                BinaryInstruction* BI = isSameOp(Op->getOperand(1)->I, Opcode);
                if(BI && !cannotRewrite.count(BI->reg)){
                    insToRewrite.push_back(BI);
                }
                Op->setOperands(1,NewRHS);
                expChangeBegin = Op;
                if(!expChangeEnd){
                    expChangeEnd = Op;
                }
            }
        }

        auto BI = isSameOp(Op->getOperand(0)->I,Opcode);
        if(BI && !cannotRewrite.count(BI->reg)){
            Op = BI;
            continue;
        }

        BinaryInstruction* NewOp = nullptr;
        if(insToRewrite.empty()){
            auto Undef = Const::createConstant(Type::getInt(), int(0));

            auto temp = InstructionPtr(new BinaryInstruction(Undef,Undef,ins->op,ins));
            NewOp = dynamic_cast<BinaryInstruction*>(temp.get());
            insertInstructionBefore(NewOp->getSharedThis(), ins);
        }
        else{
            NewOp = insToRewrite.back();
            insToRewrite.pop_back();
        }

        Op->setOperands(0, NewOp->reg);
        expChangeBegin = Op;
        if(!expChangeEnd){
            expChangeEnd = Op;
        }
        Op = NewOp;
    }

    if(expChangeBegin){
        while(true){
            if(expChangeBegin == ins){
                break;
            }
            expChangeBegin->moveBefore(expChangeBegin->getSharedThis(), ins->getSharedThis());
            expChangeBegin = dynamic_cast<BinaryInstruction*>(expChangeBegin->reg->useHead->user);
        }
    }

    for (auto BI : insToRewrite) {
        if (isInRedoInss.insert(BI).second) {
            redoInss.push_back(BI);
        }
    }
}

ValuePtr removeFacFromExp(ValuePtr val, ValuePtr Factor){
    auto BI = isSameOp(val->I, Mul, FMul);
    if(!BI){
        return nullptr;
    }

    vector<pair<ValuePtr, int>> Trees;
    vector<valEntry> Factors;
    linearizeExpTree(BI,Trees);
    
    for (const auto& E : Trees) {
        Factors.insert(Factors.end(), E.second, valEntry(getRank(E.first), E.first));
    }

    bool hasFactor = false;
    bool needNeg = false;

    for (unsigned i = 0; i < Factors.size(); i++) {
        if(Factors[i].Op == Factor){
            hasFactor = true;
            Factors.erase(Factors.begin()+i);
            break;
        }
        if(Const * FC1 = dynamic_cast<Const*>(Factor.get())){
            if(Const* FC2 = dynamic_cast<Const*>(Factors[i].Op.get())){
                if(FC1->type == FC2->type){
                    if ((FC1->type->isInt() && FC1->intVal == -FC2->intVal) ||
                        (FC1->type->isFloat() && FC1->floatVal == FC2->floatVal)) {
                        Factors.erase(Factors.begin()+i);
                        hasFactor = true;
                        needNeg = true;
                        break;
                    }
                }
            }
        }
    }

    if(!hasFactor){
        redoExpTree(BI, Factors);
        return nullptr;
    }

    auto InsertPt = ++BI->getIterator();

    if(Factors.size() == 1){
        if(isInRedoInss.insert(BI).second)
            redoInss.push_back(BI);
        val = Factors[0].Op;
    }else{
        redoExpTree(BI,Factors);
    }

    if(needNeg){
        val = createBinaryBefore(val, nullptr, '-', *InsertPt)->reg;
    }
    return val;
}

static bool sumMulFactors(vector<valEntry>& Ops, vector<factor>& Factors){
    int sum = 0;

    for(int idx = 1, Size = Ops.size();idx < Size;++idx){
        ValuePtr Op = Ops[idx-1].Op;
        
        unsigned Count = 1;
        while (idx < Size && Ops[idx].Op == Op) {
            Count++;
            idx++;
        }
        if(Count > 1){
            sum += Count;
        }
    }

    if(sum < 4){
        return false;
    }

    sum = 0;

    for(unsigned Idx = 1; Idx<Ops.size();Idx++){
        ValuePtr Op = Ops[Idx-1].Op;
        unsigned Count = 1;
        while (Idx < Ops.size() && Ops[Idx].Op == Op) {
            Count++;
            Idx++;
        }
        if(Count == 1)
            continue;

        Count &= ~1U;

        Idx-=Count;
        sum += Count;
        Factors.push_back(factor(Op,Count));
        Ops.erase(Ops.begin()+Idx,Ops.begin()+Idx+Count);
    }


    stable_sort(Factors.begin(),Factors.end(), [](const factor &LHS, const factor &RHS){
        return LHS.Power>RHS.Power;
    });
    return true;
}

ValuePtr buildMulDAG(vector<factor>& Factors, Instruction* InsertBefore) {
    vector<ValuePtr> outerVal;
    vector<tuple<vector<factor>, Instruction*>> workStack;
    workStack.push_back(make_tuple(Factors, InsertBefore));

    while (!workStack.empty()) {
        auto [curFactors, currentInsertBefore] = workStack.back();
        workStack.pop_back();

        vector<ValuePtr> innerVal;
        int lastIdx = 0;
        int idx = lastIdx + 1;

        while (idx < curFactors.size()) {
            if (curFactors[idx].Power <= 0) {
                break;
            }

            if (curFactors[idx].Power != curFactors[lastIdx].Power) {
                lastIdx = idx++;
                continue;
            }

            vector<ValuePtr> innerVal;
            innerVal.push_back(curFactors[lastIdx].Base);
            while (idx < curFactors.size()) {
                if (curFactors[idx].Power != curFactors[lastIdx].Power) {
                    break;
                }
                innerVal.push_back(curFactors[idx++].Base);
            }

            ValuePtr temp = curFactors[lastIdx].Base = innerVal.back();
            if (temp->I) {
                redoInss.push_back(temp->I);
            }
            lastIdx = idx;
        }

        auto last = unique(curFactors.begin(), curFactors.end(), [](const factor &lhs, const factor &rhs) {
            return lhs.Power == rhs.Power;
        });
        curFactors.erase(last, curFactors.end());

        for (factor &F : curFactors) {
            if (F.Power & 1) {
                innerVal.push_back(F.Base);
            }
            F.Power >>= 1;
        }

        if (curFactors[0].Power) {
            workStack.push_back(make_tuple(curFactors, currentInsertBefore));
        }else {
            if (!innerVal.empty()) {
                outerVal.push_back(innerVal.back());
            }
        }
    }
    return !outerVal.empty() ? outerVal.back() : nullptr;
}

ValuePtr optMul(Instruction *ins,vector<valEntry>& Ops){
    if(Ops.size() < 4){
        return nullptr;
    }

    vector<factor> Factors;
    if(!sumMulFactors(Ops, Factors)){
        return nullptr;
    }

    ValuePtr V = buildMulDAG(Factors,ins);
    if(Ops.empty()){
        return V;
    }

    valEntry newEntry = valEntry(getRank(V->I), V);
    auto it = lower_bound(Ops.begin(), Ops.end(), newEntry, [](const valEntry& LHS, const valEntry& RHS) {
        return LHS.rank < RHS.rank;
    });
    Ops.insert(it, newEntry);
    return nullptr;
}


ValuePtr optAdd(Instruction* ins, vector<valEntry>& Ops){
    for (int i = 0; i < Ops.size(); ++i) {
        ValuePtr TheOp = Ops[i].Op;  
        if(i+1 <= Ops.size() && Ops[i+1].Op == TheOp){
            int numCount = 1;

            Ops.erase(Ops.begin()+i);
            while(i!=Ops.size() && Ops[i].Op == TheOp){
                Ops.erase(Ops.begin()+i);
                ++numCount;
            }
            auto constType = TheOp->type;
            ValuePtr constMul = nullptr;
            if(constType->isInt()){
                constMul = Const::createConstant(Type::getInt(), int(numCount));
            }else{
                constMul = Const::createConstant(Type::getFloat(), float(numCount));
            }

            InstructionPtr Mul = createBinaryBefore(TheOp, constMul, '*', ins->getSharedThis());
            if(isInRedoInss.insert(Mul.get()).second)
                redoInss.push_back(Mul.get());

            if(Ops.empty()){
                return Mul->reg;
            }

            Ops.insert(Ops.begin(),valEntry(getRank(Mul.get()), Mul->reg));
            --i;
        }
    }

    unsigned MaxOcc = 0;
    ValuePtr MaxoccVal = nullptr;
    unordered_map<ValuePtr, unsigned> FactorOccurrences;

    for (auto oprand : Ops) {
        BinaryInstruction * BOp = isSameOp(oprand.Op->I, Mul, FMul);
        if(!BOp){
            continue;
        }

        vector<ValuePtr> Factors;
        auto binaryIns = isSameOp(BOp->reg->I, Mul, FMul);
        if(!binaryIns){
            Factors.push_back(BOp->reg);
        }else{
            queue<ValuePtr>worklist;
            worklist.push(binaryIns->getOperand(0));
            worklist.push(binaryIns->getOperand(1));

            while(!worklist.empty()){
                auto val = worklist.front();
                worklist.pop();

                auto valIns = isSameOp(val->I, Mul, FMul);
                if(!valIns){
                    Factors.push_back(val);
                }else{
                    worklist.push(valIns->getOperand(0));
                    worklist.push(valIns->getOperand(1));
                }
            }
        }

        unordered_set<ValuePtr> dup;
        for (auto Factor : Factors) {
            if(!dup.insert(Factor).second){
                continue;
            }
            unsigned Occ = ++FactorOccurrences[Factor];
            if(Occ>MaxOcc){
                MaxOcc = Occ;
                MaxoccVal = Factor;
            }
            
            if(Const* C = dynamic_cast<Const*>(Factor.get())){
                if (C->type->isInt() && C->intVal < 0 && C->intVal != INT_MIN) {
                    Factor = Const::createConstant(Type::getInt(),-C->intVal);
                    if(!dup.insert(Factor).second){
                        continue;
                    }
                    unsigned Occ = ++FactorOccurrences[Factor];
                    if(Occ>MaxOcc){
                        MaxOcc = Occ;
                        MaxoccVal = Factor;
                    }
                }
                else if (C->type->isFloat() && C->floatVal < 0) {
                    Factor = Const::createConstant(Type::getFloat(),-C->floatVal);
                    if(!dup.insert(Factor).second){
                        continue;
                    }
                    unsigned Occ = ++FactorOccurrences[Factor];
                    if(Occ>MaxOcc){
                        MaxOcc = Occ;
                        MaxoccVal = Factor;
                    }
                }
            }
        }
    }

    if(MaxOcc>1){
        vector<ValuePtr> newMulOp;
        for(int i = 0;i!=Ops.size(); i++){
            auto BOp = isSameOp(Ops[i].Op->I, Mul,FMul);
            if(!BOp) continue;

            if(ValuePtr val = removeFacFromExp(Ops[i].Op, MaxoccVal)){
                for(int j = Ops.size(); j!=i;){
                    j--;
                    if(Ops[j].Op == Ops[i].Op){
                        newMulOp.push_back(val);
                        Ops.erase(Ops.begin()+j);
                    }
                }
                i--;
            }
        } 
        ValuePtr val = nullptr;

        if (!newMulOp.empty()) {
            stack<ValuePtr> valStack;

            for (auto & op : newMulOp) {
                valStack.push(op);
            }
            while (valStack.size() > 1) {
                ValuePtr v1 = valStack.top();
                valStack.pop();
                ValuePtr v2 = valStack.top();
                valStack.pop();

                ValuePtr result = createBinaryBefore(v2, v1, '+', ins->getSharedThis())->reg;
                valStack.push(result);
            }
            val = valStack.top();
        }

        if(auto valI = val->I){
            if(isInRedoInss.insert(valI).second)
                redoInss.push_back(valI);
        }

        auto val2 = createBinaryBefore(val, MaxoccVal, '*', ins->getSharedThis()).get();

        if(isInRedoInss.insert(val2).second)
            redoInss.push_back(val2);

        if(Ops.empty()){
            return val2->reg;
        }

        Ops.insert(Ops.begin(), valEntry(getRank(val2), val2->reg));
    }
    return nullptr;
}


ValuePtr optExp(BinaryInstruction* ins ,vector<valEntry>&Ops){
    ValuePtr mConst = nullptr;
    auto Opcode = ins->getOpcode();
    while(!Ops.empty() && Ops.back().Op->isConst){
        ValuePtr C = Ops.back().Op;
        Ops.pop_back();
        if(mConst){
           mConst = createNewConst(C, mConst, Opcode);
        }else{
            mConst = C;
        }
    }

    if(Ops.empty()){
        return mConst;
    }
    if (mConst) {
        if ((mConst->type->isInt() && dynamic_cast<Const*>(mConst.get())->intVal != dynamic_cast<Const*>(getBinOpIdentity(Opcode, ins->reg->type).get())->intVal) ||
            (mConst->type->isFloat() && dynamic_cast<Const*>(mConst.get())->floatVal != dynamic_cast<Const*>(getBinOpIdentity(Opcode, ins->reg->type).get())->floatVal)) {
            
            if (auto absorber = getBinOpAbsorber(Opcode, ins->reg->type)) {
                if (mConst == absorber) {
                    return mConst;
                }
            }
            Ops.push_back(valEntry(0, mConst));
        }
    }
    if (Ops.size() == 1) {
        return Ops[0].Op;
    }
    
    auto OpsNum = Ops.size();
    ValuePtr ret = nullptr;

    if(Opcode == FAdd || Opcode == Add){
        ret = optAdd(ins, Ops);
    }else if(Opcode == Mul || Opcode == FMul){
        ret = optMul(ins, Ops);
    }

    if (ret) {
        return ret;
    }else if (Ops.size() != OpsNum) {
        return optExp(ins, Ops);
    }else {
        return nullptr;
    }
}


void reassExp(BinaryInstruction* ins){
    vector<pair<ValuePtr, int>> tree;
    vector<valEntry> Ops;
    linearizeExpTree(ins, tree);
    
    for (auto& E : tree) {
        Ops.insert(Ops.end(), E.second, valEntry(getRank(E.first), E.first));
    }

    stable_sort(Ops.begin(),Ops.end(),[&](valEntry a ,valEntry b){return a.rank>b.rank;});

    if(auto val = optExp(ins,Ops)){
        if(dynamic_cast<BinaryInstruction*>(val.get()) == ins){
            return;
        }
        replaceVarByVar(ins->reg, val);
        if(isInRedoInss.insert(ins).second){
            redoInss.push_back(ins);
        }
        return;
    }

    if(ins->reg->hasSingleUse()){
        auto binInsReg = dynamic_cast<BinaryInstruction*>(ins->reg->useHead->user);
        if(Ops.back().Op->isConst && binInsReg && (ins->getOpcode()==Mul && binInsReg->getOpcode()==Add && Ops.back().Op == Const::createConstant(Type::getInt(),int(-1)))
        || (ins->getOpcode()==FMul && binInsReg->getOpcode()==FAdd && Ops.back().Op == Const::createConstant(Type::getFloat(),float(-1)))){
                rotate(Ops.rbegin(), Ops.rbegin() + 1, Ops.rend());
        }
    }

    if(Ops.size() == 1){
        if (Ops[0].Op->I != ins) {
            replaceVarByVar(ins->reg, Ops[0].Op);
            if (isInRedoInss.insert(ins).second)
                redoInss.push_back(ins);
        }
        return;
    }

    if(Ops.size() > 2 && Ops.size() < Threshold){
        unsigned Max = 1;
        unsigned bestRank = 0;
        unsigned Idx = ins->getOpcode();
        unsigned LimitIdx = 0;
        pair<unsigned, unsigned> bestPair;
        bool CSELocalOpt = true;

        if(CSELocalOpt){
            BasicBlockPtr FirstSeenBB = nullptr;
            for (int i = Ops.size() - 2; i >= 0; --i) {
                auto Val = Ops[i].Op;
                auto curLeafIns = Val->I;
                BasicBlockPtr seenBb = nullptr;

                if(!curLeafIns){
                    seenBb = ins->basicblock->parentFunction->basicBlocks[0];
                }else{
                    seenBb = curLeafIns->basicblock;
                }

                if(!FirstSeenBB){
                    FirstSeenBB = seenBb;
                    continue;
                }else if(FirstSeenBB != seenBb){
                    LimitIdx = i+1;
                    break;
                }
            }
        }

        for(int i = Ops.size()-1; i > LimitIdx; --i){
            for(int j = i - 1;j >= (int)LimitIdx; --j){
                unsigned sum = 0;
                auto Op0 = Ops[i].Op;
                auto Op1 = Ops[j].Op;
                if(less<ValuePtr>()(Op1,Op0)){
                    swap(Op0,Op1);
                }
                auto it = paiMap[Idx].find({Op0,Op1});
                if(it!=paiMap[Idx].end()){
                    if(Op0 && Op1){
                        sum += it->second;
                    }
                }
                auto maxRank = max(Ops[i].rank,Ops[j].rank);
                if(sum > Max ||(sum == Max && maxRank < bestRank)){
                    Max = sum;
                    bestPair = {j,i};
                    bestRank = maxRank;
                }
            }
        }
        if (Max > 1) {
            rotate(Ops.begin() + bestPair.first, Ops.begin() + bestPair.first + 1, Ops.begin() + bestPair.second + 1);
        }
    }
    redoExpTree(ins,Ops);
}

static bool needToTranToSub(BinaryInstruction* Sub){
    if (isFneg(Sub) || !Sub->reg->useHead) {
        return false;
    }

    auto op0 = Sub->getOperand(0);
    auto op2 = Sub->getOperand(1);

    if(isSameOp(op0->I,Add,FAdd) || isSameOp(op0->I,BinaryInstructionOps::Sub,FSub) || isSameOp(op2->I,Add,FAdd) || isSameOp(op2->I,BinaryInstructionOps::Sub,FSub)){
        return true;
    }
    auto regUser = Sub->reg->useHead->user->reg;
    if(Sub->reg->hasSingleUse() && regUser&& (isSameOp(regUser->I,Add,FAdd) || isSameOp(regUser->I,BinaryInstructionOps::Sub,FSub))){
        return true;
    }
    return false;
}

static ValuePtr negVal(ValuePtr val, BinaryInstruction* BI, vector<Instruction*>toRedo){
    if(auto C = dynamic_cast<Const*>(val.get())){
        return C->type->isFloat()?Const::createConstant(Type::getFloat(),float(-1.0*C->floatVal)):Const::createConstant(Type::getInt(),-1*C->intVal);
    }

    if (auto ins = isSameOp(val->I, Add, FAdd)) {
        ins->setOperands(0, negVal(ins->getOperand(0), BI, toRedo));
        ins->setOperands(1, negVal(ins->getOperand(1), BI, toRedo));
        ins->moveBefore(ins->getSharedThis(), BI->getSharedThis());
        ins->setRegisterName(ins->getRegisterName() + ".neg");
        toRedo.push_back(ins);
        return ins->reg;
    }

    auto uHead = val->useHead;
    while(uHead != nullptr){
        if(!isFneg(dynamic_cast<BinaryInstruction*>(uHead->user))){
            uHead=uHead->next;
            continue;
        }
        auto negIns = uHead->user;

        if(negIns->basicblock->parentFunction != BI->basicblock->parentFunction){
            uHead=uHead->next;
            continue;
        }

        vector<InstructionPtr>::iterator insIT;
        if(auto valI = val->I){
            if(val->I->type == Phi){
                insIT = valI->basicblock->instructions.end();
                auto insBefore = --insIT - 1;
                if(valI->basicblock->instructions.size() > 1 && ((*insBefore)->type == Icmp || (*insBefore)->type == Fcmp)){
                    insIT = insBefore;
                }
            }else{
                insIT = valI->getIterator();
                insIT++;
            }
            if(insIT == valI->basicblock->instructions.end()){
                uHead = uHead->next;
                continue;
            }
        }else{
            insIT = negIns->basicblock->parentFunction->getEntryBlock()->instructions.begin();
        }

        negIns->moveBefore(negIns->getSharedThis(), *insIT);
        toRedo.push_back(negIns);
        return negIns->reg;
    }
    auto retNeg = createBinaryBefore(val, nullptr, '-', BI->getSharedThis());
    toRedo.push_back(retNeg.get());
    return retNeg->reg;
}

void optIns(InstructionPtr ins){
    auto binIns = dynamic_cast<BinaryInstruction*>(ins.get());
    if(!binIns){
        return;
    }

    if(binIns->isCommutative){
        ValuePtr op0 = ins->getOperand(0);
        ValuePtr op2 = ins->getOperand(1);
    
        if(op0->isConst || getRank(op2->I) < getRank(op0->I)){
            binIns->swapOperands();
        }
    }

    if(binIns->getOperand(0)->type->isFloat() || binIns->getOperand(0)->type->isBool()){
        return;
    }

    if(binIns->getOpcode() == Sub){
        if(needToTranToSub(binIns)){
            ValuePtr neg = negVal(binIns->getOperand(1),binIns, redoInss);
            auto newBin = createBinaryBefore(binIns->getOperand(0), neg, '+', binIns->getSharedThis());
            
            replaceVarByVar(binIns->reg, newBin->reg);

            binIns->setOperands(0,Const::createConstant(Type::getInt(),int(0)));
            binIns->setOperands(1,Const::createConstant(Type::getInt(),int(0)));
            redoInss.push_back(binIns);
            binIns = dynamic_cast<BinaryInstruction*>(newBin.get());
        }
        else if(isFneg(binIns)){
            if(isSameOp(binIns->getOperand(1)->I, Mul) && (!isSameOp(binIns->reg->useHead->user, Mul))){
                    InstructionPtr ni = createBinaryBefore(binIns->getOperand(1), Const::createConstant(Type::getInt(),int(-1)), '*', binIns->getSharedThis());
                    // maybe wrong
                    replaceVarByVar(binIns->reg, ni->reg);
                    binIns->removeFromParent();
                    binIns = dynamic_cast<BinaryInstruction*>(ni.get());
                 }
        }
    }

    if(binIns->isAssociative) {
        auto Opcode = binIns->getOpcode();
        auto useBinIns = dynamic_cast<BinaryInstruction*>(binIns->reg->useHead->user);
        if(binIns->reg->hasSingleUse() && useBinIns && useBinIns->getOpcode() == Opcode){
            if(isSameBb(useBinIns, binIns) && isInRedoInss.insert(binIns->reg->useHead->user).second){
                redoInss.push_back(binIns->reg->useHead->user);
            }
            return;
        }
        auto binInsUser = dynamic_cast<BinaryInstruction*>(binIns->reg->useHead->user);
        if(isSameOp(binIns, Add) && binInsUser && binInsUser->getOpcode() == Sub){
            return;
        }
        reassExp(binIns);
    }
}

void removeIns(Instruction * ins){
    eraseSet.insert(ins);

    vector<ValuePtr> opErase;
    for(int i = 0; i < ins->getOperandCount(); i++){
        opErase.push_back(ins->getOperand(i));
    }
    insRankMap.erase(ins);
    for(auto it = redoInss.begin(); it!=redoInss.end(); it++){
        if((*it)==ins){
            isInRedoInss.erase(*it);
            redoInss.erase(it);
            break;
        }
    }

    // deal op
    unordered_map<Instruction*, bool>isVisited;
    for (auto& Op : opErase) {
        auto use = Op->useHead;
        while(use){
            if(use->user == ins){
                use->rmUse();
                break;
            }
            use = use->next;
        }
        if(auto BinOp = dynamic_cast<BinaryInstruction*>(Op->I)){
            auto Opcode  = BinOp->getOpcode();
            auto biTemp = dynamic_cast<BinaryInstruction*>(BinOp->reg->useHead->user);
            while(BinOp->reg->useCount == 1 && biTemp && biTemp->getOpcode() == Opcode && isVisited[BinOp] == false){
                biTemp = dynamic_cast<BinaryInstruction*>(BinOp->reg->useHead->user);
                BinOp = biTemp;
                isVisited[BinOp] = true;
            }

            if(insRankMap.count(BinOp)){
                if(isInRedoInss.insert(BinOp).second && !eraseSet.count(BinOp))
                    redoInss.push_back(BinOp);
            }
        }
    }
    ins->removeFromParent();
}

void reassociate(FunctionPtr func){
    // clear全局变量
    valRankMap.clear();
    bbRankMap.clear();
    insRankMap.clear();
    redoInss.clear();

    for(int i =0; i<NumOp; i++){
        paiMap[i].clear();
    }

    //计算函数中bb的逆后序排列
    vector<BasicBlockPtr> rpoBB;
    if(!func->basicBlocks.empty()){
        unordered_map<BasicBlockPtr, bool> vis;
        vector<BasicBlockPtr> workVec;
        stack<BasicBlockPtr> stack;
        stack.push(func->getEntryBlock());

        while (!stack.empty()) {
            auto current = stack.top();
            stack.pop();

            if (vis.find(current) == vis.end()) {
                vis[current] = true;
                workVec.push_back(current);

                for (auto &succ : current->succBasicBlocks) {
                    if (vis.find(succ) == vis.end()) {
                        stack.push(succ);
                    }
                }
            }
        }
        rpoBB.assign(workVec.begin(), workVec.end());
    }

    buildMap(func, rpoBB);
    
    for(auto& bb :rpoBB){
        for(auto it = bb->instructions.begin(); it != bb->instructions.end();){
            if(mayBeDeadCode((*it).get())){
                removeIns((*it).get());
            }else{
                auto tempI = *it;
                optIns(*it);
                it = find(bb->instructions.begin(), bb->instructions.end(),tempI);
                it++;
            }
        }
        vector<Instruction*> toRedo(redoInss);

        while(!toRedo.empty()){
            auto ins = toRedo.back();
            toRedo.pop_back();
            if(mayBeDeadCode(ins)){
                eraseSet.insert(ins);
                insRankMap.erase(ins);
                vector<ValuePtr> opVec;
                for(int i = 0; i < ins->getOperandCount(); i++){
                    opVec.push_back(ins->getOperand(i));
                }
                // erase
                auto it = find(toRedo.begin(), toRedo.end(), ins);
                if (it != toRedo.end()){
                    toRedo.erase(it);
                }
                it = find(redoInss.begin(), redoInss.end(), ins);
                if (it != redoInss.end()) {
                    redoInss.erase(it);
                }
                isInRedoInss.erase(ins);
                // deal with op
                for (auto& op : opVec) {
                    rmInsUse(ins, op);
                    if (op->I && op->hasNoUse() && !eraseSet.count(op->I)) {
                        toRedo.push_back(op->I);
                    }
                }
                ins->removeFromParent();
            }
        }
        
        while(!redoInss.empty()){
            auto ins = redoInss.front();
            isInRedoInss.erase(ins);
            redoInss.erase(redoInss.begin());
            if(!mayBeDeadCode(ins)){
                optIns(ins->getSharedThis());
            }
        }
    }
}