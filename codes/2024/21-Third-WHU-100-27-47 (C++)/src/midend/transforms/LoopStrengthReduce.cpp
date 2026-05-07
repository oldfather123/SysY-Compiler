#include "LoopStrengthReduce.h"
#include "LoopDetection.h"
#include "CFA.h"
#include "DFA.h"
#include "UseDefAnalysis.h"
#include "LoopInvariantAnalysis.h"
#include "LoopIndVarAnalysis.h"

using namespace ir;
using IndVar = LoopIndVarAnalysisContext::IndVar;
using BasicIndVar = LoopIndVarAnalysisContext::BasicIndVar;
using DerivedIndVar = LoopIndVarAnalysisContext::DerivedIndVar;

bool ir::loopStrengthReduce(ir::FuncPtr func) {
    bool changed = false;

    dbgout << std::endl
           << "LoopStrengthReduce pass started (" << func->name() << ")." << std::endl;

    CFAContext cfaCtx{func};
    LoopDetectionContext loopDetCtx{cfaCtx};
    DFAContext dfaCtx{func, DFAContext::RD | DFAContext::LV};
    UseDefAnalysisContext useDefCtx{dfaCtx};

    for (auto loop : loopDetCtx.loops()) {
        LoopInvariantAnalysisContext loopInvarCtx{loop, cfaCtx, dfaCtx, useDefCtx};

        // Induction variable analysis
        LoopIndVarAnalysisContext loopIndVarCtx{loopInvarCtx};
        loopIndVarCtx.printDebug();

        Map<Ptr<DerivedIndVar>, bool> processed{};
        std::function<void(Ptr<DerivedIndVar>)> handleDerivedIndVar = [&](Ptr<DerivedIndVar> div) -> void {
            if (processed[div]) {
                return;
            }
            auto dep = div->dep;

            // 递归处理依赖的归纳变量
            if (auto depDIV = castPtr<DerivedIndVar>(dep)) {
                handleDerivedIndVar(depDIV);
            }

            // 预处理初值和步长
            auto ivDataType = div->indReg->dataType();
            auto stepDataType = dep->getStep().dataType();
            auto preheaderBB = loop->getOrCreatePreheaderBB();
            div->computedInitVal = Register::create(func, ivDataType, div->indReg->name() + ".initval");
            div->computedStep = Register::create(func, stepDataType, div->indReg->name() + ".step");
            if (div->isMulDiv) {
                // 乘除法，默认 bias 为 0
                // 计算初值
                Instruction::insertBefore(preheaderBB->exitInst(), OperInst::createBinary(preheaderBB, div->computedInitVal.getRegister(), div->recipCoef ? Op::Div : Op::Mul, dep->getInitVal(), div->coef));
                // 计算步长
                Value stepVal = dep->getStep();
                if (auto depBIV = castPtr<BasicIndVar>(dep)) {
                    if (depBIV->subtractStep) {
                        auto newStepValReg = Register::create(func, stepDataType);
                        Instruction::insertBefore(preheaderBB->exitInst(), OperInst::createBinary(preheaderBB, newStepValReg, Op::Sub, Literal{stepDataType}, stepVal));
                        stepVal = newStepValReg;
                    }
                }
                Instruction::insertBefore(preheaderBB->exitInst(), OperInst::createBinary(preheaderBB, div->computedStep.getRegister(), div->recipCoef ? Op::Div : Op::Mul, stepVal, div->coef));
            } else {
                // 加减法，coef 为 1/-1
                // step = coef * i.step
                // initVal = coef * i.initVal + bias
                // 计算初值
                auto ci0Reg = Register::create(func, dep->getInitVal().dataType());
                Instruction::insertBefore(preheaderBB->exitInst(), OperInst::createBinary(preheaderBB, ci0Reg, Op::Mul, div->coef, dep->getInitVal()));
                Instruction::insertBefore(preheaderBB->exitInst(), OperInst::createBinary(preheaderBB, div->computedInitVal.getRegister(), div->negBias ? Op::Sub : Op::Add, ci0Reg, div->bias));
                // 计算步长
                Value stepVal = dep->getStep();
                if (auto depBIV = castPtr<BasicIndVar>(dep)) {
                    if (depBIV->subtractStep) {
                        auto newStepValReg = Register::create(func, stepDataType);
                        Instruction::insertBefore(preheaderBB->exitInst(), OperInst::createBinary(preheaderBB, newStepValReg, Op::Sub, Literal{stepDataType}, stepVal));
                        stepVal = newStepValReg;
                    }
                }
                Instruction::insertBefore(preheaderBB->exitInst(), OperInst::createBinary(preheaderBB, div->computedStep.getRegister(), Op::Mul, div->coef, stepVal));
            }

            if (!div->inDesiredForm) {
                // 创建新的变量
                auto newReg = Register::create(func, ivDataType, div->indReg->name());
                if (div->indReg->dataType().isPointer()) {
                    newReg->joinAliasGroup(div->indReg);
                }
                dbgout << "├── Created new induction variable `" << newReg->toString() << "` for derived induction variable `" << div->indReg->toString() << "`." << std::endl;

                // 将原变量的初值减去步长，得到新变量的初值
                auto newInitValReg = Register::create(func, ivDataType);
                Instruction::insertBefore(preheaderBB->exitInst(), OperInst::createBinary(preheaderBB, newInitValReg, Op::Sub, div->getInitVal(), div->getStep()));

                // 替换定值指令
                Instruction::replace(div->defInst, OperInst::createBinary(div->defInst->parentBB(), div->indReg, Op::Add, newReg, div->getStep()));

                // 创建 Phi 节点
                Ptr<PhiInst> phi = nullptr;
                auto headerBB = loop->headerBB();
                phi = PhiInst::create(headerBB, newReg);
                Instruction::insertAfter(headerBB->entryInst(), phi);
                for (auto srcBB : headerBB->preds()) {
                    if (!loop->bbs().count(srcBB)) {
                        // 来自循环外的是新变量的初值
                        phi->addTuple(srcBB, newInitValReg);
                    } else {
                        // 来自循环内的是归纳变量
                        phi->addTuple(srcBB, div->indReg);
                    }
                }
            }

            processed[div] = true;
        };

        for (auto iv : loopIndVarCtx.ivs) {
            if (auto div = castPtr<DerivedIndVar>(iv)) {
                handleDerivedIndVar(div);
            }
        }
    }

    dbgout << "└── LoopStrengthReduce pass done." << std::endl;

    return changed;
}
