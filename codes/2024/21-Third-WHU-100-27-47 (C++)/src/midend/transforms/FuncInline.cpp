#include "FuncInline.h"

using namespace ir;
using namespace std;

bool isSelfCall(ir::InstPtr inst, ir::FuncPtr func) {
    if (auto callinst = dynamic_pointer_cast<ir::CallInst>(inst)) {
        // 调用本身即为递归
        return func == callinst->function();
    }
    return false;
}

bool isRecursive(ir::FuncPtr func) {
    auto allBlocks = func->dfsBBList();
    for (auto bb : allBlocks) {

        for (auto inst : bb->getInstSet()) {
            if (isSelfCall(inst, func)) {
                return true; // 指令中调用了当前函数自身，即递归
            }
        }
    }

    return false; // 没有找到递归调用
}

bool useInline(ir::FuncPtr func, bool isTailCall) {
    if (func->isPrototype()) {
        return false;
    }

    if (isRecursive(func)) {
        // 递归不做处理
        return false;
    }

    if (!isTailCall) {
        int blockNum = func->bbSet().size() - 2;
        return blockNum <= 6;
    }

    return true;
}

Set<BBPtr> cloneIntoCaller(const FuncPtr &func, const FuncPtr &caller, BBPtr &entryBB, BBPtr &exitBB) {
    Set<BBPtr> newBBs{};

    // a set of all basicblock in original function
    // which is used to decide if an edge would be copied
    Set<BBPtr> bbSet = func->bbSet();
    Map<BBPtr, BBPtr> cloneBB;
    Set<Ptr<CFGEdge>> visitedEdges;

    for (auto bb : func->bbSet()) {
        auto newBB = BasicBlock::clone(caller, bb, "inlined." + func->name(), "");
        cloneBB.insert({bb, newBB});
        newBBs.insert(newBB);
        if (bb == func->entryBB()) {
            entryBB = newBB;
        }
        if (bb == func->exitBB()) {
            exitBB = newBB;
        }
    }

    for (auto bb : func->bbSet()) {
        for (auto outEdge : bb->outEdges()) {
            if (visitedEdges.find(outEdge) != visitedEdges.end()) {
                continue;
            }
            if (bbSet.find(outEdge->src()) != bbSet.end() && bbSet.find(outEdge->dest()) != bbSet.end()) {
                auto newEdge = BasicBlock::duplicateEdge(outEdge);
                newEdge->redirectSrc(cloneBB[outEdge->src()]);
                newEdge->redirectDest(cloneBB[outEdge->dest()]);
            }
        }
    }

    return newBBs;
}

void handleInline(ir::FuncPtr caller, Ptr<ir::CallInst> callInst) {
    auto callBB = callInst->parentBB();

    // 拆分 callBB
    BasicBlock::split(callBB, callInst);

    // 被调用函数及入口
    auto callee = callInst->function();
    auto calleeEntry = callee->entryBB();
    std::ofstream out("inline.log");
    out << callee->toString() << std::endl;

    // 映射关系
    Map<ir::BBPtr, ir::BBPtr> bbInlineMap;     // bb -> inlined bb
    Map<ir::RegPtr, ir::Value> paramArgMap;    // param -> arg
    Map<ir::RegPtr, ir::RegPtr> varReplaceMap; // reg -> newReg

    // Arguments
    auto args = callInst->argList();   // real args
    auto params = callee->paramList(); // formal params
    Set<ir::RegPtr> argsFSet{params.begin(), params.end()};
    if (params.size() != args.size()) {
        return;
    }

    for (int i = 0; i < params.size(); ++i) {
        paramArgMap[params[i]] = args[i];
    }

    BBPtr inlinedEntryBB = nullptr;
    BBPtr inlinedExitBB = nullptr;
    auto inlinedBBs = cloneIntoCaller(callee, caller, inlinedEntryBB, inlinedExitBB);

    // 更新指令操作数，相当于代入实参
    for (auto inlinedBB : inlinedBBs) {
        auto insts = inlinedBB->getInstSet();
        for (auto inst : insts) {

            // Replace temporary registers and formal parameter references with new version
            Vector<ir::RegPtrRef> allRefs = {};
            auto useRefs = inst->regsReadRefs();
            auto defRefs = inst->regsWrittenRefs();
            allRefs.insert(allRefs.end(), useRefs.begin(), useRefs.end());
            allRefs.insert(allRefs.end(), defRefs.begin(), defRefs.end());
            for (auto regRef : allRefs) {
                auto reg = regRef.get();
                if (reg->isGlobal()) {
                    continue;
                }
                if (varReplaceMap.find(reg) == varReplaceMap.end()) {
                    auto newReg = ir::Register::create(caller, reg->dataType(), /* "inlined." + callee->name() + "." + */ reg->name());
                    // Insert the mapping from reg to newReg for replacing reg afterwards
                    varReplaceMap.insert({reg, newReg});
                    if (paramArgMap.find(reg) != paramArgMap.end()) {
                        // reg is a formal parameter, so add a move instruction
                        ir::Instruction::insertAfter(inlinedBB->entryInst(), ir::MoveInst::create(inlinedBB, newReg, paramArgMap.at(reg)));
                    }
                }
                if (varReplaceMap.find(reg) != varReplaceMap.end()) {
                    // Replace reg with its new version
                    auto phiInst = castPtr<PhiInst>(inst);
                    inst->replaceReg(regRef, varReplaceMap.at(reg));
                }
            }
            inst->recompute();

            // Handle return
            if (auto retInst = inst->as<RetInst>()) {
                if (!callInst->destReg()->isDiscard() && callInst->function()->retDataType() != PrimaryDataType::Void) {
                    ir::Instruction::replace(retInst, ir::MoveInst::create(inlinedBB, callInst->destReg(), retInst->retVal()));
                } else {
                    ir::Instruction::remove(retInst);
                }
            }
        }
    }

    // Connecting inlined function to the original block which is divided into two blocks
    auto outEdges = callBB->outEdges();
    for (auto outEdge : outEdges) {
        outEdge->redirectSrc(inlinedExitBB);

        outEdge->brCondition() = outEdge->brCondition();
        outEdge->tag() = outEdge->tag();
    }
    BasicBlock::addEdge(callBB, inlinedEntryBB)->setUncondBr();

    dbgout << "├── Inlined function call: " << callInst->toString() << std::endl;

    // Remove the call instruction
    Instruction::remove(callInst);
}

bool ir::funcInline(ir::FuncPtr func, unsigned maxIter) {
    bool changed = false;

    dbgout << std::endl
           << "FuncInline pass started (" << func->name() << ")." << std::endl;
    FixedPoint{
        [&](bool &_changed) {
            for (auto bb : func->bbSet()) {
                for (auto inst : bb->getInstSet()) {
                    if (auto callInst = dynamic_pointer_cast<ir::CallInst>(inst)) {
                        if (!useInline(callInst->function(), callInst->isTailCall())) {
                            continue;
                        }
                        handleInline(func, callInst);
                        _changed = true;
                    }
                }
            }
            changed |= _changed;
        },
        true,
        "Function Inlining",
        func->name(),
        maxIter,
    };

    dbgout << "└── FuncInline pass done." << std::endl;
    return changed;
}

/*
int func(int a, int b){
    a = 1;
    b = 2;
    t = 0;
    %1 = a + b;
    return 123;
}

void main(){
    // int func(int a, int b){
        a' = 0;
        b' = 1;
        a' = 1 + b';
        b' = 2;
        return a+b;
    // }
    int m = func(0,1);
}
*/
