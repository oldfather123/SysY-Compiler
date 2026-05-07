#pragma once

#include "Common.h"
#include "IR.h"
#include "LoopInvariantAnalysis.h"

namespace ir {
    struct LoopIndVarAnalysisContext {
    private:
        Map<ir::Variable, Set<ir::InstPtr>> defInstMap{};
        Map<ir::InstPtr, Set<ir::InstPtr>> userMap{};

    public:
        ir::LoopInvariantAnalysisContext &loopInvarCtx;
        Ptr<ir::Loop> loop = nullptr;

        class IndVar {
        public:
            RegPtr indReg = nullptr;

        public:
            IndVar(RegPtr indReg) : indReg(indReg) { }
            virtual Value getInitVal() const = 0;
            virtual Value getStep() const = 0;

            virtual String toString() const = 0;
        };

        class BasicIndVar : public IndVar {
        public:
            Value initVal{};
            Value step{};
            bool subtractStep = false;

        public:
            BasicIndVar(RegPtr indReg, Value initVal, Value step, bool subtractStep)
                : IndVar(indReg), initVal(initVal), step(step), subtractStep(subtractStep) { }
            static Ptr<BasicIndVar> create(RegPtr indReg, Value initVal, Value step, bool subtractStep) {
                return makePtr<BasicIndVar>(indReg, initVal, step, subtractStep);
            }
            Value getInitVal() const override { return initVal; }
            Value getStep() const override { return step; }

            String toString() const override {
                return indReg->toString() + ": " + initVal.toString() + ", " + step.toString();
            }
        };

        class DerivedIndVar : public IndVar {
        public:
            InstPtr defInst = nullptr;
            Ptr<IndVar> dep = nullptr;
            Value coef{};
            Value bias{};
            bool recipCoef = false;
            bool negBias = false;
            bool isMulDiv = false;

            Value computedInitVal{};
            Value computedStep{};

            bool inDesiredForm = false;

        public:
            DerivedIndVar(RegPtr indReg, InstPtr defInst, Ptr<IndVar> dep, Value coef, Value bias, bool recipCoef, bool negBias)
                : IndVar(indReg), defInst(defInst), dep(dep), coef(coef), bias(bias), recipCoef(recipCoef), negBias(negBias) { }
            static Ptr<DerivedIndVar> create(RegPtr indReg, InstPtr defInst, Ptr<IndVar> dep, Value coef, Value bias, bool recipCoef, bool negBias) {
                return makePtr<DerivedIndVar>(indReg, defInst, dep, coef, bias, recipCoef, negBias);
            }
            Value getInitVal() const override { return computedInitVal; }
            Value getStep() const override { return computedStep; }

            String toString() const override {
                return indReg->toString() + ": " + dep->indReg->toString() + (coef.hasValue() ? (recipCoef ? " / " : " * ") + coef.toString() : "") + (bias.hasValue() ? (negBias ? " - " : " + ") + bias.toString() : "");
            }
        };

        Set<Ptr<IndVar>> ivs{};
        Map<RegPtr, Ptr<IndVar>> ivMap{};
        Map<RegPtr, Ptr<BasicIndVar>> basicConjMap{};

        LoopIndVarAnalysisContext(ir::LoopInvariantAnalysisContext &loopInvarCtx);
        LoopIndVarAnalysisContext(const LoopIndVarAnalysisContext &other) = delete;

        void printDebug() const {
            dbgout << "Induction variables of loop " << loop->headerBB()->label() << ":" << std::endl;
            if (ivMap.size() == 0) {
                dbgout << "No induction variables found." << std::endl;
            } else {
                for (auto [reg, indVar] : ivMap) {
                    dbgout << indVar->toString() << std::endl;
                }
            }
            dbgout << loop->headerBB()->parentFunc()->toString(); // DEBUG
        }
    };
}
