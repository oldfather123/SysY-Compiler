#include "Block.h"
#include "IRbuilder.h"
#include "Instruction.h"
#include "Label.h"
#include "Type.h"
#include "Value.h"
#include "loopUtils.h"
#include "patternMatch.h"
#include "utils.h"
#include <CFGAnalysis.h>
#include <Function.h>
#include <Module.h>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <domTreeAnalysis.h>
#include <loopParallelUtil.h>
#include <memory>
#include <string>

static bool isNoSideEffectExpr(Instruction* inst) {
    if(!inst->canbeOperand())
        return false;
    if(inst->isTerminator())
        return false;
    switch(inst->insId) {
        case InsID::Store:
        case InsID::AtomicAdd: {
            return false;
        }
        case InsID::Call: {
            // const auto callee = inst->lastOperand();
            auto callee = inst->getOperand(inst->getNumOperands() - 1);
            if(auto func = dynamic_cast<Function*>(callee)) {
                // auto& attr = func->attr();
                // return attr.hasAttr(FunctionAttribute::NoSideEffect) && attr.hasAttr(FunctionAttribute::Stateless) &&
                //     !attr.hasAttr(FunctionAttribute::NoReturn);
                return func->noSideEffect && func->stateless && !func->noReturn;
            }
            return false;
        }
        default:
            break;
    }

    return true;
}

//
static bool matchAddRec(Value* giv, BasicBlock* latch, std::unordered_set<Value*>& values) {

    // auto givInst = dynamic_cast<PhiInstruction*>(giv);
    if(!giv->is<PhiInstruction>())
        return false;
    /// if(!givInst->incoming.count(latch))
    ///     return false;
    if(!giv->as<PhiInstruction>()->incomings().count(latch))
        return false;

    const auto givNext = giv->as<PhiInstruction>()->incomings().at(latch)->val;
    if(givNext->isInstruction()) {
        Value* v2;
        // TODO: to deal with phi
        if(givNext->is<PhiInstruction>()) {

            for(auto& [pred, val] : givNext->as<PhiInstruction>()->incomings()) {
                if(val->val == giv) {
                    continue;
                }
                PhiInstruction* base;
                if(!add(phi(base), any(v2))(MatchContext<Value>{ val->val })
                   // && !fadd(phi(base), any(v2))(MatchContext<Value>{ val->value })
                )
                    return false;
                if(!matchAddRec(base, pred, values))
                    return false;
                values.insert(val->val);
                values.insert(base);
            }
            values.insert(giv);
            values.insert(givNext);
            return true;
        }
        if(add(exactly(giv), any(v2))(MatchContext<Value>{ givNext })
           // || fadd(exactly(giv), any(v2))(MatchContext<Value>{ givNext })
        ) {
            values.insert(giv);
            values.insert(givNext);
            return true;
        }
    }

    return false;
}

bool extractLoopBody(FunctionPtr func, Loop* loop, const DominateAnalysisResult& dom, const CFGAnalysisResult& cfg, Module& mod,
                     bool independent, bool allowInnermost, bool allowInnerLoop, bool onlyAddRec, bool estimateBlockSizeForUnroll,
                     bool needSubLoop, bool convertReduceToAtomic, bool duplicateCmp, LoopBodyInfo* ret) {

    std::cout << YELLOW "to extract loop body" RESET << endl;
    if(!allowInnermost && loop->header == loop->latch)
        return false;

    uint32_t phiCount = 0;
    for(auto& inst : loop->header->instructionsRef())
        if(inst->insId == InsID::Phi)
            ++phiCount;
        else
            break;
    // at most 1 BIV + 1 GIV
    // FIXME: support more GIV
    if(phiCount > 2)
        return false;
    std::cout << RED "phiCount: " << phiCount << RESET << endl;

    std::unordered_set<BasicBlock*> body;
    if(!collectLoopBody(loop->header, loop->latch, dom, cfg, body, allowInnerLoop, needSubLoop))
        return false;
    std::cout << GREEN "collect loop body success" RESET << endl;

    // latch -> exit
    for(auto block : body) {
        if(block == loop->latch)
            continue;
        for(auto succ : cfg.successors(block)) {
            if(!body.count(succ))
                return false;
        }
    }

    // FIXME: 似乎 cmmc 只传false
    // if(estimateBlockSizeForUnroll) {
    //    auto& heuristics = mod.getTarget().getOptHeuristic();
    //    uint32_t sum = 0;
    //    bool hasFuncCall = false;
    //    for(auto block : body) {
    //        sum += estimateBlockSize(block, !loop.initial->isConstant() || !loop.bound->isConstant());
    //        hasFuncCall |= hasCall(*block, false);
    //    }
    //    if(hasFuncCall || sum * heuristics.unrollBlockSize > heuristics.maxUnrollBodySize)
    //        return false;
    //}

    // new_loop:
    //   biv/giv
    //   loop_body: void loop_body(i, inv...)/giv = loop_body(i, giv, inv...)
    //   next
    //   cmp
    //   br cmp, new_loop, exit

    Value* giv = nullptr;
    // can't not use by outer
    bool givUsedByOuter = false;
    for(auto& inst : loop->header->instructionsRef())
        if(inst->insId == InsID::Phi) {
            // auto phiInst = dynamic_cast<PhiInstruction*>(inst.get());
            // if(phiInst != loop->indPhi) {
            //     giv = phiInst;
            //     auto givInst = dynamic_cast<PhiInstruction*>(giv);
            //     std::cout << RED << "giv: " << RESET;
            //     givInst->print();
            // }
            if(inst.get() != loop->inductionVar)
                giv = inst.get();
        } else
            break;
    if(giv) {
        // auto phiGiv = dynamic_cast<PhiInstruction*>(giv);
        for(auto inst : { giv, giv->as<PhiInstruction>()->incomings().at(loop->latch)->val }) {
            if(!inst->isInstruction())
                continue;
            for(auto user : inst->as<Instruction>()->users()) {
                if(!body.count(user->getBasicBlock())) {
                    givUsedByOuter = true;
                    break;
                }
            }
            if(givUsedByOuter)
                break;
        }
    }
    std::cout << GREEN "giv not be used in outer" RESET << endl;
    bool givUsedByInner = false;
    Value* givAddRecInnerStep = nullptr;
    // FIXME: 需要把ir 中 的 sub 改为 add
    if(giv && onlyAddRec) {
        // std::cerr << "matching\n";
        std::unordered_set<Value*> values;
        if(!matchAddRec(giv, loop->latch, values))
            return false;
        std::cout << GREEN "match add rec success" RESET << endl;

        for(auto inst : values) {
            auto ii = dynamic_cast<Instruction*>(inst);
            for(auto user : ii->users()) {
                if(!values.count(user) && body.count(user->getBasicBlock())) {
                    givUsedByInner = true;
                    std::cout << GREEN "giv used innner" RESET << endl;
                    break;
                }
            }
        }
        if(givUsedByInner) {
            if(givUsedByOuter)
                return false;

            // The initial value of giv should be inferred from the indvar
            // auto phiGiv = dynamic_cast<PhiInstruction*>(giv);
            const auto givNext = giv->as<PhiInstruction>()->incomings().at(loop->latch)->val;
            Value* v2;
            if(add(exactly(giv), any(v2))(MatchContext<Value>{ givNext })) {
                // scev addrec
                // FIXME: v2 basicblock
                // not constant
                if(v2->getBasicBlock() &&
                   (v2->getBasicBlock() == loop->header || !dom.dominate(v2->getBasicBlock(), loop->header)))
                    return false;
                std::cout << GREEN "giv add rec inner step is " RESET << endl;
                givAddRecInnerStep = v2;

            } else
                return false;
        }
        std::cout << GREEN "matched" RESET << endl;
    }
    std::unordered_set<Value*> allowedToBeUsedByOuter;
    allowedToBeUsedByOuter.insert(loop->inductionVar);
    allowedToBeUsedByOuter.insert(loop->next);
    // print indphi and next
    std::cout << RED "indPhi: " RESET;
    // dump
    std::cout << RED "next: " RESET;

    // TODO: ???????? giv 不是不能被out 使用吗？
    if(giv) {

        allowedToBeUsedByOuter.insert(giv);
        allowedToBeUsedByOuter.insert(giv->as<PhiInstruction>()->incomings().at(loop->latch)->val);
    }
    // print allowedtobeusedinouter
    std::cout << GREEN "allowedToBeUsedByOuter: " RESET << endl;
    // for(auto val : allowedToBeUsedByOuter) {
    //     val->I->print();
    // }

    for(auto block : body) {
        for(auto& inst : block->instructionsRef()) {
            // FIXME:
            if(allowedToBeUsedByOuter.count(inst.get()))
                continue;
            for(auto user : inst->users()) {
                if(body.count(user->getBasicBlock()))
                    continue;
                std::cout << RED "exist inst used by outter, by not permitted" RESET << endl;
                return false;
            }
        }
    }

    // FIXME:
    auto lookupPtrBase = [&](auto&& self, Value* ptr) -> Value* {
        if(ptr->is<Variable>()) {
            return ptr;
        }
        if(ptr->isInstruction()) {
            if(ptr->is<GetElementPtrInstruction>()) {
                return self(self, ptr->as<GetElementPtrInstruction>()->getBase());
            }
            return ptr;
        }
        return ptr;
    };

    std::cout << RED "to deal with load and store" RESET << endl;
    if(independent) {
        std::unordered_map<Value*, uint32_t> loadStoreMap;
        for(auto b : body)
            for(auto& inst : b->instructionsRef()) {
                if(inst->isTerminator())
                    continue;
                if(inst->insId == InsID::Load || inst->insId == InsID::Store) {
                    // TODO:  获取 指针 基地址 ？？？
                    const auto ptr = inst->getOperand(0);
                    const auto base = lookupPtrBase(lookupPtrBase, ptr);

                    if(!base) {
                        return false;
                    }

                    loadStoreMap[base] |= (inst->insId == InsID::Load ? 1 : 2);
                } else if(!isNoSideEffectExpr(inst.get())) {
                    return false;
                }
            }
        // TODO: 处理 load store 的情况
        // std::vector<std::pair<Instruction*, Instruction*>> workList;
        // for(auto [k, v] : loadStoreMap) {
        //     if(v == 3) {
        //         if(convertReduceToAtomic) {
        //             // match load-store pair
        //             // FIXME: check pointer aliasing
        //             // v0 = load ptr
        //             // v1 = v0 + inc
        //             // store v1, ptr
        //             // ==>
        //             // atomicadd ptr, inc

        //            Instruction* load = nullptr;
        //            for(auto b : body)
        //                for(auto& inst : b->instructions()) {
        //                    if(inst.getInstID() == InstructionID::Load || inst.getInstID() == InstructionID::Store) {
        //                        const auto base = pointerBase->lookup(inst.getOperand(0));
        //                        if(base == k) {
        //                            if(inst.getInstID() == InstructionID::Load) {
        //                                if(load)
        //                                    return false;
        //                                load = &inst;
        //                            } else {
        //                                if(!load)
        //                                    return false;
        //                                const auto store = &inst;
        //                                if(load->getOperand(0) == store->getOperand(0) && load->hasExactlyOneUse() &&
        //                                   load->getBlock() == store->getBlock()) {
        //                                    const auto val = store->getOperand(1);
        //                                    Value *loadVal = load, *rhs;
        //                                    if(oneUse(add(exactly(loadVal), any(rhs)))(MatchContext<Value>{ val })) {
        //                                        workList.emplace_back(load, store);
        //                                        load = nullptr;
        //                                    } else
        //                                        return false;
        //                                } else
        //                                    return false;
        //                            }
        //                        }
        //                    }
        //                }
        //            if(load)
        //                return false;
        //            continue;
        //        }
        //        return false;
        //    }
        //}

        // for(auto [load, store] : workList) {
        //     const auto block = load->basicblock.get();
        //     const auto ptr = store->getOperand(0);
        //     const auto val = store->getOperand(1)->as<Instruction>();
        //     const auto inc = val->getOperand(0) == load ? val->getOperand(1) : val->getOperand(0);
        //     const auto atomicAdd = make<AtomicAddInst>(ptr, inc);
        //     auto& insts = block->instructions();
        //     insts.erase(load->asNode());
        //     insts.erase(val->asNode());
        //     atomicAdd->insertBefore(block, store->asIterator());
        //     insts.erase(store->asNode());
        // }
    }
    std::cout << GREEN "deal with load and store success" RESET << endl;

    //  FIXME:  暂时 用 void type
    vector<ValuePtr> args;
    auto bodyFunc = std::make_unique<Function>(giv ? giv->getType() : Type::getVoid(), "yatcc_loop_body", args);
    bodyFunc->noRecurse = true;
    bodyFunc->loopBody = true;
    std::cout << GREEN "create function named " << bodyFunc->getName() << RESET << endl;
    mod.pushFunction(std::move(bodyFunc));
    auto bodyFuncPtr = mod.getGlobalFunctions().back();

    std::unordered_map<Value*, Value*> val2arg;
    // TODO: 需要立即处理 全局参数问题 (很久以前的注释)
    // FIXME:  value 无法打印  需要variable 或者 inst 所以这里的arg 有问题
    val2arg.emplace(loop->inductionVar, bodyFuncPtr->addArg(loop->inductionVar->type));
    if(giv) {
        val2arg.emplace(giv, bodyFuncPtr->addArg(giv->getType()));
    }
    // print val2arg

    if(duplicateCmp)
        for(auto block : body) {
            const auto terminator = block->getTerminator();
            if(terminator->insId != InsID::Br)
                continue;
            // should be conditional branch
            const auto cond = terminator->as<BrInstruction>()->getCondition();
            if(cond == nullptr)
                continue;
            // FIXME:  如果碰到永真 不清楚？？？
            if(body.count(cond->getBasicBlock()))
                continue;
            // TODO: should 立刻处理 cmp
            // CompareOp cmp;
            Value* v1;
            int i1;
            if(icmp(any(v1), int_(i1))(MatchContext<Value>{ cond })) {
                const auto newCond = cond->as<Instruction>()->clone();
                // newCond->insertBefore(block, terminator->asIterator());
                auto iter = std::find_if(block->instructionsRef().begin(), block->instructionsRef().end(),
                                         [&](const auto& inst) { return inst.get() == terminator; });
                insertBefore(block, iter, newCond);
                terminator->getOperandsRef().front()->replaceValue(newCond);
            }
        }
    std::cout << GREEN "duplicate cmp success" RESET << endl;

    // TODO: 需要立即处理全局参数问题
    for(auto block : body) {
        for(auto& inst : block->instructionsRef()) {
            for(auto& operand : inst->getOperandsRef()) {
                if(val2arg.count(operand->val))
                    continue;
                if(operand->val->isConst || (operand->val->is<Variable>() && operand->val->as<Variable>()->isGlobal))
                    continue;
                if(body.count(operand->val->getBasicBlock())) {
                    continue;
                }
                val2arg.emplace(operand->val, bodyFunc->addArg(operand->val->getType()));
            }
        }
    }

    // print val2arg
    std::cout << GREEN "val2arg: " RESET << endl;

    std::unordered_map<Value*, Value*> arg2Val;
    // std::vector<Value*> args;
    for(auto [k, v] : val2arg) {
        arg2Val.emplace(v, k);
        auto track = k->as<User>();
        for(auto it = track->users().begin(); it != track->users().end();) {  // NOLINT
            auto next = std::next(it);
            if(body.count(it.use->user->getBasicBlock())) {
                it.use->replaceValue(v);
            }
            it = next;
        }
    }

    std::vector<Value*> callArgs;
    callArgs.reserve(bodyFuncPtr->formArguments.size());
    for(auto arg : bodyFuncPtr->formArguments)
        callArgs.push_back(arg2Val.at(arg));

    // print callArgs
    std::cout << GREEN "callArgs: " << RESET << endl;

    // TODO: 解决所有权 立即解决

    bodyFuncPtr->basicBlocks.emplace_back(std::unique_ptr<BasicBlock>(loop->header->releaseFromParent()));
    loop->header->setBelongFunc(bodyFuncPtr);
    for(auto block : body) {
        block->belongfunc = bodyFuncPtr;
        if(block != loop->header)
            bodyFunc->basicBlocks.emplace_back(unique_ptr<BasicBlock>(block->releaseFromParent()));
    }

    std::cout << GREEN "push basic block success to new function" RESET << endl;

    IRbuilder builder{};

    // TODO:  立即处理 to deal with return inst
    const auto oldCond = loop->latch->getTerminator()->getOperand(0);

    const auto exit = loop->latch->getTerminator()->as<BrInstruction>()->getFalseTarget();

    loop->latch->instructionsRef().pop_back();

    builder.setCurBasicBlock(loop->latch);

    if(giv) {
        // make return inst
        builder.createRet(giv->as<PhiInstruction>()->incomings().at(loop->latch)->val);
    } else {
        builder.createRet(Void::get());
    }

    builder.setCurFunc(func);
    // TODO: 处理原来函数的loop, add new basicblock
    // FIXME: 暂时只处理一个并行，只有一个新的基本块？？
    auto newLoop = builder.addBasicBlock("new_loop");

    for(auto it = loop->header->instructionsRef().begin(); it != loop->header->instructionsRef().end();) {
        auto& inst = *it;
        if(inst->insId == InsID::Phi) {
            const auto next = std::next(it);
            // inst.insertBefore(newLoop, newLoop->instructions().begin());
            // insertBefore(newLoop, newLoop->instructions().begin(), inst.get());
            moveBefore(newLoop, newLoop->instructionsRef().begin(), inst.get());
            it = next;
        } else
            break;
    }
    for(auto pred : cfg.predecessors(loop->header)) {
        if(pred != loop->latch)
            resetTarget(pred->getTerminator()->as<BrInstruction>(), loop->header, newLoop);
    }
    for(auto succ : cfg.successors(loop->latch)) {
        if(succ != loop->header)
            retargetBlock(succ, loop->latch, newLoop);
    }

    builder.setCurBasicBlock(newLoop);
    // FIXME: call args is valu, 无法打印
    // const auto call = builder.makeOp<FunctionCallInst>(bodyFunc, callArgs);
    auto call = builder.createCall(bodyFuncPtr, callArgs);
    //  const auto next = loop.next->as<Instruction>()->clone();
    auto next = loop->next->as<Instruction>()->clone();
    insertBefore(newLoop, newLoop->instructionsRef().end(), next);
    //  next->insertBefore(newLoop, newLoop->instructions().end());
    //  const auto cond = oldCond->as<Instruction>()->clone();
    auto cond = oldCond->as<Instruction>()->clone();
    //  cond->insertBefore(newLoop, newLoop->instructions().end());
    insertBefore(newLoop, newLoop->instructionsRef().end(), cond);
    builder.setCurBasicBlock(newLoop);
    builder.createBranch(newLoop, exit, cond);
    loop->inductionVar->as<PhiInstruction>()->removeSource(loop->latch);
    loop->inductionVar->as<PhiInstruction>()->addIncoming(newLoop, next);
    if(giv) {
        giv->as<PhiInstruction>()->removeSource(loop->latch);
        giv->as<PhiInstruction>()->addIncoming(newLoop, call);
    }
    for(auto inst : std::initializer_list<Instruction*>{ cond, next, loop->inductionVar->as<Instruction>(),
                                                         giv ? giv->as<Instruction>() : nullptr }) {
        if(!inst)
            continue;
        for(auto& operand : inst->getOperandsRef()) {
            if(auto it = arg2Val.find(operand->val); it != arg2Val.end()) {
                operand->replaceValue(it->second);
            }
            if(operand->val == loop->next)
                operand->replaceValue(next);
        }
    }

    for(auto val : allowedToBeUsedByOuter) {
        if(val == loop->inductionVar || val == giv)
            continue;
        if(!val->isInstruction())
            continue;
        const auto tracked = val->as<User>();
        Value* rep = val == loop->next ? next : call;
        for(auto it = tracked->users().begin(); it != tracked->users().end();) {
            auto nextIt = std::next(it);
            auto block = it.use->user->getBasicBlock();
            if(block != newLoop && !body.count(block)) {
                it.use->replaceValue(rep);
            }
            it = nextIt;
        }
    }

    // bodyFunc->dump(std::cerr, Noop{});
    // if(!bodyFunc->verify(std::cerr))
    //     reportUnreachable(CMMC_LOCATION());
    if(ret) {
        ret->loop = newLoop;
        ret->indvar = loop->inductionVar->as<PhiInstruction>();
        ret->bound = loop->bound;
        ret->rec = (giv ? giv->as<PhiInstruction>() : nullptr);
        ret->recUsedByOuter = givUsedByOuter;
        ret->recUsedByInner = givUsedByInner;
        ret->recInnerStep = givAddRecInnerStep;
        ret->recNext = call->as<CallInstruction>();
        ret->exit = exit;
    }
    //    return true;
    return false;
}
