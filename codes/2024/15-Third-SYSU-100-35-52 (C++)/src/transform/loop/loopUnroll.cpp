#include "Block.h"
#include "IRbuilder.h"
#include "Instruction.h"
#include "loopAnalysis.h"
#include "loopUnroll.h"
#include <memory>


// reference: https://github.com/dtcxzyw/cmmc/blob/3888660547f82af579fb9013cdfefea52fd923f8/cmmc/Transforms/Loop/LoopUnroll.cpp#L4

static uint32_t estimateBlockSize(BasicBlockPtr block, bool dynamic) {
    uint32_t count = 0;
    for(auto& inst : block->instructionsRef()) {
        if(inst->isTerminator()) {
            if(inst->isContiditonBranch() && inst->getOperand(0)->getBasicBlock() == block &&
               inst->getOperand(0)->as<Instruction>()->isCompare())
                --count;
            break;
        }
        switch(inst->insId) {
            case InsID::Call: {
                // const auto callee = inst.lastOperand()->as<Function>();
                //  when loop Parallel
                // if(callee->attr().hasAttr(FunctionAttribute::LoopBody)) {
                //     for(auto b : callee->blocks())
                //         count += estimateBlockSize(b, dynamic);
                // } else
                count += 16;
            } break;
            case InsID::Load:
            case InsID::Store:
                count += dynamic ? 4 : 1;
                break;
            case InsID::AtomicAdd:
                count += 4;
                break;
            case InsID::GEP:
            case InsID::LoopGEP:
            case InsID::Bitcast:
            case InsID::Phi:
            case InsID::Alloca:
                break;
            default:
                if(inst->insId == InsID::Binary) {
                    const auto bin = inst->as<BinaryInstruction>();
                    if(bin->op == BinaryInstructionOps::Div || bin->op == BinaryInstructionOps::Rem)
                        count += 4;
                } else
                    ++count;
                break;
        }
    }
    return count;
}

static bool hasCall(BasicBlock* block, bool excludeLoopBody) {
    for(auto& inst : block->instructionsRef())
        if(inst->insId == InsID::Call) {
            // LOOP Parallel
            // if(excludeLoopBody) {
            //    const auto callee = inst.lastOperand()->as<Function>();
            //    if(callee->attr().hasAttr(FunctionAttribute::LoopBody))
            //        continue;
            //}
            return true;
        }
    return false;
}

bool runLoopUnroll(FunctionPtr func, LoopAnalysisResult& loopInfo, CFGAnalysisResult& cfg) {

    bool modified = false;
    for(auto& loop : loopInfo.loops) {
        // innermost loop
        if(loop.header != loop.latch)
            continue;
        if(std::abs(loop.step) > 65536)
            continue;
        const auto blockSize = std::max(2U, estimateBlockSize(loop.header, false)) - 1U;
        if(blockSize * 2 > 128)
            continue;
        if(hasCall(loop.header, true))
            continue;
        if(!loop.bound->is<Const>())
            continue;
        if(!loop.initVal->is<Const>())
            continue;

        int initial = loop.initVal->as<Const>()->intVal;  // NOLINT
        int bound = loop.bound->as<Const>()->intVal;      // NOLINT
        assert(loop.step != 0);

        // const auto size = (bound - initial + loop.step + (loop.step > 0 ? -1 : 1)) / loop.step;
        auto tripCount = loop.getTripCount();
        assert(tripCount >= 0);

        const auto maxUnrollSize = 128 / blockSize;
        const auto epilogueSize = tripCount > maxUnrollSize ? tripCount % 4 : tripCount;
        if(std::max(epilogueSize, epilogueSize == tripCount ? 0 : 4) > maxUnrollSize)
            continue;
        const auto epilogueStart = initial + (tripCount - epilogueSize) * loop.step;
        assert(epilogueStart != initial || epilogueSize > 0);

        modified = true;
        std::vector<unique_ptr<BasicBlock>> insertedBlocks;
        using ReplaceMap = std::unordered_map<Value*, Value*>;
        std::unordered_map<BasicBlock*, ReplaceMap> replaceMap;

        BasicBlock* prev = loop.latch;
        const auto retarget = [&](BasicBlock* block) {
            prev->instructionsRef().pop_back();
            // IRBuilder builder{ target, prev };
            IRbuilder builder{ prev };
            // builder.makeOp<BranchInst>(block);
            builder.createJump(block);
            for(auto& inst : block->instructionsRef()) {
                if(inst->insId == InsID::Phi) {
                    ReplaceMap replace;
                    const auto phi = inst.get()->as<PhiInstruction>();  // NOLINT
                    for(auto [pred, val] : phi->incomings()) {
                        if(pred != loop.latch) {
                            phi->removeSource(pred);
                            break;
                        }
                    }

                    if(loop.latch != prev && phi->incomings().count(loop.latch)) {
                        // block->dump(std::cerr, HighlightInst{ phi });
                        phi->replaceSource(loop.latch, prev);
                    }
                    if(replaceMap.count(prev)) {
                        const auto val = phi->incomings().at(prev);
                        auto& map = replaceMap.at(prev);
                        if(map.count(val->val)) {
                            const auto rep = map.at(val->val);
                            val->replaceValue(rep);
                        }
                    }
                } else
                    break;
            }
        };
        const auto append = [&]() {
            ReplaceMap replace;
            // const auto block = loop.header->clone(replace, false);
            auto block = loop.header->clone(replace);
            insertedBlocks.push_back(std::move(block));
            auto tmp = insertedBlocks.back().get();
            replaceMap[tmp] = std::move(replace);
            retarget(tmp);
            prev = tmp;
        };
        const auto startValue = Const::getConst(loop.bound->getType(), epilogueStart);
        BasicBlock* head;
        if(epilogueStart != initial) {
            // super blocks

            {
                ReplaceMap replace;
                // head = loop.header->clone(replace, false);
                auto block = loop.header->clone(replace);
                insertedBlocks.push_back(std::move(block));
                auto tmp = insertedBlocks.back().get();
                head = tmp;
                replaceMap[head] = std::move(replace);
                for(auto block : cfg.predecessors(loop.latch)) {
                    resetTarget(block->getTerminator()->as<BrInstruction>(), loop.latch, tmp);
                }
                prev = tmp;
            }

            for(uint32_t idx = epilogueSize ? 1 : 2; idx < 4; ++idx)
                append();

            if(!epilogueSize) {
                // use original block directly
                retarget(loop.latch);
                prev = loop.latch;
            }

            const auto terminator = prev->getTerminator()->as<BrInstruction>();
            // reset terminator
            terminator->trueTarget = head;
            // loop.bound -> startValue
            terminator->getOperand(0)->as<IcmpInstruction>()->getOperandsRef()[1]->replaceValue(startValue);
            const auto& replace = replaceMap[prev];
            for(auto& inst : head->instructionsRef()) {
                if(inst->insId == InsID::Phi) {
                    const auto phi = inst.get()->as<PhiInstruction>();  // NOLINT
                    auto val = phi->incomings().at(loop.latch)->val;
                    if(replace.count(val))
                        val = replace.at(val);
                    phi->removeSource(loop.latch);
                    phi->addIncoming(prev, val);
                } else
                    break;
            }

            // const auto tripCount = (size - epilogueSize) / heuristic.unrollBlockSize;
        }

        if(epilogueSize) {
            auto exitSuperBlock = [&] {
                const auto terminator = prev->getTerminator()->as<BrInstruction>();
                terminator->falseTarget = head;
            };

            if(epilogueSize == 1) {
                for(auto& inst : loop.latch->instructionsRef()) {
                    if(inst->insId == InsID::Phi) {
                        const auto phi = inst.get()->as<PhiInstruction>();  // NOLINT
                        phi->clear();
                        const auto val = replaceMap.at(head).at(phi)->as<PhiInstruction>()->incomings().at(prev)->val;
                        phi->addIncoming(prev, val);
                    } else
                        break;
                }
                head = loop.latch;
                exitSuperBlock();
                prev = loop.latch;
            } else {
                {
                    ReplaceMap replace;
                    // head = loop.latch->clone(replace, false);
                    //head = loop.latch->clone(replace);
                    auto block = loop.latch->clone(replace);
                    insertedBlocks.push_back(std::move(block));
                    auto tmp = insertedBlocks.back().get();
                    head = tmp;
                    replaceMap[head] = std::move(replace);
                }

                if(prev == loop.latch) {
                    for(auto block : cfg.predecessors(loop.latch)) {
                        resetTarget(block->getTerminator()->as<BrInstruction>(), loop.latch, head);
                    }
                    for(auto& inst : head->instructionsRef()) {
                        if(inst->insId == InsID::Phi) {
                            const auto phi = inst.get()->as<PhiInstruction>();  // NOLINT
                            phi->removeSource(loop.latch);
                        } else
                            break;
                    }
                } else {
                    exitSuperBlock();
                    auto& replace = replaceMap[prev];
                    for(auto& inst : head->instructionsRef()) {
                        if(inst->insId == InsID::Phi) {
                            const auto phi = inst.get()->as<PhiInstruction>();  // NOLINT
                            auto val = phi->incomings().at(loop.latch)->val;
                            if(replace.count(val))
                                val = replace.at(val);
                            phi->clear();
                            phi->addIncoming(prev, val);
                        } else
                            break;
                    }
                }

                prev = head;

                for(uint32_t idx = 2; idx < epilogueSize; ++idx) {
                    append();
                }

                {
                    retarget(loop.latch);
                    prev = loop.latch;
                }
            }

            // remove backedge
            const auto terminator = prev->getTerminator()->as<BrInstruction>();
            const auto exiting = terminator->getFalseTarget();
            prev->instructionsRef().pop_back();
            // const auto branch = make<BranchInst>(exiting);
            auto branch = new BrInstruction(exiting);  // NOLINT
            // branch->insertBefore(prev, prev->instructions().end());
            insertBefore(prev, prev->instructionsRef().end(), branch);
        }

        auto& blocks = func->basicBlocks;
        auto iter = std::find_if(blocks.cbegin(), blocks.cend(), [&](auto& ptr) { return ptr.get() == loop.latch; });

        for(auto& block : insertedBlocks) {
            //if (block == insertedBlocks.back()) break;
            iter = blocks.insert(iter, std::move(block)) + 1;
        }
        // FIXME
        //blocks.insert(blocks.end(), std::move(insertedBlocks.back()));
        std::cout << "hello" << endl;
    }

    return modified;
}