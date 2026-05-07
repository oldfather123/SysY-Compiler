#pragma once

#include "Function.hpp"
#include "Instruction.hpp"
#include "PassManager.hpp"
#include "Value.hpp"

#include <queue>

class LatticeCell {
  private:
    enum { UNKNOWN = -1, CONST, UNDEF } kind_;
    Constant *val_;

  public:
    LatticeCell() : kind_(UNDEF), val_(nullptr) {}
    LatticeCell(Constant *c) : kind_(CONST), val_(c) {}

    bool operator==(const LatticeCell &rhs) {
        if (kind_ != rhs.kind_)
            return false;

        if (kind_ != CONST)
            return true;

        if (auto i1 = dynamic_cast<ConstantInt *>(val_)) {
            if (auto i2 = dynamic_cast<ConstantInt *>(rhs.val_)) {
                return i1->get_value() == i2->get_value();
            }
        } else if (auto f1 = dynamic_cast<ConstantFP *>(val_)) {
            if (auto f2 = dynamic_cast<ConstantFP *>(rhs.val_)) {
                return f1->get_value() == f2->get_value();
            }
        }

        return false;
    }

    bool operator!=(const LatticeCell &rhs) { return !(*this == rhs); }

    void merge(const LatticeCell &rhs) {
        if (kind_ < rhs.kind_)
            return;

        if (kind_ > rhs.kind_)
            *this = rhs;

        if (*this != rhs)
            mark_unknown();
    }

    bool is_undef() const { return kind_ == UNDEF; }

    bool is_const() const { return kind_ == CONST; }

    bool is_unknown() const { return kind_ == UNKNOWN; }

    void mark_undef() { kind_ = UNDEF; }

    void mark_const(Constant *val) {
        kind_ = CONST;
        val_ = val;
    }

    void mark_unknown() { kind_ = UNKNOWN; }

    Constant *get_const() const {
        assert(is_const());
        return val_;
    }
};

class ConstProp : public Pass<Function> {
  private:
    void runSCCP(Function *f);

    LatticeCell get_lattice_cell(Value *v);
    void update(Value *val, LatticeCell cell);

    void visit_inst(Instruction *inst);
    void visit_phi(PhiInst *phi);
    void visit_br(BranchInst *br);

    Constant *evaluate(Instruction *inst, Constant *l, Constant *r = nullptr);

    bool is_executable(BasicBlock *bb);
    bool is_executable(BasicBlock *from, BasicBlock *to);

    std::map<Value *, LatticeCell> lattice_map_;
    std::set<std::pair<BasicBlock *, BasicBlock *>> is_executable_;

    std::queue<std::pair<BasicBlock *, BasicBlock *>> flow_worklist_;
    std::queue<Instruction *> ssa_worklist_;

  public:
    ~ConstProp() = default;

    void run(Function *f, AnalysisPassManager &APM) override;
};
