#include "ScalarEvolution.h"
#include <iostream>

//#define scev_debug

#ifdef scev_debug 
#define SCEV_DEBUG_PRINT(x) do { x; } while(0)
#else
#define SCEV_DEBUG_PRINT(x) do {} while(0)
#endif

void SCEVConstant::print(std::ostream &OS) const {
    OS << getValue()->GetIntImmVal();
}

void SCEVUnknown::print(std::ostream &OS) const {
	if(isLoopInvariant){
		OS << getValue() << " (I)"; // invariant
	} else {
		OS << getValue() << " (U)"; // unknown
	}
}

void SCEVCommutativeExpr::print(std::ostream &OS) const {
    OS << "(";
    for (size_t i = 0; i < getOperands().size(); ++i) {
        if (i > 0) {
            OS << (getType() == scAddExpr ? " + " : " * ");
        }
        getOperands()[i]->print(OS);
    }
    OS << ")";
}

void SCEVAddRecExpr::print(std::ostream &OS) const {
    OS << "{" ;
    getStart()->print(OS);
    OS << ",+,";
    getStep()->print(OS);
    OS << "}";
}

void SCEV::printType(std::ostream &OS) const {
	switch(Type){
		case scConstant:
			OS << "Constant ";
			break;
		case scUnknown:
			OS << "Unknown ";
			break;
		case scAddExpr:
			OS << "AddExpr ";
			break;
		case scMulExpr:
			OS << "MulExpr ";
			break;
		case scAddRecExpr:
			OS << "AddRecExpr ";
			break;
	}	
}

Instruction ScalarEvolution::getDef(Operand V) const {
    if (V->GetOperandType() != BasicOperand::REG) {
        return nullptr;
    }
    int regNo = ((RegOperand*)V)->GetRegNo();
    if (C->def_map.count(regNo)) {
        return C->def_map.at(regNo);
    }
    return nullptr;
}

ScalarEvolution::ScalarEvolution(CFG* C, LoopInfo *LI, DominatorTree *DT)
    : C(C), LI(LI), DT(DT) {
    for (auto& loop_pair : LI->getTopLevelLoops()) {
        analyzeLoop(loop_pair);
    }
}

void ScalarEvolution::analyzeLoop(Loop *L) {
    for (LLVMBlock BB : L->getBlocks()) {
        for (Instruction Inst : BB->Instruction_list) {
            if (Inst->GetResult() != nullptr) {
                getSCEV(Inst->GetResult(), L);
            }
        }
    }
    for (auto& subloop_pair : L->getSubLoops()) {
        analyzeLoop(subloop_pair);
    }
}

bool ScalarEvolution::isLoopInvariant(Operand V, Loop *L) const {
	// std::cout << "[isLoopInvariant] 分析变量: " << V << " in LoopHeader: " << L->getHeader()->block_id << std::endl;
    auto key = std::make_pair(V, L);
    auto it = invariantCache.find(key);
    if (it != invariantCache.end()) return it->second;

    if (V->GetOperandType() == BasicOperand::IMMI32 || V->GetOperandType() == BasicOperand::GLOBAL) {
        return invariantCache[key] = true;
    }
    Instruction Def = getDef(V);
    if (!Def) {
        return invariantCache[key] = true;
    }
	// 如果定义是PHI，并且定义在循环头，则是归纳变量，不是循环不变量
	// if(Def->GetOpcode() == BasicInstruction::PHI && L->getHeader()->block_id == Def->GetBlockID()){
	// 	return invariantCache[key] = false;
	// }
	LLVMBlock DefBlock = C->GetBlockWithId(Def->GetBlockID());
	if(Def->GetOpcode() == BasicInstruction::PHI && L->contains(DefBlock)){ // 嵌套循环，包含自循环头
		return invariantCache[key] = false;
	}
    if (!L->contains(DefBlock)) {
        return invariantCache[key] = true;
    }
    for (Operand op : Def->GetNonResultOperands()) {
        if (!isLoopInvariant(op, L)) {
            return invariantCache[key] = false;
        }
    }
	if(Def->GetOpcode() == BasicInstruction::CALL || Def->GetOpcode() == BasicInstruction::LOAD){
		return invariantCache[key] = false;
	}
    return invariantCache[key] = true;
}

SCEV *ScalarEvolution::getSCEV(Operand V, Loop *L) {
    SCEV_DEBUG_PRINT(std::cout << "[getSCEV] 分析变量: " << V << " in LoopHeader: " << L->getHeader()->block_id << std::endl);
    if (LoopToSCEVMap.count(L) && LoopToSCEVMap.at(L).count(V)) {
        return LoopToSCEVMap.at(L).at(V);
    }
    SCEV *S = createSCEV(V, L);
    LoopToSCEVMap[L][V] = S;
    return S;
}

SCEV *ScalarEvolution::getSimpleSCEV(Operand V, Loop *L) {
    if (LoopToSCEVMap.count(L) && LoopToSCEVMap.at(L).count(V)) {
        auto scev = LoopToSCEVMap.at(L).at(V);
		if (SimplifiedSCEVCache.count(scev)) {
			return SimplifiedSCEVCache.at(scev);
		} else {
			SCEV* simplifiedSCEV = simplify(scev);
			SimplifiedSCEVCache[scev] = simplifiedSCEV;
			return simplifiedSCEV;
		}
    }

	// 如果查表没有，则创建一个，并简化
    SCEV* createdSCEV = getSCEV(V, L);
	SCEV* simplifiedSCEV = simplify(createdSCEV);
	SimplifiedSCEVCache[createdSCEV] = simplifiedSCEV;
	return simplifiedSCEV;
}

SCEV *ScalarEvolution::createSCEV(Operand V, Loop *L) {
    if (isLoopInvariant(V, L)) {
        SCEV_DEBUG_PRINT(std::cout << "[createSCEV] " << V << " 是循环不变量" << std::endl);
        if (V->GetOperandType() == BasicOperand::IMMI32) {
            return getSCEVConstant(static_cast<ImmI32Operand*>(V));
        }
        return getSCEVUnknown(V, L);
    }

    Instruction I = getDef(V);
    if (!I) {
        SCEV_DEBUG_PRINT(std::cout << "[createSCEV] " << V << " 没有定义指令，返回Unknown" << std::endl);
        return getSCEVUnknown(V, L);
    }

    if (auto *GEP = dynamic_cast<GetElementptrInstruction*>(I)) {
        SCEV_DEBUG_PRINT(std::cout << "[createSCEV] " << V << " 的定义是GEP指令" << std::endl);

        auto Ptr = GEP->GetPtrVal();
        auto Dims = GEP->GetDims();
        auto Indexes = GEP->GetIndexes();
        
		// SCEV* BaseSCEV = getSCEV(Ptr, L);
        // SCEV* TotalOffsetSCEV = getSCEVConstant(new ImmI32Operand(0));
    
        // 检查ptr是否为循环不变量
        SCEV* BaseSCEV = nullptr;
        if (isLoopInvariant(Ptr, L)) {
            BaseSCEV = getSCEV(Ptr, L);
        } else {
            // ptr不是循环不变量，直接返回Unknown避免嵌套递推
            SCEV_DEBUG_PRINT(std::cout << "[createSCEV] ptr不是循环不变量，直接返回Unknown" << std::endl);
            BaseSCEV = getSCEVUnknown(Ptr, L);
			return BaseSCEV;
        }
        
        SCEV_DEBUG_PRINT(std::cout << "[createSCEV] BaseSCEV: "; BaseSCEV->print(std::cout); std::cout << std::endl);
        SCEV* TotalOffsetSCEV = getSCEVConstant(new ImmI32Operand(0));
    
        for (size_t i = 0; i < Indexes.size(); ++i) {
            SCEV* IndexSCEV = getSCEV(Indexes[i], L);
            if (auto C = dynamic_cast<SCEVConstant*>(IndexSCEV); C && C->getValue()->GetIntImmVal() == 0) {
                continue;
            }
            SCEV* ScaledIndex;
            if (i == 0) { // 第一个index，乘以整个数组大小
                 SCEV* SizeOfArray = getSCEVConstant(new ImmI32Operand(1));
                 for(int d : Dims) SizeOfArray = getMulExpr({SizeOfArray, getSCEVConstant(new ImmI32Operand(d))});
                 ScaledIndex = getMulExpr({IndexSCEV, SizeOfArray});
            } else { // 后续的index
                 SCEV* SizeOfSubArray = getSCEVConstant(new ImmI32Operand(1));
                 for(size_t j = i; j < Dims.size(); ++j) {
                     SizeOfSubArray = getMulExpr({SizeOfSubArray, getSCEVConstant(new ImmI32Operand(Dims[j]))});
                 }
                 ScaledIndex = getMulExpr({IndexSCEV, SizeOfSubArray});
            }

            TotalOffsetSCEV = getAddExpr({TotalOffsetSCEV, ScaledIndex});
        }

        SCEV* result = getAddExpr({BaseSCEV, TotalOffsetSCEV});
		SCEV_DEBUG_PRINT(std::cout << "[createSCEV] " << V << " 的定义是GEP指令, result="; result->print(std::cout); std::cout << std::endl);
        return result;
    }

    if (auto *PN = dynamic_cast<PhiInstruction*>(I)) {
        SCEV_DEBUG_PRINT(std::cout << "[createSCEV] " << V << " 的定义是PHI, block_id=" << PN->GetBlockID() << std::endl);
        if (L->getHeader()->block_id == PN->GetBlockID()) {
            return createAddRecFromPHI(PN, L);
        } else {
            return getSCEVUnknown(V, L);
        }
    }

    if (auto *ArithI = dynamic_cast<ArithmeticInstruction*>(I)) {
        SCEV_DEBUG_PRINT(std::cout << "[createSCEV] " << V << " 的定义是算术指令, opcode=" << ArithI->GetOpcode() << std::endl);
        return createSCEVForArithmetic(ArithI, L);
    }

    SCEV_DEBUG_PRINT(std::cout << "[createSCEV] " << V << " 的定义类型未知，返回Unknown" << std::endl);
    return getSCEVUnknown(V, L);
}

SCEV *ScalarEvolution::createAddRecFromPHI(PhiInstruction *PN, Loop *L) {
    LLVMBlock preheader = L->getPreheader();
    if (!preheader) {
        SCEV_DEBUG_PRINT(std::cout << "[createAddRecFromPHI] 没有preheader, 返回Unknown" << std::endl);
        return getSCEVUnknown(PN->GetResult(), L);
    }
    
    Operand startValue = nullptr;
    Operand stepInstOperand = nullptr;

    for (const auto& pair : PN->GetPhiList()) {
        LLVMBlock incomingBlock = C->GetBlockWithId(((LabelOperand*)pair.first)->GetLabelNo());
        SCEV_DEBUG_PRINT(std::cout << "[createAddRecFromPHI] PHI分支: label=" << ((LabelOperand*)pair.first)->GetLabelNo()
                  << " value=" << pair.second << std::endl);
        if (incomingBlock == preheader) {
            startValue = pair.second;
        } else if (L->contains(incomingBlock)) { // From a latch
            stepInstOperand = pair.second;
        }
    }

    if (!startValue || !stepInstOperand) {
        SCEV_DEBUG_PRINT(std::cout << "[createAddRecFromPHI] start/step未识别, 返回Unknown" << std::endl);
        return getSCEVUnknown(PN->GetResult(), L);
    }
    
    Instruction stepInstruction = getDef(stepInstOperand);
    auto* ArithI = dynamic_cast<ArithmeticInstruction*>(stepInstruction);

    if (!ArithI || (ArithI->GetOpcode() != BasicInstruction::ADD && ArithI->GetOpcode() != BasicInstruction::SUB)) {
        SCEV_DEBUG_PRINT(std::cout << "[createAddRecFromPHI] step不是加法或减法, 返回Unknown" << std::endl);
        return getSCEVUnknown(PN->GetResult(), L);
    }

    Operand recurrentOperand = nullptr;
    Operand stepValue = nullptr;
    bool isSubtraction = (ArithI->GetOpcode() == BasicInstruction::SUB);
    
    if (ArithI->GetOperand1() == PN->GetResult()) {
        recurrentOperand = ArithI->GetOperand1();
        stepValue = ArithI->GetOperand2();
    } else if (ArithI->GetOperand2() == PN->GetResult()) {
        recurrentOperand = ArithI->GetOperand2();
        stepValue = ArithI->GetOperand1();
    } else {
        SCEV_DEBUG_PRINT(std::cout << "[createAddRecFromPHI] step算术指令没有归纳变量, 返回Unknown" << std::endl);
        return getSCEVUnknown(PN->GetResult(), L);
    }
    
    SCEV_DEBUG_PRINT(std::cout << "[createAddRecFromPHI] start=" << startValue << " step=" << stepValue << " isSubtraction=" << isSubtraction << std::endl);
    SCEV* StartSCEV = getSCEV(startValue, L);
    SCEV* StepSCEV = getSCEV(stepValue, L);
    
    // 如果是减法，需要将step取负
    if (isSubtraction) {
        StepSCEV = getMulExpr({StepSCEV, getSCEVConstant(new ImmI32Operand(-1))});
    }

    return getAddRecExpr(StartSCEV, StepSCEV, L);
}

SCEV* ScalarEvolution::simplify(SCEV* S) const {
    SCEV* prev = S->clone();
    while (true) {
        SCEV* next = simplifyOnce(prev);
        if (SCEVStructurallyEqual(next, prev)) break;
        prev = next;
    }
    return SimplifiedSCEVCache[S] = prev;
}

SCEV *ScalarEvolution::simplifyOnce(SCEV *S) const {
    if (SimplifiedSCEVCache.count(S)) {
        return SimplifiedSCEVCache.at(S);
    }
    switch (S->getType()) {
        case scConstant:
        case scUnknown:
            return S;
        case scAddExpr: {
            auto *Add = static_cast<SCEVAddExpr*>(S);
            std::vector<SCEV *> flatOps;
            int constSum = 0;
            for (auto *Op : Add->getOperands()) {
                SCEV* simplifiedOp = simplify(Op);
                if (auto *C = dynamic_cast<SCEVConstant*>(simplifiedOp)) {
                    constSum += C->getValue()->GetIntImmVal();
                } else if (auto *NestedAdd = dynamic_cast<SCEVAddExpr*>(simplifiedOp)) {
                    for (auto *subOp : NestedAdd->getOperands()) {
                        flatOps.push_back(subOp);
                    }
                } else {
                    flatOps.push_back(simplifiedOp);
                }
            }
            if (constSum != 0) {
                flatOps.push_back(getSCEVConstant(new ImmI32Operand(constSum)));
            }
            // 合并AddRec和常量
            for (size_t i = 0; i < flatOps.size(); ++i) {
                for (size_t j = i + 1; j < flatOps.size(); ) {
                    auto *A = dynamic_cast<SCEVAddRecExpr*>(flatOps[i]);
                    auto *B = dynamic_cast<SCEVAddRecExpr*>(flatOps[j]);
                    if (A && B && A->getLoop() == B->getLoop()) {
                        SCEV *newStart = simplify(getAddExpr({A->getStart(), B->getStart()}));
                        SCEV *newStep = simplify(getAddExpr({A->getStep(), B->getStep()}));
                        flatOps[i] = getAddRecExpr(newStart, newStep, const_cast<Loop*>(A->getLoop()));
                        flatOps.erase(flatOps.begin() + j);
                    } else {
                        ++j;
                    }
                }
            }
            // AddRec + 常量 合并
            for (size_t i = 0; i < flatOps.size(); ++i) {
                if (auto *A = dynamic_cast<SCEVAddRecExpr*>(flatOps[i])) {
                    for (size_t j = 0; j < flatOps.size(); ++j) {
                        if (i == j) continue;
                        if (auto *C = dynamic_cast<SCEVConstant*>(flatOps[j])) {
                            SCEV *newStart = simplify(getAddExpr({A->getStart(), C}));
                            flatOps[i] = getAddRecExpr(newStart, A->getStep(), const_cast<Loop*>(A->getLoop()));
                            flatOps.erase(flatOps.begin() + j);
                            if (j < i) --i;
                            break;
                        }
                    }
                }
            }
            // 判断是否有变化
            if (flatOps.size() == Add->getOperands().size()) {
                bool allSame = true;
                for (size_t i = 0; i < flatOps.size(); ++i) {
                    if (flatOps[i] != Add->getOperands()[i]) {
                        allSame = false;
                        break;
                    }
                }
                if (allSame) return S;
            }
            if (flatOps.size() == 0) return getSCEVConstant(new ImmI32Operand(0));
            if (flatOps.size() == 1) return flatOps[0];
            return getAddExpr(flatOps);
        }
        case scMulExpr: {
            auto *Mul = static_cast<SCEVMulExpr*>(S);
            std::vector<SCEV *> flatOps;
            int constProd = 1;
            for (auto *Op : Mul->getOperands()) {
                SCEV* simplifiedOp = simplify(Op);
                if (auto *C = dynamic_cast<SCEVConstant*>(simplifiedOp)) {
                    constProd *= C->getValue()->GetIntImmVal();
                } else if (auto *NestedMul = dynamic_cast<SCEVMulExpr*>(simplifiedOp)) {
                    for (auto *subOp : NestedMul->getOperands()) {
                        flatOps.push_back(subOp);
                    }
                } else {
                    flatOps.push_back(simplifiedOp);
                }
            }
            if (constProd == 0) {
                return getSCEVConstant(new ImmI32Operand(0));
            }
            // 只要有且仅有一个AddRec和常量，直接分配律展开
            if (flatOps.size() == 1 && dynamic_cast<SCEVAddRecExpr*>(flatOps[0]) && constProd != 1) {
                auto *addRec = static_cast<SCEVAddRecExpr*>(flatOps[0]);
                SCEV* newStart = simplify(getMulExpr({addRec->getStart(), getSCEVConstant(new ImmI32Operand(constProd))}));
                SCEV* newStep = simplify(getMulExpr({addRec->getStep(), getSCEVConstant(new ImmI32Operand(constProd))}));
                return getAddRecExpr(newStart, newStep, const_cast<Loop*>(addRec->getLoop()));
            }
            if (constProd != 1) flatOps.push_back(getSCEVConstant(new ImmI32Operand(constProd)));
            if (flatOps.size() == 0) return getSCEVConstant(new ImmI32Operand(1));
            if (flatOps.size() == 1) return flatOps[0];
            return getMulExpr(flatOps);
        }
        case scAddRecExpr: {
            auto *AddRec = static_cast<SCEVAddRecExpr*>(S);
            SCEV *Start = simplify(AddRec->getStart());
            SCEV *Step = simplify(AddRec->getStep());
            if (auto *StepC = dynamic_cast<SCEVConstant*>(Step); StepC && StepC->getValue()->GetIntImmVal() == 0) {
                return Start;
            }
            if (Start == AddRec->getStart() && Step == AddRec->getStep()) return S;
            return getAddRecExpr(Start, Step, const_cast<Loop*>(AddRec->getLoop()));
        }
    }
    return S;
}

SCEV *ScalarEvolution::createSCEVForArithmetic(ArithmeticInstruction *I, Loop *L) {
    SCEV *Op1 = getSCEV(I->GetOperand1(), L);
    SCEV *Op2 = getSCEV(I->GetOperand2(), L);
    
    if (I->GetOpcode() == BasicInstruction::ADD) {
        return getAddExpr({Op1, Op2});
    }
    if (I->GetOpcode() == BasicInstruction::SUB) {
        // SUB: a - b == a + (-b)
        SCEV* negOp2 = getMulExpr({Op2, getSCEVConstant(new ImmI32Operand(-1))});
        return getAddExpr({Op1, negOp2});
    }
    if (I->GetOpcode() == BasicInstruction::MUL) {
        return getMulExpr({Op1, Op2});
    }
    
    return getSCEVUnknown(I->GetResult(), L);
}


SCEV *ScalarEvolution::getSCEVUnknown(Operand V, Loop *L) const {
    SCEV *S = new SCEVUnknown(V, isLoopInvariant(V, L));
    Allocations.push_back(S);
    return S;
}

SCEV *ScalarEvolution::getSCEVConstant(ImmI32Operand *C) const {
    SCEV *S = new SCEVConstant(C);
    Allocations.push_back(S);
    return S;
}

SCEV *ScalarEvolution::getAddExpr(const std::vector<SCEV *> &Ops) const {
    SCEV *S = new SCEVAddExpr(Ops);
    Allocations.push_back(S);
    return S;
}

SCEV *ScalarEvolution::getMulExpr(const std::vector<SCEV *> &Ops) const {
    SCEV *S = new SCEVMulExpr(Ops);
    Allocations.push_back(S);
    return S;
}

SCEV *ScalarEvolution::getAddRecExpr(SCEV *Start, SCEV *Step, Loop *L) const {
    SCEV *S = new SCEVAddRecExpr(Start, Step, L);
    Allocations.push_back(S);
    return S;
}

void ScalarEvolution::print(std::ostream &OS) const {
    OS << "ScalarEvolution Analysis for function " << C->function_def->GetFunctionName() << "\n";
    for (auto const& [L, ScevMap] : LoopToSCEVMap) {
        OS << "Loop at depth " << L->getLoopDepth() << " with header " << L->getHeader()->block_id << ":\n";
        for (auto const& [V, S] : ScevMap) {
            OS << "  " << V << " => ";
			OS << "Type: ";
			S->printType(OS);
			OS << " Expr: ";
            S->print(OS);
            if (SimplifiedSCEVCache.count(S)) {
                OS << " simplifiedExpr: ";
                SimplifiedSCEVCache.at(S)->print(OS);
                OS << "\n";
            } else {
                OS << "\n";
            }
        }
    }
} 

void SCEVPass::Execute() {
	for (auto [defI, cfg] : llvmIR->llvm_cfg) {
		// SCEV_DEBUG_PRINT(std::cerr << "ScalarEvolution Analysis for function " << cfg->function_def->GetFunctionName() << "\n");		
		cfg->reSetBlockID();
		ScalarEvolution* SE = new ScalarEvolution(cfg, cfg->getLoopInfo(), cfg->getDomTree());
		cfg->SCEVInfo = SE;
		simplifyAllSCEVExpr();
		// fixAllSCEVExpr();
		SE->print(std::cout);
    }
}

void SCEVPass::simplifyAllSCEVExpr() {
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        if(cfg->SCEVInfo) {
            ScalarEvolution* SE = cfg->getSCEVInfo();
            for (auto& [L, ScevMap] : SE->LoopToSCEVMap) {
                for (auto& [V, S] : ScevMap) {
                    SE->simplify(S);
                }
            }
        }
    }
}

SCEV* SCEVConstant::clone() const { return new SCEVConstant(*this); }
SCEV* SCEVUnknown::clone() const { return new SCEVUnknown(*this); }
SCEV* SCEVAddExpr::clone() const {
    std::vector<SCEV*> newOps;
    for (auto* op : Operands) {
        newOps.push_back(op->clone());
    }
    return new SCEVAddExpr(newOps);
}
SCEV* SCEVMulExpr::clone() const {
    std::vector<SCEV*> newOps;
    for (auto* op : Operands) {
        newOps.push_back(op->clone());
    }
    return new SCEVMulExpr(newOps);
}
SCEV* SCEVAddRecExpr::clone() const {
    return new SCEVAddRecExpr(Start->clone(), Step->clone(), L);
}

bool ScalarEvolution::SCEVStructurallyEqual(const SCEV* A, const SCEV* B) {
    if (A == B) return true;
    if (!A || !B) return false;
    if (A->getType() != B->getType()) return false;
    switch (A->getType()) {
        case scConstant: {
            auto* CA = static_cast<const SCEVConstant*>(A);
            auto* CB = static_cast<const SCEVConstant*>(B);
            return CA->getValue()->GetIntImmVal() == CB->getValue()->GetIntImmVal();
        }
        case scUnknown: {
            auto* UA = static_cast<const SCEVUnknown*>(A);
            auto* UB = static_cast<const SCEVUnknown*>(B);
            return UA->getValue() == UB->getValue();
        }
        case scAddExpr:
        case scMulExpr: {
            auto* EA = static_cast<const SCEVCommutativeExpr*>(A);
            auto* EB = static_cast<const SCEVCommutativeExpr*>(B);
            if (EA->getOperands().size() != EB->getOperands().size()) return false;
            for (size_t i = 0; i < EA->getOperands().size(); ++i) {
                if (!SCEVStructurallyEqual(EA->getOperands()[i], EB->getOperands()[i])) return false;
            }
            return true;
        }
        case scAddRecExpr: {
            auto* RA = static_cast<const SCEVAddRecExpr*>(A);
            auto* RB = static_cast<const SCEVAddRecExpr*>(B);
            return SCEVStructurallyEqual(RA->getStart(), RB->getStart()) &&
                   SCEVStructurallyEqual(RA->getStep(), RB->getStep()) &&
                   RA->getLoop() == RB->getLoop();
        }
    }
    return false;
}

void SCEVPass::fixAllSCEVExpr() {
	for (auto [defI, cfg] : llvmIR->llvm_cfg) {
		if(cfg->SCEVInfo) {
			ScalarEvolution* SE = cfg->getSCEVInfo();
			for (auto& [L, ScevMap] : SE->LoopToSCEVMap) {
				for (auto& [V, S] : ScevMap) {
					SCEV* simplifiedS = SE->SimplifiedSCEVCache.at(S);
					SCEV* fixedS = SE->fixLoopInvariantUnknowns(simplifiedS, L);
					fixedS = SE->simplify(fixedS);
					SE->SimplifiedSCEVCache[S] = fixedS;
				}
			}
		}
	}
}

// 递归修正：将循环内但本质循环不变量的SCEVUnknown递归还原为真实表达式
SCEV* ScalarEvolution::fixLoopInvariantUnknowns(SCEV* scev, Loop* L) {
    if (auto* unk = dynamic_cast<SCEVUnknown*>(scev)) {
        Operand V = unk->getValue();
        Instruction Def = getDef(V);
        LLVMBlock DefBlock = C->GetBlockWithId(Def->GetBlockID());
        if (Def && DefBlock && L->contains(DefBlock) && isLoopInvariant(V, L)) {
            // 如果是算术指令，递归还原为表达式
            if (auto* arith = dynamic_cast<ArithmeticInstruction*>(Def)) {
                SCEV* op1 = fixLoopInvariantUnknowns(createSCEV(arith->GetOperand1(), L), L);
                SCEV* op2 = fixLoopInvariantUnknowns(createSCEV(arith->GetOperand2(), L), L);
                if (arith->GetOpcode() == BasicInstruction::ADD) {
                    return getAddExpr({op1, op2});
                }
                if (arith->GetOpcode() == BasicInstruction::SUB) {
                    // SUB: a - b == a + (-b)
                    SCEV* negOp2 = getMulExpr({op2, getSCEVConstant(new ImmI32Operand(-1))});
                    return getAddExpr({op1, negOp2});
                }
                if (arith->GetOpcode() == BasicInstruction::MUL) {
                    return getMulExpr({op1, op2});
                }
                // 其它算术类型可按需扩展
            }
            // 其它类型递归还原
            SCEV* real = createSCEV(V, L);
            if (dynamic_cast<SCEVUnknown*>(real) == nullptr) {
                return fixLoopInvariantUnknowns(real, L);
            }
        }
        return scev;
    }
	if (auto* add = dynamic_cast<SCEVAddExpr*>(scev)) {
        std::vector<SCEV*> newOps;
        bool changed = false;
        for (auto* op : add->getOperands()) {
            SCEV* fixed = fixLoopInvariantUnknowns(op, L);
            if (fixed != op) changed = true;
            newOps.push_back(fixed);
        }
        if (!changed) return scev;
        return getAddExpr(newOps);
    }
	if (auto* mul = dynamic_cast<SCEVMulExpr*>(scev)) {
        std::vector<SCEV*> newOps;
        bool changed = false;
        for (auto* op : mul->getOperands()) {
            SCEV* fixed = fixLoopInvariantUnknowns(op, L);
            if (fixed != op) changed = true;
            newOps.push_back(fixed);
        }
        if (!changed) return scev;
        return getMulExpr(newOps);
    }
	if (auto* addrec = dynamic_cast<SCEVAddRecExpr*>(scev)) {
		SCEV* newStart = fixLoopInvariantUnknowns(addrec->getStart(), L);
        SCEV* newStep = fixLoopInvariantUnknowns(addrec->getStep(), L);
        if (newStart == addrec->getStart() && newStep == addrec->getStep()) return scev;
        return getAddRecExpr(newStart, newStep, const_cast<Loop*>(addrec->getLoop()));
	}
    return scev;
}

Operand ScalarEvolution::buildExpression(SCEV* expr, LLVMBlock preheader, CFG* cfg) const {    
    if (!expr) return nullptr;
    
    if (auto* const_val = dynamic_cast<SCEVConstant*>(expr)) {
        return const_val->getValue();
    } else if (auto* unknown = dynamic_cast<SCEVUnknown*>(expr)) {
        return unknown->getValue();
    } else if (auto* addrec = dynamic_cast<SCEVAddRecExpr*>(expr)) {
        // 处理归纳变量，提取起始值
        if (auto* start_const = dynamic_cast<SCEVConstant*>(addrec->getStart())) {
            return start_const->getValue();
        } else if (auto* start_unknown = dynamic_cast<SCEVUnknown*>(addrec->getStart())) {
            return start_unknown->getValue();
        } else if (auto* start_add = dynamic_cast<SCEVAddExpr*>(addrec->getStart())) {
            return buildExpression(start_add, preheader, cfg);
        } else if (auto* start_mul = dynamic_cast<SCEVMulExpr*>(addrec->getStart())) {
            return buildExpression(start_mul, preheader, cfg);
        } else {
            SCEV_DEBUG_PRINT(std::cerr << "SCEV buildExpression: 复杂的AddRec起始值，跳过处理" << std::endl);
            return nullptr;
        }
    } else if (auto* add_expr = dynamic_cast<SCEVAddExpr*>(expr)) {
        // 处理加法表达式
        Operand result = nullptr;
        for (SCEV* operand : add_expr->getOperands()) {
            Operand op = buildExpression(operand, preheader, cfg);
            if (op == nullptr) {
                SCEV_DEBUG_PRINT(std::cerr << "SCEV buildExpression: 加法操作数构建失败" << std::endl);
                return nullptr;
            }
            if (result == nullptr) {
                result = op;
            } else {
                // 生成加法指令
                Operand res = GetNewRegOperand(++cfg->max_reg);
                auto* addInst = new ArithmeticInstruction(BasicInstruction::LLVMIROpcode::ADD, BasicInstruction::I32, result, op, res);
                preheader->Instruction_list.push_back(addInst);
                result = res;
            }
        }
        return result;
    } else if (auto* mul_expr = dynamic_cast<SCEVMulExpr*>(expr)) {
        // 处理乘法表达式
        Operand result = nullptr;
        for (SCEV* operand : mul_expr->getOperands()) {
            Operand op = buildExpression(operand, preheader, cfg);
            if (op == nullptr) {
                SCEV_DEBUG_PRINT(std::cerr << "SCEV buildExpression: 乘法操作数构建失败" << std::endl);
                return nullptr;
            }
            if (result == nullptr) {
                result = op;
            } else {
                // 生成乘法指令
                Operand res = GetNewRegOperand(++cfg->max_reg);
                auto* mulInst = new ArithmeticInstruction(BasicInstruction::LLVMIROpcode::MUL, BasicInstruction::I32, result, op, res);
                preheader->Instruction_list.push_back(mulInst);
                result = res;
            }
        }
        return result;
    }
    
    SCEV_DEBUG_PRINT(std::cerr << "SCEV buildExpression: 未知的SCEV表达式类型，跳过处理" << std::endl);
    return nullptr;
}

ScalarEvolution::LoopParams ScalarEvolution::extractLoopParams(Loop* L, CFG* cfg) const {
    LoopParams params = {0, 0, 0, 0, false};
    
    // 从循环头部的icmp指令提取循环参数
    LLVMBlock header = L->getHeader();
    for (auto inst : header->Instruction_list) {
        if (inst->GetOpcode() == BasicInstruction::ICMP) {
            auto* icmp = dynamic_cast<IcmpInstruction*>(inst);
            if (icmp) {
                SCEV* scev1 = const_cast<ScalarEvolution*>(this)->getSimpleSCEV(icmp->GetOp1(), L);
                SCEV* scev2 = const_cast<ScalarEvolution*>(this)->getSimpleSCEV(icmp->GetOp2(), L);
                
                // 找到归纳变量和边界值
                SCEVAddRecExpr* induction_addrec = nullptr;
                SCEV* bound_scev = nullptr;
                
                if (auto* addrec = dynamic_cast<SCEVAddRecExpr*>(scev1); addrec && addrec->getLoop() == L) {
                    induction_addrec = addrec;
                    bound_scev = scev2;
                } else if (auto* addrec = dynamic_cast<SCEVAddRecExpr*>(scev2); addrec && addrec->getLoop() == L) {
                    induction_addrec = addrec;
                    bound_scev = scev1;
                }
                
                if (induction_addrec && bound_scev) {
                    // 判断是否为简单循环：检查起始值、步长值和边界值是否都是常量或循环不变量
                    bool is_start_simple = false;
                    bool is_step_simple = false;
                    bool is_bound_simple = false;
                    
                    // 检查起始值
                    if (auto* start_const = dynamic_cast<SCEVConstant*>(induction_addrec->getStart())) {
                        params.start_val = start_const->getValue()->GetIntImmVal();
                        is_start_simple = true;
                    } else if (auto* start_unknown = dynamic_cast<SCEVUnknown*>(induction_addrec->getStart())) {
                        // 检查是否为循环不变量
                        if (start_unknown->isLoopInvariant) {
                            is_start_simple = true;
                            // 对于循环不变量，暂时设置为0，实际值需要进一步分析
                            params.start_val = 0;
                        }
                    }
                    
                    // 检查步长值
                    if (auto* step_const = dynamic_cast<SCEVConstant*>(induction_addrec->getStep())) {
                        params.step_val = step_const->getValue()->GetIntImmVal();
                        is_step_simple = true;
                    } else if (auto* step_unknown = dynamic_cast<SCEVUnknown*>(induction_addrec->getStep())) {
                        // 检查是否为循环不变量
                        if (step_unknown->isLoopInvariant) {
                            is_step_simple = true;
                            // 对于循环不变量，暂时设置为0，实际值需要进一步分析
                            params.step_val = 0;
                        }
                    }
                    
                    // 检查边界值
                    if (auto* bound_const = dynamic_cast<SCEVConstant*>(bound_scev)) {
                        params.bound_val = bound_const->getValue()->GetIntImmVal();
                        is_bound_simple = true;
                    } else if (auto* bound_unknown = dynamic_cast<SCEVUnknown*>(bound_scev)) {
                        // 检查是否为循环不变量
                        if (bound_unknown->isLoopInvariant) {
                            is_bound_simple = true;
                            // 对于循环不变量，暂时设置为0，实际值需要进一步分析
                            params.bound_val = 0;
                        }
                    }
                    
                    // 判断是否为简单循环
                    params.is_simple_loop = is_start_simple && is_step_simple && is_bound_simple;
                    
                    // 计算迭代次数（仅当所有值都是常量时）
                    if (params.is_simple_loop && params.step_val != 0) {
                        int diff = params.bound_val - params.start_val;
                        params.count = diff / params.step_val;
                        
                        // 根据比较条件调整
                        auto cmp_op = icmp->GetCond();
                        if (cmp_op == BasicInstruction::IcmpCond::sle || cmp_op == BasicInstruction::IcmpCond::ule) {
                            params.count += 1;
                        }
                    }
                    
                    break;
                }
            }
        }
    }
    
    return params;
}

ScalarEvolution::GepParams ScalarEvolution::extractGepParams(SCEV* array_scev, Loop* L, LLVMBlock preheader, CFG* cfg) const {
    Operand base_ptr = nullptr;
    Operand offset_op = nullptr;
    
    // 处理加法表达式：base + index1*stride1 + index2*stride2 + ... + induction_var
	auto br_inst = preheader->Instruction_list.back();
	preheader->Instruction_list.pop_back();
    if (auto* add_expr = dynamic_cast<SCEVAddExpr*>(array_scev)) {
        for (SCEV* operand : add_expr->getOperands()) {
            if (auto* unknown = dynamic_cast<SCEVUnknown*>(operand)) {
                Operand val = unknown->getValue();
                // 仅当是指针类型时作为base_ptr；否则视为偏移量的一部分
                if (val && (val->GetOperandType() == BasicOperand::REG || val->GetOperandType() == BasicOperand::GLOBAL) && base_ptr == nullptr) {
                    base_ptr = val;
                    continue;
                }
                // 非指针unknown归入offset
                Operand operand_op = buildExpression(operand, preheader, cfg);
                if (operand_op == nullptr) {
                    continue;
                }
                if (offset_op == nullptr) {
                    offset_op = operand_op;
                } else {
                    Operand res = GetNewRegOperand(++cfg->max_reg);
                    auto* addInst = new ArithmeticInstruction(BasicInstruction::LLVMIROpcode::ADD, BasicInstruction::I32, offset_op, operand_op, res);
                    preheader->Instruction_list.push_back(addInst);
                    offset_op = res;
                }
            } else {
                // 常量、AddRec、Mul 等均加入offset表达式（AddRec将取其start，表示起始地址）
                Operand operand_op = buildExpression(operand, preheader, cfg);
                if (operand_op == nullptr) {
                    continue;
                }
                if (offset_op == nullptr) {
                    offset_op = operand_op;
                } else {
                    Operand res = GetNewRegOperand(++cfg->max_reg);
                    auto* addInst = new ArithmeticInstruction(BasicInstruction::LLVMIROpcode::ADD, BasicInstruction::I32, offset_op, operand_op, res);
                    preheader->Instruction_list.push_back(addInst);
                    offset_op = res;
                }
            }
        }
    }
    preheader->Instruction_list.push_back(br_inst);
    return {base_ptr, offset_op};
}