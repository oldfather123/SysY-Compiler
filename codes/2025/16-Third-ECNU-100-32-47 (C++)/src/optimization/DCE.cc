#include "ADT/CFG.h"
#include "frontend.h"
#include "sym.h"
#include <unordered_map>

namespace aaac {
using namespace frontend;
namespace optimization {
class DCEContext {
public:
    DCEContext(frontend::ControlFlowGraph *cfg): cfg(cfg) {}
    void DCE();
private:
    std::unordered_map<sym::Var *, std::vector<frontend::InsnNode *>> UseChain;
    std::unordered_map<sym::Var *, BasicBlock *> DefInsn;
    ControlFlowGraph *cfg;

    void init();
};

void DCEContext::init() {
    for (auto bb : cfg->getBBList()) {
        for (auto insn : *bb) {
            for (auto def_var : insn->getDefs()) {
                this->UseChain[def_var.get()] = std::vector<frontend::InsnNode *>();
                this->UseChain[def_var.get()].reserve(2);
                this->DefInsn[def_var.get()] = bb.get();
            }
        }
    }
    for (auto bb : cfg->getBBList()) {
        for (auto insn : *bb) {
        for (auto use_var : insn->getUses()) {
            this->UseChain[use_var.get()].push_back(insn.get());
        }
        }
    }
}

void DCEContext::DCE() {
    int count = 0;
    for (auto &[def, chain] : UseChain) {
        if (chain.size() == 0) {
            auto bb = DefInsn[def];
            for (auto it = bb->begin(); it != bb->end(); it++) {
                auto insn = *it;
                if (insn->isDef(def)) {
                    bb->removeNode(it);
                    count ++;
                    break;
                }
            }
        }
    }
    std::cout << "DCE Pass : eliminate insn " << count << "\n";
}

} // namespace optimization
} // namespace aaac

void DCE(aaac::frontend::ControlFlowGraph *cfg) {
    auto context = aaac::optimization::DCEContext(cfg);
    context.DCE();
}