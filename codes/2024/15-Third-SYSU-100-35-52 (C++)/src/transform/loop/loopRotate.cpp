#include "Block.h"
#include "CFGAnalysis.h"
#include "Function.h"
#include "Instruction.h"
#include "domTreeAnalysis.h"
#include "loopRotate.h"
#include <cassert>

// reference: https://github.com/dtcxzyw/cmmc/blob/3888660547f82af579fb9013cdfefea52fd923f8/cmmc/Transforms/Loop/LoopRotate.cpp#L60

struct SimpleLoopInfo final {
    BasicBlock* header;
    BasicBlock* latch;
};

constexpr uint32_t maxRotateCount = 8;

// emit ir 的 loop
static std::vector<SimpleLoopInfo> detectLoops(FunctionPtr func, DominateAnalysisResult& dom) {
    std::vector<SimpleLoopInfo> ret;
    for(auto& block : func->basicBlocks) {
        auto terminator = block->getTerminator();
        if(terminator->insId != InsID::Br)
            continue;
        const auto branch = terminator->as<BrInstruction>();
        const auto header = branch->getTrueTarget();
        if(header == block.get())
            continue;
        if(header->instructionsRef().front()->insId != InsID::Phi)
            continue;
        if(dom.dominate(header, block.get())) {
            ret.push_back({ header, block.get() });
        }
    }
    return ret;
}
//

static bool hasCall(BasicBlock* bb) {
    for(auto& inst : bb->instructionsRef())
        if(inst->insId == InsID::Call)
            return true;
    return false;
}

static bool runImpl(FunctionPtr func) {
    auto cfg = runCFGAnalysis(func);
    auto dom = runDominateTreeAnalysis(func, cfg);

    bool modified = false;
    auto loops = detectLoops(func, dom);
    for(auto loop : loops) {
        if(loop.header == loop.latch)
            continue;

        // 如果 有 call 指令 while ( i < call() )
        if (hasCall(loop.header)) continue;

        auto& succ = cfg.successors(loop.latch);
        // 必须无条件跳转到 header
        if(!(succ.size() == 1 && succ.front() == loop.header)) // NOLINT
            continue;

        auto& succHeader = cfg.successors(loop.header);
        // br latch  and  exit
        if(succHeader.size() != 2)
            continue;
        
        // just latch, !exit 不可能支配 latch？？？  
        BasicBlock* phiLoc = nullptr;
        for(auto b : succHeader)
            if(dom.dominate(loop.header, b) && dom.dominate(b, loop.latch)) {
                phiLoc = b;
            }
        // FIXME
        if(!phiLoc)
            continue;

        std::unordered_set<BasicBlock*> body;
        if(!collectLoopBody(loop.header, loop.latch, dom, cfg, body, true, false))
            continue;

        const auto exiting = succHeader.front() == phiLoc ? succHeader.back() : succHeader.front();
        if(body.count(exiting))
            continue;

        //TODO: to understand
        auto& rotateCount = loop.latch->rotateCount;
        if(rotateCount < maxRotateCount) {
            ++rotateCount;
            rotateCount = std::max(loop.header->rotateCount, rotateCount);
        } else
            continue;

        if(cfg.predecessors(phiLoc).size() != 1 || phiLoc->instructionsRef().front()->insId == InsID::Phi) {
            auto newPhiLoc = make_unique<BasicBlock>("phiLoc");
            newPhiLoc->belongfunc = func;
            auto tmp = newPhiLoc.get();
            // FIXME: insert 到一个合适的位置 
            func->basicBlocks.push_back(std::move(newPhiLoc));
            auto term = new BrInstruction(phiLoc); // NOLINT
            insertBefore(tmp, func->basicBlocks.back()->instructionsRef().end(),term);

            resetTarget(loop.header->getTerminator(), phiLoc, tmp);
            retargetBlock(phiLoc, loop.header, tmp);
            body.insert(tmp);
            phiLoc = tmp;
        }

        // reset target
        // 如果 出口 不止header一个前缀 或者 出口有phi
        BasicBlock* indirectBlock = nullptr;
        if(cfg.predecessors(exiting).size() != 1 || exiting->instructionsRef().front()->insId == InsID::Phi) {
            indirectBlock = new BasicBlock("indirect");   // NOLINT
            indirectBlock->belongfunc = func;
            resetTarget(loop.header->getTerminator(), exiting, indirectBlock);
        }

        // duplicate instructions
        std::unordered_map<Value*, PhiInstruction*> replace;
        loop.latch->instructionsRef().pop_back();
        std::unordered_set<Instruction*> oldInsts;
        std::unordered_set<Instruction*> newInsts;
        for(auto b : body)
            if(b != loop.header)
                for(auto& inst : b->instructionsRef())
                    oldInsts.insert(inst.get());
        for(auto& inst : loop.header->instructionsRef()) {
            // 一般就是 phi-> phi  以及 icmp -> phi.. 
            if(inst->canbeOperand()) {
                auto phi = new PhiInstruction(inst->getType());    //NOLINT
                replace.emplace(inst.get(), phi);
                insertBefore(phiLoc, phiLoc->instructionsRef().begin(), phi);
            }
        }

        for(auto& inst : loop.header->instructionsRef()) {
            if(inst->insId == InsID::Phi) {
                auto val = inst.get()->as<PhiInstruction>()->incomings().at(loop.latch)->val;   //NOLINT
                inst->as<PhiInstruction>()->removeSource(loop.latch);
                const auto phi = replace.at(inst.get());
                phi->addIncoming(loop.header, inst.get());
                if(auto iter = replace.find(val); iter != replace.end())
                    val = iter->second;
                phi->addIncoming(loop.latch, val);
            } else {
                auto newInst = inst->clone();
                
                assert(newInst->type->id == inst->type->id);
                insertBefore(loop.latch, loop.latch->instructionsRef().end(), newInst);
                if(inst->canbeOperand()) {
                    auto phi = replace.at(inst.get());
                    phi->addIncoming(loop.header, inst.get());
                    phi->addIncoming(loop.latch, newInst);
                }
                newInsts.insert(newInst);
            }
        }
        // body/tail header use header
        // body:
        //   new phis
        //   old body
        //   tail header
        for(auto& inst : loop.header->instructionsRef()) {
            auto& users = inst->users();
            for(auto iter = users.begin(); iter != users.end();) {
                auto next = std::next(iter);
                const auto user = iter.use->user;
                // FIXME: 有问题
                if(oldInsts.count(user)) {
                    // old body
                    iter.use->replaceValue(replace.at(inst.get()));
                } else if(newInsts.count(user)) {
                    // tail header
                    iter.use->replaceValue(replace.at(inst.get())->incomings().at(loop.latch)->val);
                } else {
                    // new phis/outer users
                }
                iter = next;
            }
        }

        // outer use header

        for(auto& inst : loop.header->instructionsRef()) {
            bool usedByOuter = false;
            auto& users = inst->users();
            for(auto it = users.begin(); it != users.end(); ++it) {
                auto ref = it.use;
                auto user = ref->user;
                const auto block = user->getBasicBlock();
                if(block == loop.header && user->insId == InsID::Phi) {
                    for(auto& [pred, val] : user->as<PhiInstruction>()->incomings()) {
                        if(val != ref)
                            continue;
                        if(!body.count(pred)) {
                            usedByOuter = true;
                            break;
                        }
                    }
                    if(usedByOuter)
                        break;
                } else {
                    if(!body.count(block)) {
                        usedByOuter = true;
                        break;
                    }
                }
            }

            if(!usedByOuter)
                continue;

            //const auto phi = make<PhiInst>(inst.getType());
            auto phi  = new PhiInstruction(inst->getType());    //NOLINT

            for(auto iter = users.begin(); iter != users.end();) {
                const auto next = std::next(iter);
                auto ref = iter.use;
                const auto user = ref->user;
                const auto block = user->getBasicBlock();

                if(!body.count(block)) {
                    ref->replaceValue(phi);
                } else if(block == loop.header && user->insId == InsID::Phi) {
                    for(auto& [pred, val] : user->as<PhiInstruction>()->incomings()) {
                        if(val != ref)
                            continue;
                        if(!body.count(pred)) {
                            ref->replaceValue(phi);
                            break;
                        }
                    }
                }
                iter = next;
            }

            phi->addIncoming(loop.header, inst.get());
            phi->addIncoming(loop.latch, replace.at(inst.get())->incomings().at(loop.latch)->val);
            if(indirectBlock)
                //phi->insertBefore(indirectBlock, indirectBlock->instructions().begin());
                insertBefore(indirectBlock, indirectBlock->instructionsRef().begin(), phi);
            else
                //phi->insertBefore(exiting, exiting->instructions().begin());
                insertBefore(exiting, exiting->instructionsRef().begin(), phi);
        }
        if(indirectBlock) {
            //const auto terminator = make<BranchInst>(exiting);
            auto term = new BrInstruction(exiting);    //NOLINT
            //terminator->insertBefore(indirectBlock, indirectBlock->instructions().end());
            insertBefore(indirectBlock, indirectBlock->instructionsRef().end(), term);
            
            //func.blocks().push_back(indirectBlock);
            func->basicBlocks.push_back(std::unique_ptr<BasicBlock>(indirectBlock));

            for(auto& phi : exiting->instructionsRef()) {
                if(phi->insId == InsID::Phi) {
                    auto phiInst = phi.get()->as<PhiInstruction>();    // NOLINT
                    phiInst->replaceSource(loop.header, func->basicBlocks.back().get());
                } else
                    break;
            }
        }

        modified = true;
        break;
    }
    return modified;
}

static bool cleanupPhi(FunctionPtr func) {
    bool modified = false;
    vector<Instruction*> toRemove;
    for(auto& block : func->basicBlocks) {
        auto& insts = block->instructionsRef();
        for(auto it = insts.begin(); it != insts.end();) {
            auto next = std::next(it);
            auto inst = it->get();
            if(inst->insId == InsID::Phi) {
                const auto phi = inst->as<PhiInstruction>();
                if(phi->incomings().size() == 1) {
                    inst->replaceWith(phi->incomings().begin()->second->val);
                    //inst->eraseFromParent();
                    toRemove.push_back(inst);
                }
            } else
                break;
            it = next;
        }
    }
    for (auto inst : toRemove) {
        inst->eraseFromParent();
    }

    return modified;
}

bool runLoopRotate(FunctionPtr func) {
    bool modified = false;
    modified |= cleanupPhi(func);
    while(runImpl(func)) {

        cleanupPhi(func);
        // func.dumpCFG(std::cerr);
        modified = true;
        // analysis.invalidateFunc(func);
    }
    return modified;
}