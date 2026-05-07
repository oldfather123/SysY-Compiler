#include "dataflowanalysis.h"

namespace IR
{
    void DataFlowAnalysis::runOnFunction(Function *func)
    {
        if (func->isBuiltinFunction())
            return;
        gen.clear(), kill.clear(), in.clear(), out.clear();
        calculateInAndOut(func);
    }

    Value *isStore(Instruction *inst)
    {
        if (inst->getOpcode() == Instruction::Store)
        {
            StoreInstruction *store = static_cast<StoreInstruction *>(inst);
            Value *dst = store->getDest();
            if (dst->isInstruction() &&
                static_cast<Instruction *>(dst)->getOpcode() == Instruction::Alloca)
                return dst;
        }
        return nullptr;
    }

    void DataFlowAnalysis::calculateInAndOut(Function *func)
    {
        //     in.clear(), out.clear();
        //     bool change = true;
        //     auto cfg = func->cfg();

        //     std::map<BasicBlock *, std::set<BasicBlock *>> prev;
        //     for (auto &[v, G] : cfg)
        //     {
        //         for (auto &u : G)
        //             prev[u].insert(v);
        //     }

        //     while (change)
        //     {
        //         change = false;

        //         for (ListNode* i = func->blocks().begin(); i != func->blocks().end(); i = i->nextNode()) {
        //             BasicBlock *u = static_cast<BasicBlock *>(i);
        //             for (auto &pred : prev[u])
        //             {
        //                 std::map<Value *, std::set<Value *>> newIn, newOut;
        //                 for (auto &[var, def] : out[pred])
        //                     newIn[var].insert(def.begin(), def.end());
        //                 newOut = newIn;
        //                 for (auto& inst: u->getInstruction()) {
        //                     auto dst = isStore(inst);
        //                     if (dst) {
        //                         newOut[dst].clear();
        //                         newOut[dst].insert(inst);
        //                     }
        //                 }
        //                 if (newIn != in[u] || newOut != out[u]) {
        //                     change = true;
        //                     in[u] = newIn;
        //                     out[u] = newOut;
        //                 }
        //             }
        //         }
        //     }
    }
}