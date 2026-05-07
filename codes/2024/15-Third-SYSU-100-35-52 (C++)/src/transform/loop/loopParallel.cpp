#include "IRbuilder.h"
#include "Instruction.h"
#include "Module.h"
#include "Type.h"
#include "Value.h"
#include "Variable.h"
#include "loopAnalysis.h"
#include "loopParallel.h"
#include "loopParallelUtil.h"
// #include "Module.h"
// #include "Value.h"
#include <CFGAnalysis.h>
// #include <Loop.h>
#include <cassert>
#include <domTreeAnalysis.h>
// #include <memory>
//

static Value* expandConstant(Value* val, IRbuilder& builder) {
    if(val->isConst)
        return val;
    if(val->is<Variable>() && val->as<Variable>()->isGlobal)
        return val;
    if(val->isInstruction()) {
        auto inst = val->as<Instruction>()->clone();
        for(auto& operand : inst->getOperandsRef())
            operand->replaceValue(expandConstant(operand->val, builder));
        const auto point = builder.getInsertPoint();
        const auto block = builder.getCurBasicBlock();
        insertBefore(block, point, inst);
        return inst;
    }
    assert(false);

}

static bool isNoSideEffectExpr(const Instruction& inst) {
    if(!inst.canbeOperand())
        return false;
    if(inst.isTerminator())
        return false;
    switch(inst.insId) {
        case InsID::Store:
        case InsID::AtomicAdd: {
            return false;
        }
        case InsID::Call: {
            // const auto callee = inst.lastOperand();
            auto callee = inst.getOperand(inst.getNumOperands() - 1);
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

static bool isMovableExpr(const Instruction& inst, bool relaxedCtx) {
    if(!isNoSideEffectExpr(inst))
        return false;
    switch(inst.insId) {
        case InsID::Alloca:
        case InsID::Phi:
        case InsID::Load:
            return false;
        // It is not safe to speculate division, since SIGFPE may be raised.
        default:
            return true;
    }
}
// FIXME:
static bool isConstant(Value* val) {
    if(val->isConst)
        return true;
    if(val->is<Variable>() && val->as<Variable>()->isGlobal)
        return true;
    if(val->isInstruction()) {
        auto inst = val->as<Instruction>();
        if(!isMovableExpr(*inst, false))
            return false;
        for(auto& opreand : inst->getOperandsRef()) {
            if(!isConstant(opreand->val))
                return false;
        }
        return true;
    }
    return false;
}

static Function* lookupParallelFor(Module& mod) {
    for(auto& func : mod.globalFunctions) {
        if(func->getName().find("cmmcParallelFro") != string::npos) {
            return func.get()->as<Function>();  // NOLINT
        }
    }
    //auto func = make_unique<Function>(Void::get(), "cmmcParallelFor");
    //func->isLib = true;
    //func->noRecurse = true;
    //// vector<Value*> cacheLookupArgv = { new Variable(table, ""), new Variable(i32, ""), new Variable(i32, "") };
    //// func->formArguments = std::move(cacheLookupArgv);
    //mod.pushFunction(std::move(func));
    return mod.globalFunctions.back().get();
}

bool runLoopParallel(FunctionPtr func, Module& mod) {
    func->dump();
    auto cfg = runCFGAnalysis(func);
    auto dom = runDominateTreeAnalysis(func, cfg);
    auto loopInfo = runLoopAnalysis(func, dom);
    auto loops = loopInfo.loops;
    std::sort(loops.begin(), loops.end(), [&](Loop& lhs, Loop& rhs) {  // NOLINT
        return dom.getIndex(lhs.header) < dom.getIndex(rhs.header);
    });

    bool modified = false;
    for(auto& loop : loops) {
        if(loop.step != 1)
            continue;
        auto tripCount = loop.getTripCount();
        if(tripCount < 0)
            continue;
        // 暂不清楚 interRange 是什么
        // auto initialRange = rangeInfo.query(loop.initial, dom, nullptr, 5);
        // const auto isAligned = (initialRange.knownZeros() & 3) == 3;
        // auto boundRange = rangeInfo.query(loop.bound, dom, nullptr, 5);
        // const auto needSubLoop = (boundRange - initialRange).maxSignedValue() < 400;
        auto needSubLoop = tripCount < 400;
        // std::cerr << "collect\n";
        LoopBodyInfo bodyInfo;
        // Module mod;
        if(!extractLoopBody(func, &loop, dom, cfg, mod,
                            /*independent*/ true,
                            /*allow innermost*/ true,
                            /*allow innerloop*/ true,
                            /*only addrec*/ true,
                            /*estimate block size*/ false,
                            /*need sub-loop*/ needSubLoop,
                            /*convert reduce to atomic*/ true,
                            /*duplicate cmp*/ true,
                            /*ret*/ &bodyInfo))
            continue;
        const auto parallelFor = lookupParallelFor(mod);

        bodyInfo.indvar->removeSource(bodyInfo.loop);
        if(bodyInfo.rec)
            bodyInfo.rec->removeSource(bodyInfo.loop);

        // auto getUniqueIDStorage = [&] {
        //     auto base = String::get("cmmc_parallel_body_payload_");
        //     for(int32_t id = 0;; ++id) {
        //         const auto name = base.withID(id);
        //         bool used = false;
        //         for(auto global : mod.globals()) {
        //             if(global->getSymbol() == name) {
        //                 used = true;
        //                 break;
        //             }
        //         }
        //         if(!used)
        //             return name;
        //     }
        // };

        // auto funcType = make<FunctionType>(VoidType::get(), Vector<const Type*>{});
//        auto bodyFunc = make_unique<Function>("cmmc_parallel_body_", Void::get());
//        auto bodyFuncPtr = bodyFunc.get();
//        bodyFunc->noRecurse = true;
//        bodyFunc->parallelBody = true;
//        mod.pushFunction(std::move(bodyFunc));
//
//        std::vector<std::pair<Value*, size_t>> payload;
//        size_t totalSize = 0;
//        size_t maxAlignment = 0;
//        Value* givOffset = nullptr;
//        const auto i32 = Type::getInt();
//        const auto f32 = Type::getFloat();
//
//        std::unordered_set<Value*> insertedParam;
//        auto addArgument = [&](Value* param) {
//            if(param == bodyInfo.indvar)
//                return;
//            if(isConstant(param))
//                return;
//            if(param == bodyInfo.rec && !bodyInfo.recUsedByOuter && !bodyInfo.recUsedByInner)
//                return;
//
//            if(!insertedParam.insert(param).second)
//                return;
//            // FIXME:
//            // const auto align = param->getType()->getAlignment(dataLayout);
//            // const auto size = param->getType()->getSize(dataLayout);
//            // totalSize = (totalSize + align - 1) / align * align;
//            totalSize = (totalSize + 3) / 4 * 4;
//            // maxAlignment = std::max(maxAlignment, align);
//            if(param == bodyInfo.rec)
//                givOffset = Const::getConst(i32, totalSize);
//            else
//                payload.emplace_back(param, totalSize);
//            totalSize += 4;
//        };
//
//        // FIXME:
//        for(auto& param : bodyInfo.recNext->getOperandsRef())
//            addArgument(param->val);
//
//        if(bodyInfo.recInnerStep)
//            addArgument(bodyInfo.recInnerStep);
//
//        // const auto payloadStorage =
//        //     make<GlobalVariable>(getUniqueIDStorage(), make<ArrayType>(IntegerType::get(8), static_cast<uint32_t>(totalSize)),
//        //                          std::max(dataLayout.getStorageAlignment(), maxAlignment));
//        auto payloadStorge = new Variable(new ArrType(Type::getInt8(), totalSize), "cmmc_parallel_body_payload_");
//        // FIXME:
//        mod.pushVariable(payloadStorge, "");
//
//        IRbuilder builder{};
//        builder.setCurFunc(bodyFuncPtr);
//
//        const auto beg = bodyFuncPtr->addArg(i32);
//        const auto end = bodyFuncPtr->addArg(i32);
//        const auto entry = builder.addBasicBlock("entry");
//        const auto subLoop = builder.addBasicBlock("subLoop");
//        const auto exit = builder.addBasicBlock("exit");
//        builder.setCurBasicBlock(entry);
//        const auto bodyExec = bodyInfo.recNext->clone();
//        // const auto indVar = make<PhiInst>(i32);
//        const auto indVar = new PhiInstruction(i32);
//        indVar->addIncoming(entry, beg);
//        // const auto giv = (bodyInfo.rec ? make<PhiInst>(bodyInfo.rec->getType()) : nullptr);
//        const auto giv = (bodyInfo.rec ? new PhiInstruction(bodyInfo.rec->getType()) : nullptr);
//
//        // FIXME:
//        auto remapArgument = [&](Use* operand) {
//            if(operand->val == bodyInfo.indvar) {
//                operand->replaceValue(indVar);
//            } else if(operand->val == bodyInfo.rec) {
//                operand->replaceValue(giv);
//            } else {
//                if(isConstant(operand->val)) {
//                    operand->replaceValue(expandConstant(operand->val, builder));
//                    builder.setCurBasicBlock(entry);
//                } else {
//                    bool replaced = false;
//                    for(auto& [param, offset] : payload) {
//                        if(operand->val == param) {
//                            const auto ptr = builder.makeOp<PtrAddInst>(payloadStorage,
//                                                                        ConstantInteger::get(i32, static_cast<intmax_t>(offset)),
//                                                                        PointerType::get(param->getType()));
//                            const auto val = builder.makeOp<LoadInst>(ptr);
//                            operand->replaceValue(val);
//                            replaced = true;
//                            break;
//                        }
//                    }
//                    if(!replaced)
//                        // reportUnreachable(CMMC_LOCATION());
//                        assert(false);
//                }
//            }
//        };
//        for(auto& operand : bodyExec->getOperandsRef())
//            remapArgument(operand.get());
//
//        // indVar->insertBefore(subLoop, subLoop->instructions().end());
//        insertBefore(subLoop, subLoop->instructionsRef().end(), indVar);
//        if(giv) {
//            // giv->insertBefore(subLoop, subLoop->instructions().end());
//            insertBefore(subLoop, subLoop->instructionsRef().end(), indVar);
//            if(giv->getType()->id == i32->id) {
//                if(bodyInfo.recUsedByInner) {
//                    // FIXME:
//                    const auto ptr = builder.makeOp<PtrAddInst>(payloadStorage, givOffset, PointerType::get(i32));
//                    const auto initial = builder.makeOp<LoadInst>(ptr);
//                    const auto step = bodyInfo.recInnerStep;
//                    const auto offset = builder.makeOp<BinaryInst>(InstructionID::Mul, beg, step);
//                    builder.setInsertPoint(entry, offset->asIterator());
//                    remapArgument(offset->mutableOperands().back().get());
//                    builder.setCurrentBlock(entry);
//                    const auto realInitial = builder.makeOp<BinaryInst>(InstructionID::Add, initial, offset);
//                    giv->addIncoming(entry, realInitial);
//                } else
//                    giv->addIncoming(entry, ConstantInteger::get(i32, 0));
//            } else if(giv->getType()->isSame(f32)) {
//                if(bodyInfo.recUsedByInner) {
//                    const auto ptr = builder.makeOp<PtrAddInst>(payloadStorage, givOffset, PointerType::get(f32));
//                    const auto initial = builder.makeOp<LoadInst>(ptr);
//                    const auto step = bodyInfo.recInnerStep;
//                    const auto offset = builder.makeOp<BinaryInst>(
//                        InstructionID::FMul, builder.makeOp<CastInst>(InstructionID::S2F, step->getType(), beg), step);
//                    builder.setInsertPoint(entry, offset->asIterator());
//                    remapArgument(offset->mutableOperands().back().get());
//                    builder.setCurrentBlock(entry);
//                    const auto realInitial = builder.makeOp<BinaryInst>(InstructionID::FAdd, initial, offset);
//                    giv->addIncoming(entry, realInitial);
//                } else
//                    giv->addIncoming(entry, make<ConstantFloatingPoint>(f32, 0.0));
//            } else
//                reportUnreachable(CMMC_LOCATION());
//            giv->addIncoming(subLoop, bodyExec);
//        }
//        builder.makeOp<BranchInst>(subLoop);
//
//        bodyExec->insertBefore(subLoop, subLoop->instructions().end());
//        builder.setCurrentBlock(subLoop);
//        const auto next = builder.makeOp<BinaryInst>(InstructionID::Add, indVar, ConstantInteger::get(i32, 1));
//        indVar->addIncoming(subLoop, next);
//        const auto cond = builder.makeOp<CompareInst>(InstructionID::ICmp, CompareOp::ICmpSignedLessThan, next, end);
//        builder.makeOp<BranchInst>(cond, defaultLoopProb, subLoop, exit);
//        builder.setCurrentBlock(exit);
//        if(giv && bodyInfo.recUsedByOuter) {
//            const auto ptr = builder.makeOp<PtrAddInst>(payloadStorage, givOffset, PointerType::get(giv->getType()));
//            if(giv->getType()->isSame(i32)) {
//                if(mod.getTarget().isNativeSupported(InstructionID::AtomicAdd)) {
//                    builder.makeOp<AtomicAddInst>(ptr, bodyExec);
//                } else {
//                    Function* reduceAddI32 = lookupReduceAddI32(mod);
//                    builder.makeOp<FunctionCallInst>(reduceAddI32, std::vector<Value*>{ ptr, bodyExec });
//                }
//            } else if(giv->getType()->isSame(f32)) {
//                Function* reduceAddF32 = lookupReduceAddF32(mod);
//                builder.makeOp<FunctionCallInst>(reduceAddF32, std::vector<Value*>{ ptr, bodyExec });
//            } else
//                reportUnreachable(CMMC_LOCATION());
//        }
//        builder.makeOp<ReturnInst>();
//
//        builder.setInsertPoint(bodyInfo.loop, bodyInfo.recNext);
//        Value* givPtr = nullptr;
//        if(giv && (bodyInfo.recUsedByOuter || bodyInfo.recUsedByInner)) {
//            givPtr = builder.makeOp<PtrAddInst>(payloadStorage, givOffset, PointerType::get(giv->getType()));
//            builder.makeOp<StoreInst>(givPtr, bodyInfo.rec);
//        }
//        for(auto [k, v] : payload) {
//            const auto ptr = builder.makeOp<PtrAddInst>(payloadStorage, ConstantInteger::get(i32, static_cast<intmax_t>(v)),
//                                                        PointerType::get(k->getType()));
//            builder.makeOp<StoreInst>(ptr, k);
//        }
//
//        builder.makeOp<FunctionCallInst>(
//            parallelFor,
//            std::vector<Value*>{ bodyInfo.indvar, bodyInfo.bound,
//                                 builder.makeOp<FunctionPtrInst>(bodyFunc, PointerType::get(IntegerType::get(8))) });
//        if(giv && bodyInfo.recUsedByOuter) {
//            const auto val = builder.makeOp<LoadInst>(givPtr);
//            bodyInfo.recNext->replaceWith(val);
//        }
//        bodyInfo.loop->instructions().erase(bodyInfo.recNext->asIterator(), bodyInfo.loop->instructions().end());
//        builder.setCurrentBlock(bodyInfo.loop);
//        builder.makeOp<BranchInst>(bodyInfo.exit);
//
//        // bodyFunc->dump(std::cerr, Noop{});
//        // if(!bodyFunc->verify(std::cerr))
//        //     reportUnreachable(CMMC_LOCATION());
//        // const auto loopExec = bodyInfo.recNext->lastOperand();
//        // loopExec->dump(std::cerr, Noop{});
//
//        modified = true;
//        break;
    }
    return modified;
}