#pragma once
#include "../../../include/ir/ir.hpp"
#include "../../../include/pass/pass.hpp"

namespace pass {
class CFGAnalysis : public ModulePass {
public:
    void run(ir::Module* ctx, topAnalysisInfoManager* tp) override;
    void dump(std::ostream& out, ir::Module* ctx);
};
}