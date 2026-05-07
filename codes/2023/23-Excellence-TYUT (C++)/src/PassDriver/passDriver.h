//
// Created by 牛奕博 on 2023/5/29.
//
#ifndef ANUC_PASSDRIVER_H
#define ANUC_PASSDRIVER_H
#include <memory>
#include <vector>
#include "core.h"
#include "irBuilder.h"
#include "ssa.h"
#include "scheduleBeforeRA.h"
#include "lowerToLIR.h"
#include "rvValue.h"
#include "sbRegSpill.h"
#include "sbRegAlloc.h"
#include "ssaLivenessAnalysis.h"
#include "instEmit.h"
#include "handleFunctionCall.h"
#include "functionCallAnalysis.h"
using namespace std;
namespace anuc {
    class PassDriver {
        unique_ptr<Module> M;
        unique_ptr<IRBuilder> Builder;
        unique_ptr<RegTable> regTable;
        string fileName;
    public:
        PassDriver(unique_ptr<Module> M, unique_ptr<IRBuilder> Builder, unique_ptr<RegTable> regTable) :
                M(std::move(M)), Builder(std::move(Builder)) {}

        PassDriver(unique_ptr<Module> M, unique_ptr<IRBuilder> Builder, string fileName) :
                M(std::move(M)), Builder(std::move(Builder)), fileName(fileName) {
            regTable = make_unique<RegTable>();
        }

        void run() {
            //SSA
            runSSAPass();
            //调度器
            //runScheduleBRPass();
            //降级
            backEndPass();
        }


    private:
        void backEndPass() {
            //降级
            runLowerPass1();
            runLowerPass2();
            //M->print("../files/l.ll");
            runRaSpillPass();
            runLowerPass3();
            auto callGraph = runRaAllocaPass();
            runHandleFuncCallPass();
            instEmitPass(callGraph);
        }

        void runSSAPass() {
            for (auto fn = M->getBegin(); fn != M->getEnd(); ++fn) {
                SSAPass(&*fn, Builder.get()).run();
            }
        };

        void runScheduleBRPass() {
            for (auto fn = M->getBegin(); fn != M->getEnd(); ++fn) {
                for (auto bb = (*fn).getBegin(); bb != (*fn).getEnd(); ++bb) {
                    BasicBlock *block = &*bb;
                    blockDFSCalulator bdc;
                    vector<BasicBlock *> postOrder;
                    bdc.calculateBBPostOrder(postOrder, block);
                    LivenessAnalysis la;
                    la.instLivenessCalculator(postOrder);
                    ScheduleBRPass sra(la);
                    sra.build(block);
                    sra.schedule();
                }
            }
        }


        void runLowerPass1() {
            //处理gep
            LIRVisitor0 *visitor0 = new LIRVisitor0(Builder.get());
            for (auto fn = M->getBegin(); fn != M->getEnd(); ++fn) {
                for (auto bb = (*fn).getBegin(); bb != (*fn).getEnd(); ++bb) {
                    for (auto inst = (*bb).getBegin(); inst != (*bb).getEnd();) {
                        Instruction *i = &*inst;
                        ++inst;
                        i->accept(visitor0);
                    }
                }
            }
            delete visitor0;

            //处理其他的
            LIRVisitor1 *visitor1 = new LIRVisitor1(Builder.get(), regTable.get());
            for (auto fn = M->getBegin(); fn != M->getEnd(); ++fn) {
                for (auto bb = (*fn).getBegin(); bb != (*fn).getEnd(); ++bb) {
                    for (auto inst = (*bb).getBegin(); inst != (*bb).getEnd();) {
                        Instruction *i = &*inst;
                        ++inst;
                        i->accept(visitor1);
                    }
                }
            }
            delete visitor1;
            M->memoryClean();
        }

        void runLowerPass2() {
            LIRVisitor2 *visitor2 = new LIRVisitor2(Builder.get(), regTable.get());
            for (auto fn = M->getBegin(); fn != M->getEnd(); ++fn) {
                for (auto bb = (*fn).getBegin(); bb != (*fn).getEnd(); ++bb) {
                    for (auto inst = (*bb).getBegin(); inst != (*bb).getEnd();) {
                        Instruction *i = &*inst;
                        ++inst;
                        i->accept(visitor2);
                    }
                }
            }
            delete visitor2;
            M->memoryClean();
        }
        map<Value*, Value*> spillArgs;
        void runRaSpillPass() {
            for (auto fn = M->getBegin(); fn != M->getEnd(); ++fn) {
                SSALivenessAnalysis ssaa(&*fn, regTable.get());
                auto &liveInfo = ssaa.computeLiveness();
                //ssaa.printLiveOut();
                SBRegSpill sbs(&*fn, Builder.get(), regTable.get(), liveInfo, spillArgs);
                sbs.run();

            }
        }

        void runLowerPass3() {
            LIRVisitor3 *visitor3 = new LIRVisitor3(Builder.get(), regTable.get());
            for (auto fn = M->getBegin(); fn != M->getEnd(); ++fn) {
                auto func = &*fn;
                for (auto bb = (*fn).getBegin(); bb != (*fn).getEnd(); ++bb) {
                    for (auto inst = (*bb).getBegin(); inst != (*bb).getEnd();) {
                        Instruction *i = &*inst;
                        ++inst;
                        i->accept(visitor3);
                    }
                }
            }
            M->memoryClean();
        }

        //存储函数信息 函数--被调用者保存寄存器
        map<Function *, set<RvRegister*>> calledSaved;

        //存储call指令信息 callInst--调用者保存寄存器
        map<CallInst*, set<RvRegister*>> callerSaved;

        map<Function*, CallNode*> runRaAllocaPass() {
            //计算存储函数信息的dag图
            auto callGraph = FunctionCallAnalysis(M.get()).runAnalysis();
            for (auto fn = M->getBegin(); fn != M->getEnd(); ++fn) {
                SSALivenessAnalysis ssaa(&*fn, regTable.get());
                auto &liveInfo = ssaa.computeLiveness();
                SBRegAlloc sba(&*fn, Builder.get(), regTable.get(),
                               liveInfo, calledSaved, callerSaved,
                               callGraph[&*fn]);
                sba.run();
            }
            M->memoryClean();
            return callGraph;
        }
        void runHandleFuncCallPass() {
            //通过callinfo传递call的处理信息
            HandleFunctionCall hfc(M.get(), Builder.get(), regTable.get(),
                                   callerSaved);
            hfc.transformCallInst();
        }

        void instEmitPass( map<Function*, CallNode*> &callGraph) {
            InstEmit ie(M.get(), Builder.get(),
                        fileName, calledSaved, callGraph);
            ie.run();
        }
    };
}


#endif //ANUC_PASSDRIVER_H
