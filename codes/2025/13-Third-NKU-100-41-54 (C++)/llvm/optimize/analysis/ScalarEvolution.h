#ifndef SCALAR_EVOLUTION_H
#define SCALAR_EVOLUTION_H

#include "../../include/Instruction.h"
#include "../../include/cfg.h"
#include "loop.h"
#include "../pass.h"
#include "dominator_tree.h"
#include <map>
#include <vector>
#include <iostream>

enum SCEVType {
    scConstant,
    scUnknown,
    scAddExpr,
    scMulExpr,
    scAddRecExpr,
};

class SCEV {
    SCEVType Type;

protected:
    SCEV(SCEVType T) : Type(T) {}

public:
    SCEVType getType() const { return Type; }
	virtual void printType(std::ostream &OS) const;
    virtual void print(std::ostream &OS) const = 0;
    virtual SCEV* clone() const = 0;
};

class SCEVConstant : public SCEV {
    ImmI32Operand *V;

public:
    SCEVConstant(ImmI32Operand *v) : SCEV(scConstant), V(v) {}
    ImmI32Operand *getValue() const { return V; }
    void print(std::ostream &OS) const override;

    static bool classof(const SCEV *S) { return S->getType() == scConstant; }
    SCEV* clone() const override;
};

class SCEVUnknown : public SCEV {
    Operand V;

public:
    bool isLoopInvariant;

    SCEVUnknown(Operand v) : SCEV(scUnknown), V(v), isLoopInvariant(false) {}
	SCEVUnknown(Operand v, bool isLoopInvariant) : SCEV(scUnknown), V(v), isLoopInvariant(isLoopInvariant) {}
    Operand getValue() const { return V; }
    void print(std::ostream &OS) const override;

    static bool classof(const SCEV *S) { return S->getType() == scUnknown; }
    SCEV* clone() const override;
};

class SCEVCommutativeExpr : public SCEV {
protected:
    std::vector<SCEV *> Operands;
    SCEVCommutativeExpr(SCEVType T, const std::vector<SCEV *> &Ops) : SCEV(T), Operands(Ops) {}

public:
    const std::vector<SCEV *> &getOperands() const { return Operands; }
    void print(std::ostream &OS) const override;
    SCEV* clone() const override = 0;
    size_t getNumOperands() const { return Operands.size(); }
    SCEV* getOperand(size_t idx) const { return Operands[idx]; }
};

class SCEVAddExpr : public SCEVCommutativeExpr {
public:
    SCEVAddExpr(const std::vector<SCEV *> &Ops) : SCEVCommutativeExpr(scAddExpr, Ops) {}
    static bool classof(const SCEV *S) { return S->getType() == scAddExpr; }
    SCEV* clone() const override;
};

class SCEVMulExpr : public SCEVCommutativeExpr {
public:
    SCEVMulExpr(const std::vector<SCEV *> &Ops) : SCEVCommutativeExpr(scMulExpr, Ops) {}
    static bool classof(const SCEV *S) { return S->getType() == scMulExpr; }
    SCEV* clone() const override;
};

class SCEVAddRecExpr : public SCEV {
    SCEV *Start;
    SCEV *Step;
    Loop *L;

public:
    SCEVAddRecExpr(SCEV *start, SCEV *step, Loop *l) : SCEV(scAddRecExpr), Start(start), Step(step), L(l) {}
    SCEV *getStart() const { return Start; }
    SCEV *getStep() const { return Step; }
    const Loop *getLoop() const { return L; }
    void print(std::ostream &OS) const override;

    static bool classof(const SCEV *S) { return S->getType() == scAddRecExpr; }
    SCEV* clone() const override;
};

class ScalarEvolution {
    CFG* C;
    LoopInfo *LI;
    DominatorTree *DT;
public:
    std::map<Loop *, std::map<Operand, SCEV *>> LoopToSCEVMap;
    mutable std::vector<SCEV *> Allocations;
    mutable std::map<SCEV *, SCEV *> SimplifiedSCEVCache;  // orgin -> simplified

    mutable std::map<std::pair<Operand, Loop *>, bool> invariantCache;

    void analyzeLoop(Loop *L);
    SCEV *createSCEV(Operand V, Loop *L);
    SCEV *createAddRecFromPHI(PhiInstruction *PN, Loop *L);
    SCEV *createSCEVForArithmetic(ArithmeticInstruction *I, Loop *L);

    SCEV *getSCEVUnknown(Operand V, Loop *L) const;
    SCEV *getSCEVConstant(ImmI32Operand *C) const;
    SCEV *getAddExpr(const std::vector<SCEV *> &Ops) const;
    SCEV *getMulExpr(const std::vector<SCEV *> &Ops) const;
    SCEV *getAddRecExpr(SCEV *Start, SCEV *Step, Loop *L) const;

    ScalarEvolution(CFG* C, LoopInfo *LI, DominatorTree *DT);

	Instruction getDef(Operand V) const;
	SCEV* fixLoopInvariantUnknowns(SCEV* scev, Loop* L);

    SCEV *getSCEV(Operand V, Loop *L);
    SCEV *getSimpleSCEV(Operand V, Loop *L);
    SCEV *simplify(SCEV *S) const;
    SCEV *simplifyOnce(SCEV *S) const;
    static bool SCEVStructurallyEqual(const SCEV* A, const SCEV* B);
    bool isLoopInvariant(Operand V, Loop *L) const;
    void print(std::ostream &OS) const;
    
    // 将SCEV表达式转换为LLVM IR指令
    Operand buildExpression(SCEV* expr, LLVMBlock preheader, CFG* cfg) const;
    
    // 循环参数结构体
    struct LoopParams {
        int start_val;
        int bound_val;
        int count;
        int step_val;
        bool is_simple_loop;  // 是否为简单循环（上下界都是常量或循环不变量）
    };
    
    // GEP参数结构体
    struct GepParams {
        Operand base_ptr;
        Operand offset_op;             // 计算出的总偏移量操作数
    };
    
    // 循环外提候选变量
    struct HoistingCandidate {
        Operand operand;               // 候选变量
        SCEV* scev;                    // 该变量的SCEV表达式
    };
    
    // 提取循环参数
    LoopParams extractLoopParams(Loop* L, CFG* cfg) const;
    
    // 提取GEP参数
    GepParams extractGepParams(SCEV* array_scev, Loop* L, LLVMBlock preheader, CFG* cfg) const;

    friend class SCEVPass;
};

class SCEVPass: public IRPass {
public:
    SCEVPass(LLVMIR* IR) : IRPass(IR) {}
    void Execute() override;
	void simplifyAllSCEVExpr();
	void fixAllSCEVExpr();
};

#endif // SCALAR_EVOLUTION_H 