#ifndef IR_OPTIMIZER_H
#define IR_OPTIMIZER_H

#include "IR.h"
#include <map>
#include <vector>
#include <set>
#include <memory>

namespace MyIR
{

    class Pass
    {
    public:
        virtual ~Pass() = default;
        virtual bool run(IRUnit *unit) = 0;
    };

    class FunctionPass : public Pass
    {
    public:
        bool run(IRUnit *unit) override;
        virtual bool runOnFunction(Function *func) = 0;
    };

    class CFGAnalysis
    {
    public:
        void run(Function *func);
        const std::vector<BasicBlock *> &getPredecessors(BasicBlock *bb) const;
        const std::vector<BasicBlock *> &getSuccessors(BasicBlock *bb) const;

    private:
        std::map<BasicBlock *, std::vector<BasicBlock *>> predecessors_map_;
        std::map<BasicBlock *, std::vector<BasicBlock *>> successors_map_;
        std::vector<BasicBlock *> empty_blocks_;
    };

    class DominatorTree
    {
    public:
        void run(Function *func, const CFGAnalysis &cfg);
        bool dominates(BasicBlock *a, BasicBlock *b) const;
        BasicBlock *getIdom(BasicBlock *bb) const;
        const std::vector<BasicBlock *> &getDomChildren(BasicBlock *bb) const;
        const std::set<BasicBlock *> &getDominanceFrontier(BasicBlock *bb) const;

    private:
        void computeIdom(Function *func, const CFGAnalysis &cfg);
        void computeDF(Function *func, const CFGAnalysis &cfg);

        std::map<BasicBlock *, BasicBlock *> idom_map_;
        std::map<BasicBlock *, std::vector<BasicBlock *>> dom_children_map_;
        std::map<BasicBlock *, std::set<BasicBlock *>> dominance_frontier_map_;
        std::vector<BasicBlock *> empty_children_;
        std::set<BasicBlock *> empty_frontier_;
    };

    class Mem2Reg : public FunctionPass
    {
    public:
        bool runOnFunction(Function *func) override;

    private:
        Function *current_function_ = nullptr;
        CFGAnalysis cfg_;
        DominatorTree dom_tree_;

        std::vector<AllocaInst *> allocas_to_promote_;
        std::map<AllocaInst *, std::set<BasicBlock *>> defining_blocks_;
        std::map<AllocaInst *, std::set<BasicBlock *>> phi_locations_;
        std::map<Instruction *, AllocaInst *> phi_to_alloca_;
        std::map<AllocaInst *, std::vector<Value *>> var_stacks_;
        std::map<Value *, Value *> replacements_;
        std::map<TypeID, Constant *> zero_constants_map_;

        void rename(BasicBlock *bb);
        Value *getUndefValFor(std::shared_ptr<Type> type);
        Value *findReplacement(Value *v);
    };

    class SCCP : public FunctionPass
    {
    public:
        bool runOnFunction(Function *func) override;

    private:
        enum LatticeValueType
        {
            UNDEFINED,
            CONSTANT,
            OVERDEFINED
        };
        struct LatticeValue
        {
            LatticeValueType type = UNDEFINED;
            Constant *value = nullptr;
        };

        void visit(Value *v);
        void visitBinaryInst(BinaryInst *inst);
        void visitCmpInst(CmpInst *inst);
        void visitCastInst(CastInst *inst);
        void visitPhiNode(Instruction *phi);
        void visitLoadInst(LoadInst *inst);
        void visitBranchInst(BranchInst *inst);
        void visitReturnInst(ReturnInst *inst);

        void add_to_cfg_worklist(BasicBlock *bb);
        void add_to_ssa_worklist(Value *v);

        LatticeValue getLatticeValue(Value *v);
        void setLatticeValue(Value *v, LatticeValue lval);

        std::vector<Value *> ssa_worklist_;
        std::vector<BasicBlock *> cfg_worklist_;
        std::set<BasicBlock *> executable_blocks_;
        std::map<Value *, LatticeValue> lattice_values_;
    };

    class DeadCodeElimination : public FunctionPass
    {
    public:
        bool runOnFunction(Function *func) override;

    private:
        bool isDead(Instruction *inst);
    };

    class StrengthReduction : public FunctionPass
    {
    public:
        bool runOnFunction(Function *func) override;

    private:
        bool isPowerOfTwo(int64_t n, int &exponent);
    };

    class FunctionInlining : public Pass
    {
    public:
        FunctionInlining(int threshold = 50) : instruction_threshold_(threshold), inline_instance_counter_(0) {}
        bool run(IRUnit *unit) override;

    private:
        bool shouldInline(Function *caller, Function *callee);
        bool doInline(CallInst *call, IRUnit *unit);
        void replaceAllUsesWith(Value *old_val, Value *new_val, Function *scope);
        BasicBlock *splitBasicBlock(BasicBlock *bb, Instruction *split_point_inst);

        int instruction_threshold_;
        std::map<Function *, int> function_size_cache_;
        int inline_instance_counter_;
    };

    class IROptimizer
    {
    public:
        IROptimizer();
        void run(IRUnit *unit);

    private:
        std::vector<std::unique_ptr<Pass>> passes_;
    };

    class Loop;

    class LoopAnalysis
    {
    public:
        void run(Function *func, const CFGAnalysis &cfg, const DominatorTree &dom_tree);
        const std::vector<std::unique_ptr<Loop>> &getLoops() const { return loops_; }
        Loop *getLoopFor(BasicBlock *bb) const;

    private:
        void findLoops(Function *func, const CFGAnalysis &cfg, const DominatorTree &dom_tree);
        void createPreheaders(Function *func, CFGAnalysis &cfg);

        std::map<BasicBlock *, Loop *> block_to_loop_map_;
        std::vector<std::unique_ptr<Loop>> loops_;
    };

    class Loop
    {
    public:
        Loop(BasicBlock *header) : header_(header), preheader_(nullptr) {}

        BasicBlock *getHeader() const { return header_; }
        BasicBlock *getPreheader() const { return preheader_; }
        const std::set<BasicBlock *> &getBlocks() const { return blocks_; }
        const std::vector<BasicBlock *> &getExitBlocks() const { return exit_blocks_; }

        void addBlock(BasicBlock *bb) { blocks_.insert(bb); }
        void addExitBlock(BasicBlock *bb) { exit_blocks_.push_back(bb); }
        void setPreheader(BasicBlock *bb) { preheader_ = bb; }
        bool contains(BasicBlock *bb) const { return blocks_.count(bb); }
        bool isLoopInvariant(Value *val, const std::set<Instruction *> &invariant_instructions) const;

    private:
        BasicBlock *header_;
        BasicBlock *preheader_;
        std::set<BasicBlock *> blocks_;
        std::vector<BasicBlock *> exit_blocks_;
    };

    class LoopInvariantCodeMotion : public FunctionPass
    {
    public:
        bool runOnFunction(Function *func) override;

    private:
        bool runOnLoop(Loop *loop, Function *func);
        CFGAnalysis cfg_;
        DominatorTree dom_tree_;
        LoopAnalysis loop_analysis_;
    };

    class AllocaHoisting : public FunctionPass
    {
    public:
        bool runOnFunction(Function *func) override;
    };

} // namespace MyIR

#endif // IR_OPTIMIZER_H