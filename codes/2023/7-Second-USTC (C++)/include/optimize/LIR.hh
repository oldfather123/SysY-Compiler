#include "Module.hh"

#include "Pass.hh"


class LIR: public Pass {
public:
    explicit LIR(Module *m, bool print_ir=false) : Pass(m, print_ir) {}

public:
    void execute() final;

    void merge_cmp_br(BasicBlock *bb);
    void merge_fcmp_br(BasicBlock *bb);
    void split_gep(BasicBlock *bb);
    void split_srem(BasicBlock *bb);
    void load_offset(BasicBlock *bb);
    void store_offset(BasicBlock *bb);
    void cast_cmp(BasicBlock *bb);

    const std::string get_name() const override { return name; }

private:
    std::string name = "LIR";
};